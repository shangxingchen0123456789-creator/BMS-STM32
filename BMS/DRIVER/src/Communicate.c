#include "stm32f10x.h"
#include "Communicate.h"
#include "usart.h"
#include "bms_app.h"
#include "led.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stddef.h>

/* ------------------------- 时间基准 -------------------------
 * g_ms：在未启动 FreeRTOS 调度器前，使用 1ms 软计数作为时基。
 * Comm_On1msTick()：应在 SysTick 或定时器中断里每 1ms 调用一次。
 * get_tick()：若 RTOS 已启动，则用 FreeRTOS 的 tick；否则用 g_ms。
 * 这样在裸机阶段也能跑（比如上电早期、调度器未启动时）。
 */
static volatile uint32_t g_ms = 0;

void Comm_On1msTick(void)
{
    g_ms++;
}

uint32_t get_tick(void)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        return xTaskGetTickCount();  // FreeRTOS tick（单位通常是 1ms，取决于 configTICK_RATE_HZ）
    }
    return g_ms;                     // 裸机阶段的 1ms 计数
}

/* ------------------------- 协议与缓冲区常量 -------------------------
 * 帧格式（小端）：
 *  Byte0  = 0x55
 *  Byte1  = 0xAA
 *  Byte2  = CMD（命令字）
 *  Byte3  = SEQ（序号）
 *  Byte4  = LEN_L（负载长度低字节）
 *  Byte5  = LEN_H（负载长度高字节）
 *  Byte6..6+LEN-1 = PAYLOAD（负载）
 *  CRC_L  = CRC16_IBM 低字节（对 Byte2~Byte5+Payload 做）
 *  CRC_H  = CRC16_IBM 高字节
 */
#define COMM_HEADER0        0x55
#define COMM_HEADER1        0xAA
#define COMM_TX_BUF_SIZE    128u   // 发送缓冲区长度（发出去的最大帧长不能超过它）
#define COMM_RX_BUF_SIZE    512u   // 接收环形缓冲区长度
#define COMM_MAX_PAYLOAD    100u   // 协议约束的最大负载长度
#define COMM_HEADER_LEN     6u     // 固定头长度（不含 CRC）
#define COMM_CRC_LEN        2u     // CRC16 长度

static uint8_t  txbuf[COMM_TX_BUF_SIZE];
static uint8_t  rxbuf[COMM_RX_BUF_SIZE];
static volatile uint16_t rx_write_idx = 0;  // ISR 写指针
static volatile uint16_t rx_read_idx  = 0;  // 任务读指针
static volatile uint32_t rx_overflow_cnt = 0; // 溢出计数（丢字节次数）

/* ------------------------- 流数据发送控制 -------------------------
 * g_streaming：是否开启流模式（自动周期发遥测）
 * g_seq：发送序号（可自增）
 * g_stream_interval_ms：流发送的周期（ms）
 * g_last_stream_tick：上次流发送的时间戳
 */
static volatile uint8_t  g_streaming = 0;
static volatile uint8_t  g_seq = 0;
static volatile uint16_t g_stream_interval_ms = 500;
static uint32_t g_last_stream_tick = 0;

/* 从环形缓冲区 rbase 起偏移 offset 读取 1 字节（rbase+offset 会取模） */
static inline uint8_t ring_peek(uint16_t base, uint16_t offset)
{
    return rxbuf[(uint16_t)((base + offset) % COMM_RX_BUF_SIZE)];
}

/* ------------------------- CRC16（Modbus/IBM） -------------------------
 * 多项式 0xA001，初值 0xFFFF，按位右移/LSB-first。
 * 发送：对 txbuf[2..(2+4+plen-1)] 计算
 * 接收：对环缓中的对应窗口计算
 */
static uint16_t CRC16_IBM(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (uint16_t)((crc >> 1) ^ 0xA001);
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/* 在环形缓冲区中计算 CRC（避免复制数据） */
static uint16_t CRC16_IBM_from_ring(uint16_t base, uint16_t offset, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= ring_peek(base, (uint16_t)(offset + i));
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (uint16_t)((crc >> 1) ^ 0xA001);
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/* ------------------------- 小端写入的安全宏 -------------------------
 * WR_S16_SAFE / WR_U16_SAFE：向 payload 写入 16 位（小端序），并检查不越界。
 * 典型用途：打包遥测数据到 payload[]。
 * buf：目标缓存；w：写指针（会自增）；val：要写入的值；max：buf 最大长度。
 * do{...}while(0) 是“宏安全包裹”写法，防止 if/else 缺花括号时的副作用。
 */
#define WR_S16_SAFE(buf, w, val, max)                 \
    do {                                              \
        if ((w) + 2u <= (max)) {                      \
            int16_t _v = (val);                       \
            (buf)[(w)++] = (uint8_t)(_v & 0xFF);      \
            (buf)[(w)++] = (uint8_t)((uint16_t)_v >> 8); \
        }                                             \
    } while (0)

#define WR_U16_SAFE(buf, w, val, max)                 \
    do {                                              \
        if ((w) + 2u <= (max)) {                      \
            uint16_t _v = (val);                      \
            (buf)[(w)++] = (uint8_t)(_v & 0xFF);      \
            (buf)[(w)++] = (uint8_t)(_v >> 8);        \
        }                                             \
    } while (0)

/* ------------------------- 发送一帧 -------------------------
 * 会按协议组织头、负载、CRC，并通过 usart_send() 发出。
 * 注意：
 *  - 限制 plen <= COMM_MAX_PAYLOAD
 *  - 总长度不得超过 COMM_TX_BUF_SIZE
 *  - CRC 计算从 cmd 字段开始（即跳过 0x55 0xAA）
 */
void SendFrame(uint8_t cmd, uint8_t seq, const uint8_t *payload, uint16_t plen)
{
    if (plen > COMM_MAX_PAYLOAD) {
        return;
    }

    uint16_t total_len = COMM_HEADER_LEN + plen + COMM_CRC_LEN;
    if (total_len > COMM_TX_BUF_SIZE) {
        return;
    }

    uint16_t idx = 0;
    txbuf[idx++] = COMM_HEADER0;
    txbuf[idx++] = COMM_HEADER1;
    txbuf[idx++] = cmd;
    txbuf[idx++] = seq;
    txbuf[idx++] = (uint8_t)(plen & 0xFF);
    txbuf[idx++] = (uint8_t)(plen >> 8);

    if (plen > 0 && payload != NULL) {
        for (uint16_t i = 0; i < plen; i++) {
            txbuf[idx++] = payload[i];
        }
    }

    uint16_t crc = CRC16_IBM(&txbuf[2], (uint16_t)(4 + plen)); // 对 cmd..len..payload 做
    txbuf[idx++] = (uint8_t)(crc & 0xFF);
    txbuf[idx++] = (uint8_t)(crc >> 8);

    usart_send(txbuf, (uint8_t)idx); // idx <= 108（当前常量下），强转到 uint8_t 安全
}

/* 发送 ACK：负载是 1 字节的错误码（comm_err_t） */
static void SendAck(uint8_t seq, comm_err_t err)
{
    uint8_t payload = (uint8_t)err;
    SendFrame(CMD_ACK, seq, &payload, 1);
}

/* ------------------------- 发送一次遥测 -------------------------
 * 从 BMS 层获取结构体 BMS_Telemetry_t，并序列化到 payload（小端）。
 * 使用 WR_*_SAFE 宏确保不越界写入。
 */
static void SendTelemetry(uint8_t seq)
{
    BMS_Telemetry_t telem;
    if (BMS_GetTelemetry(&telem) != pdTRUE) {  // 若数据暂不可用，则直接返回
        return;
    }

    uint8_t payload[96] = {0};
    const uint16_t max_payload = sizeof(payload);
    uint16_t w = 0;

    // 一些 1 字节标志/百分比
    payload[w++] = telem.charge_enabled;
    payload[w++] = telem.discharge_enabled;
    payload[w++] = telem.soc_pct;
    payload[w++] = telem.soh_pct;

    // 电压、电流（小端）
    WR_U16_SAFE(payload, w, telem.pack_voltage_mv, max_payload);
    WR_S16_SAFE(payload, w, telem.current_ma, max_payload);

    // 温度（单位 0.1℃，带符号）
    for (uint8_t i = 0; i < BMS_TEMP_SENSOR_COUNT; i++) {
        WR_S16_SAFE(payload, w, telem.temperature_dC[i], max_payload);
    }

    // 循环次数、容量（10mAh 单位）
    WR_U16_SAFE(payload, w, telem.cycle_count, max_payload);
    WR_U16_SAFE(payload, w, telem.learned_capacity_10mAh, max_payload);
    WR_U16_SAFE(payload, w, telem.remaining_capacity_10mAh, max_payload);

    // 单体电压（如果 BMS_MAX_CELLS 较大，宏会自己防越界）
    for (uint8_t i = 0; i < BMS_MAX_CELLS && (w + 2u) <= max_payload; i++) {
        WR_S16_SAFE(payload, w, telem.cell_mv[i], max_payload);
    }

    // 故障位、状态枚举
    WR_U16_SAFE(payload, w, telem.fault_flags, max_payload);
    payload[w++] = (uint8_t)telem.state;

    // 时间戳（4 字节小端）
    uint32_t ts = telem.timestamp_ms;
    if ((w + 4u) <= max_payload) {
        payload[w++] = (uint8_t)(ts & 0xFFu);
        payload[w++] = (uint8_t)((ts >> 8) & 0xFFu);
        payload[w++] = (uint8_t)((ts >> 16) & 0xFFu);
        payload[w++] = (uint8_t)((ts >> 24) & 0xFFu);
    }

    SendFrame(CMD_TELEMETRY, seq, payload, w);
}

/* ------------------------- 命令处理 -------------------------
 * 处理上位机下发的命令（PING、开始/停止流、充放电开关、单次读取等）。
 * 典型流程：解析 -> HandleCommand -> 发送 ACK/遥测。
 */
static void HandleCommand(uint8_t cmd, uint8_t seq, const uint8_t *payload, uint16_t len)
{
    switch (cmd) {
    case CMD_PING:
        SendAck(seq, COMM_OK);
        break;

    case CMD_STREAM_START: {
        // payload[0..1] = interval_ms（小端）
        if (payload != NULL && len >= 2u) {
            uint16_t interval = (uint16_t)(payload[0] | ((uint16_t)payload[1] << 8));
            Comm_SetStreamInterval(interval);
        }
        g_streaming = 1;
        g_seq = seq;           // 用对端的 seq 作为起点（可选）
        SendAck(seq, COMM_OK);
        SendTelemetry(g_seq++); // 立即回一帧
        break;
    }

    case CMD_STREAM_STOP:
        g_streaming = 0;
        SendAck(seq, COMM_OK);
        break;

    case CMD_CHG_ON: {
        BMS_RequestChargeEnable(1);
        SendAck(seq, COMM_OK);
        SendTelemetry(seq);
        break;
    }

    case CMD_CHG_OFF: {
        BMS_RequestChargeEnable(0);
        SendAck(seq, COMM_OK);
        SendTelemetry(seq);
        break;
    }

    case CMD_DSG_ON: {
        BMS_RequestDischargeEnable(1);
        SendAck(seq, COMM_OK);
        SendTelemetry(seq);
        break;
    }

    case CMD_DSG_OFF: {
        BMS_RequestDischargeEnable(0);
        SendAck(seq, COMM_OK);
        SendTelemetry(seq);
        break;
    }

    case CMD_READ_ONCE:
        SendTelemetry(seq); // 单次读取遥测
        break;

    case CMD_SET_THRESHOLDS: {
        if (len < 20) { SendAck(seq, COMM_ERR_BAD_LEN); break; }
        BMS_Thresholds_t t;
        t.cell_ov_mv         = (uint16_t)(payload[0]  | (payload[1]  << 8));
        t.cell_ov_release_mv = (uint16_t)(payload[2]  | (payload[3]  << 8));
        t.cell_uv_mv         = (uint16_t)(payload[4]  | (payload[5]  << 8));
        t.cell_uv_release_mv = (uint16_t)(payload[6]  | (payload[7]  << 8));
        t.ocd_ma             = (uint32_t)payload[8]  | ((uint32_t)payload[9]  << 8)
                             | ((uint32_t)payload[10] << 16) | ((uint32_t)payload[11] << 24);
        t.chg_ot_dC = (int16_t)(payload[12] | (payload[13] << 8));
        t.chg_ut_dC = (int16_t)(payload[14] | (payload[15] << 8));
        t.dsg_ot_dC = (int16_t)(payload[16] | (payload[17] << 8));
        t.dsg_ut_dC = (int16_t)(payload[18] | (payload[19] << 8));
        BMS_SetThresholds(&t);
        SendAck(seq, COMM_OK);
        break;
    }

    default:
        SendAck(seq, COMM_ERR_UNSUPPORT); // 未支持的命令
        break;
    }
}

/* ------------------------- 解析接收环形缓冲区 -------------------------
 * 功能：在环缓中扫描、对齐到 0x55 0xAA 头，检查长度与 CRC，通过后调用 HandleCommand()。
 * 要点：
 *  - 非阻塞解析：可一次解析多帧
 *  - 头错位时跳一字节继续找（LED_TOGGLE 用于可视化提示丢同步）
 *  - 仅复制 payload 到 paybuf，其余通过 ring_peek 零拷贝读取
 */
static void ParseRxBuffer(void)
{
    uint16_t r = rx_read_idx; // 本地读指针快照

    while (1) {
        uint16_t w = rx_write_idx; // 写指针快照（volatile 防止被编译器优化）
        // 计算当前可读字节数（处理环绕）
        uint16_t available = (w >= r) ? (uint16_t)(w - r) : (uint16_t)(COMM_RX_BUF_SIZE - r + w);

        if (available < COMM_HEADER_LEN + COMM_CRC_LEN) {
            // 不足以构成最小帧，先退出等待更多数据
            break;
        }

        // 对齐帧头（0x55 0xAA），若不匹配则丢 1 字节
        if (ring_peek(r, 0) != COMM_HEADER0 || ring_peek(r, 1) != COMM_HEADER1) {
            r = (uint16_t)((r + 1) % COMM_RX_BUF_SIZE);
            LED_TOGGLE(); // 可选：用来观察乱流/丢包
            continue;
        }

        // 读取负载长度（小端）
        uint16_t plen = (uint16_t)(ring_peek(r, 4) | ((uint16_t)ring_peek(r, 5) << 8));
        uint16_t frame_len = (uint16_t)(COMM_HEADER_LEN + plen + COMM_CRC_LEN);

        // 保护：长度越界（协议层面的拒绝）
        if (plen > COMM_MAX_PAYLOAD || frame_len > COMM_TX_BUF_SIZE) {
            // 这里第二个判断冗余但安全：已限制 plen<=COMM_MAX_PAYLOAD，frame_len 就不会超过 108
            r = (uint16_t)((r + 1) % COMM_RX_BUF_SIZE);
            continue;
        }

        // 数据还没收齐，退出等待更多字节
        if (available < frame_len) {
            break;
        }

        // 读取并校验 CRC（接收区内做窗口 CRC）
        uint16_t crc_recv = (uint16_t)(ring_peek(r, (uint16_t)(COMM_HEADER_LEN + plen)) |
                               ((uint16_t)ring_peek(r, (uint16_t)(COMM_HEADER_LEN + plen + 1)) << 8));
        uint16_t crc_calc = CRC16_IBM_from_ring(r, 2u, (uint16_t)(4u + plen)); // 从 cmd 起算 4+plen 字节

        if (crc_recv != crc_calc) {
            // CRC 错，丢 1 字节重找帧头
            r = (uint16_t)((r + 1) % COMM_RX_BUF_SIZE);
            continue;
        }

        // 提取命令与序号
        uint8_t cmd = ring_peek(r, 2);
        uint8_t seq = ring_peek(r, 3);

        // 拷贝负载到局部缓冲（命令处理可能需要连续内存）
        static uint8_t paybuf[COMM_MAX_PAYLOAD];
        const uint8_t *payload = NULL;
        if (plen > 0) {
            for (uint16_t i = 0; i < plen; i++) {
                paybuf[i] = ring_peek(r, (uint16_t)(COMM_HEADER_LEN + i));
            }
            payload = paybuf;
        }

        // 分发命令
        HandleCommand(cmd, seq, payload, plen);

        // 前移读指针到下一帧
        r = (uint16_t)((r + frame_len) % COMM_RX_BUF_SIZE);
    }

    // 提交读指针
    rx_read_idx = r;
}

/* ------------------------- 串口接收回调（字节流） -------------------------
 * 典型在 USART 中断里调用：Comm_RxCallback(USART_ReceiveData());
 * 无锁环形缓冲：
 *  - 只有中断写（rx_write_idx++）
 *  - 只有任务读（ParseRxBuffer 内推进 r，最后提交 rx_read_idx）
 *  - 溢出时：丢弃新字节并计数；ISR 不推进 rx_read_idx，避免和解析任务抢写读指针。
 */
void Comm_RxCallback(uint8_t byte)
{
    uint16_t next = (uint16_t)((rx_write_idx + 1) % COMM_RX_BUF_SIZE);
    if (next == rx_read_idx) {
        rx_overflow_cnt++;
        return;
    }
    rxbuf[rx_write_idx] = byte;
    rx_write_idx = next;
}

/* ------------------------- 周期任务（在主循环或 RTOS 任务中调用） -------------------------
 * 功能：
 *  1) 调用解析器处理接收数据
 *  2) 若开启流模式且到达周期，则发送一帧遥测
 */
void Comm_PeriodicTask(void)
{
    ParseRxBuffer();

    if (g_streaming) {
        uint32_t now = get_tick();
        if ((uint32_t)(now - g_last_stream_tick) >= g_stream_interval_ms) {
            SendTelemetry(g_seq++);
            g_last_stream_tick = now;
        }
    }
}

/* 查询是否处于流模式 */
uint8_t Comm_IsStreaming(void)
{
    return g_streaming;
}

/* 设置流模式的发送周期（下限 20ms），并将“上次发送时间”重置为当前 */
void Comm_SetStreamInterval(uint16_t interval_ms)
{
    if (interval_ms < 20u) {
        interval_ms = 20u;
    }
    g_stream_interval_ms = interval_ms;
    g_last_stream_tick = get_tick();
}

/* 返回环缓溢出计数（便于统计丢字节情况） */
uint32_t Comm_GetRxOverflowCount(void)
{
    return rx_overflow_cnt;
}

/* ------------------------- 模块初始化 -------------------------
 * 重置索引、计数与流控制状态，并初始化“上次发送时间”。
 */
void Comm_Init(void)
{
    rx_write_idx = 0;
    rx_read_idx = 0;
    rx_overflow_cnt = 0;

    g_streaming = 0;
    g_seq = 0;
    g_stream_interval_ms = 500;
    g_last_stream_tick = get_tick();
}
