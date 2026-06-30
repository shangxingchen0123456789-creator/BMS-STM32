#include "bms_app.h"

#include <math.h>
#include <string.h>
#include <limits.h>

#include "bq76940.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"

/*─────────────────────────────────────────────────────────────────────────────
 * 文件功能总览
 *  本文件实现了一个典型的 BMS（电池管理系统）上层应用逻辑，运行在 FreeRTOS 上。
 *
 *  核心设计思想（可以答辩时重点讲）：
 *    1) 任务分层（pipeline 式）：测量(Meas) → 故障(Fault) → SOC计算(SOC) → 均衡(Balance) → 控制(Control)
 *       - 每个功能单元独立为一个 FreeRTOS 任务，降低模块耦合、便于扩展和调试；
 *       - 测量驱动后续所有逻辑，形成周期性的“处理流水线”。
 *
 *    2) 并发互斥设计：
 *       - 访问共享运行状态结构体 g_bms_state 时，统一通过 s_state_mutex 互斥锁保护；
 *       - 访问 BQ76940 的 I2C 接口（任何 BQ76940_* API）统一通过 s_bq_mutex 保护，
 *         保证不会出现多个任务同时访问 I2C 总线造成冲突。
 *
 *    3) 事件驱动（Event-driven）：
 *       - 使用一个 EventGroup（s_events）实现任务间同步；
 *       - 典型事件：
 *            BMS_EVT_MEASUREMENT   ：测量任务产生，通知 Fault/SOC/Balance/Control 任务有新数据；
 *            BMS_EVT_FAULTS_UPDATED：Fault 任务产生，通知 Control 任务故障状态更新；
 *            BMS_EVT_SOC_READY     ：SOC 任务产生，通知 Control 任务 SOC/SOH 已更新；
 *            BMS_EVT_HOST_CMD      ：主机（上位机）修改充放请求时产生，通知 Control 任务尽快评估。
 *
 *  数据单位统一约定（方便代码阅读和上位机显示）：
 *    - 电压：   mV（毫伏）
 *    - 电流：   mA（毫安），约定“放电为负，充电为正”
 *    - 温度：   0.1℃（decicelsius），例如 253 表示 25.3℃
 *    - 容量：   mAh（毫安时）
 *    - 时间：   FreeRTOS tick，换算时使用 configTICK_RATE_HZ
 *
 *  注意点：
 *    - dt_hours 使用 configTICK_RATE_HZ 换算，不依赖 1 tick = 1 ms。
 *
 *    - PACK 过压/欠压阈值现在按 “单体阈值 × BMS_MAX_CELLS” 近似计算。
 *      如果实际有效电芯数少于 BMS_MAX_CELLS，可以考虑改为 “× 实际 cell_count”，
 *      以避免低串数场合过压/欠压判断偏差太大。
 *
 *    - 均衡策略只在三个条件都满足时启用：
 *        (1) 允许充电（charge_enabled=1）
 *        (2) 没有故障（fault_mask=0）
 *        (3) 至少有一个有效电芯（cell_count>0）
 *      同时利用“开启/关闭两个不同阈值”实现滞回，避免电压在临界点时频繁开关均衡。
 *─────────────────────────────────────────────────────────────────────────────
 */


/* ───────── 全局变量：唯一状态对象与同步原语 ───────── */

/* 系统全局运行时状态，用于保存最新测量、故障、SOC、均衡、FET 状态等。
 * 所有任务通过互斥锁访问，避免并发冲突。
 */
static BMS_RuntimeState_t g_bms_state;

/* 互斥锁：保护 g_bms_state（状态访问） */
static SemaphoreHandle_t   s_state_mutex = NULL;

/* 互斥锁：保护 BQ76940 的 I2C 访问（任何 BQ76940_* API 前必须加锁） */
static SemaphoreHandle_t   s_bq_mutex = NULL;

/* 事件组：用于任务之间的事件通知和同步（事件驱动架构的核心） */
static EventGroupHandle_t  s_events = NULL;

/* ───────── 运行时可配置阈值（上位机可通过 CMD_SET_THRESHOLDS 修改） ───────── */
BMS_Thresholds_t g_thresholds = {
    .cell_ov_mv         = BMS_CELL_OV_THRESHOLD_MV,     /* 4200 */
    .cell_ov_release_mv = BMS_CELL_OV_RELEASE_MV,       /* 4100 */
    .cell_uv_mv         = BMS_CELL_UV_THRESHOLD_MV,     /* 2700 */
    .cell_uv_release_mv = BMS_CELL_UV_RELEASE_MV,       /* 2850 */
    .ocd_ma             = BMS_CURRENT_OC_DISCHARGE_MA,   /* 100000 */
    .chg_ot_dC          = BMS_TEMP_CHARGE_MAX_DECIC,     /* 450 */
    .chg_ut_dC          = BMS_TEMP_CHARGE_MIN_DECIC,     /* 0   */
    .dsg_ot_dC          = BMS_TEMP_DISCHARGE_MAX_DECIC,  /* 600 */
    .dsg_ut_dC          = BMS_TEMP_DISCHARGE_MIN_DECIC   /* -200 */
};

/* 前置声明：锁/解锁函数（定义在后方，此处先声明以便 BMS_SetThresholds 使用） */
static BaseType_t BMS_LockState(TickType_t ticks);
static void       BMS_UnlockState(void);

static uint16_t clamp_cfg_u16(uint16_t value, uint16_t minv, uint16_t maxv)
{
    if (value < minv) return minv;
    if (value > maxv) return maxv;
    return value;
}

static int16_t clamp_cfg_i16(int16_t value, int16_t minv, int16_t maxv)
{
    if (value < minv) return minv;
    if (value > maxv) return maxv;
    return value;
}

void BMS_SetThresholds(const BMS_Thresholds_t *t)
{
    if (!t) return;
    BMS_Thresholds_t safe = *t;

    safe.cell_ov_mv         = clamp_cfg_u16(safe.cell_ov_mv, 3800U, 4300U);
    safe.cell_ov_release_mv = clamp_cfg_u16(safe.cell_ov_release_mv, 3600U, safe.cell_ov_mv);
    safe.cell_uv_mv         = clamp_cfg_u16(safe.cell_uv_mv, 2000U, 3300U);
    safe.cell_uv_release_mv = clamp_cfg_u16(safe.cell_uv_release_mv, safe.cell_uv_mv, 3600U);
    safe.ocd_ma             = (safe.ocd_ma < 100U) ? 100U : safe.ocd_ma;
    if (safe.ocd_ma > 20000UL) safe.ocd_ma = 20000UL;
    safe.chg_ot_dC          = clamp_cfg_i16(safe.chg_ot_dC, 100, 700);
    safe.chg_ut_dC          = clamp_cfg_i16(safe.chg_ut_dC, -400, 200);
    safe.dsg_ot_dC          = clamp_cfg_i16(safe.dsg_ot_dC, 100, 800);
    safe.dsg_ut_dC          = clamp_cfg_i16(safe.dsg_ut_dC, -400, 200);

    if (BMS_LockState(pdMS_TO_TICKS(50)) == pdTRUE) {
        g_thresholds = safe;
        BMS_UnlockState();
    }
}

/* ───────── 小工具：浮点/整数钳位（避免越界） ───────── */

/* 浮点数钳位到 [minv, maxv] 区间，用于限制 SOC/SOH 等计算结果不跑飞 */
static inline float clampf_range(float value, float minv, float maxv)
{
    if (value < minv) {
        return minv;
    }
    if (value > maxv) {
        return maxv;
    }
    return value;
}

/* 将浮点数转换为 uint8，并确保在 [0,255] 范围内，带四舍五入 */
static inline uint8_t clamp_u8(float value)
{
    if (value < 0.0f) {
        return 0U;
    }
    if (value > 255.0f) {
        return 255U;
    }
    return (uint8_t)(value + 0.5f); /* 四舍五入到最近的 8 位无符号 */
}

/* 将浮点数转换为 uint16，并确保在 [0,65535] 范围内，带四舍五入 */
static inline uint16_t clamp_u16(float value)
{
    if (value < 0.0f) {
        return 0U;
    }
    if (value > 65535.0f) {
        return 65535U;
    }
    return (uint16_t)(value + 0.5f);
}

/* ───────── 统一封装：锁/解锁状态对象 ─────────
 * 目的：
 *   - 封装对 s_state_mutex 的访问，避免在各处直接调用 xSemaphoreTake/Give，方便日后替换为递归锁或临界区。
 *   - 提供空指针保护（若 mutex 未初始化，直接返回失败），避免系统未初始化时非法访问。
 */
static BaseType_t BMS_LockState(TickType_t ticks)
{
    if (s_state_mutex == NULL) {
        return pdFALSE;
    }
    return xSemaphoreTake(s_state_mutex, ticks);
}

static void BMS_UnlockState(void)
{
    /* 此处默认 s_state_mutex 非空，由调用路径保证 */
    xSemaphoreGive(s_state_mutex);
}

/* ───────── 统一封装：锁/解锁 BQ 访问 ─────────
 * 任何 BQ76940_* API（I2C 读写）都必须在持有 s_bq_mutex 时调用。
 * 这样即使多个任务（Measure/Fault/Balance/Control）都操作 BQ，
 * 也不会出现 I2C 总线抢占和乱序访问。
 */
static BaseType_t BMS_LockBQ(TickType_t ticks)
{
    if (s_bq_mutex == NULL) {
        return pdFALSE;
    }
    return xSemaphoreTake(s_bq_mutex, ticks);
}

static void BMS_UnlockBQ(void)
{
    xSemaphoreGive(s_bq_mutex);
}

/*─────────────────────────────────────────────────────────────────────────────
 * 函数：BMS_AppInit
 * 功能：系统初始化入口
 *  - 创建互斥锁和事件组；
 *  - 清空运行状态并初始化 SOC/SOH/库仑计等初值；
 *  - 创建 5 个 FreeRTOS 任务，并设置优先级。
 *
 * 任务优先级设计（相对 IDLE）：
 *   Balance(+1) < SOC(+2) ≈ Meas(+3) ≈ Control(+3) < Fault(+4)
 *
 * 各任务职责：
 *   - Meas：周期读取 BQ，得到最新测量数据，并发布事件 BMS_EVT_MEASUREMENT。
 *   - Fault：当 MEASUREMENT 事件到来时，根据 BQ 状态寄存器和软件阈值更新故障掩码；
 *            维护故障锁存与恢复计数，发布 BMS_EVT_FAULTS_UPDATED。
 *   - SOC：在 MEASUREMENT 事件到来后，进行电量积分和容量学习，更新 SOC/SOH，
 *          完成后发布 BMS_EVT_SOC_READY。
 *   - Balance：在 MEASUREMENT 事件到来后，根据电芯压差及故障/充电状态控制电芯均衡。
 *   - Control：综合主机请求、故障状态、SOC 和电流等，控制 CHG/DSG FET 开关，
 *              并维护运行状态机（IDLE/CHARGING/DISCHARGING/FAULT）。
 *─────────────────────────────────────────────────────────────────────────────
 */
void BMS_AppInit(void)
{
    /* 防止重复初始化：如果互斥锁已经存在，说明已经 init 过了，直接返回 */
    if (s_state_mutex != NULL) {
        return;
    }

    /* 创建用于保护 g_bms_state 的互斥锁 */
    s_state_mutex = xSemaphoreCreateMutex(); // 创建互斥锁，保护状态访问

    /* 创建用于保护 BQ I2C 访问的互斥锁 */
    s_bq_mutex = xSemaphoreCreateMutex();    // 保护 BQ 芯片访问

    /* 创建事件组，用于任务间通信 */
    s_events = xEventGroupCreate();

    /* 清零全局状态结构体，避免残留数据 */
    memset(&g_bms_state, 0, sizeof(g_bms_state));

    /* 上电时读取一次电压，用 OCV 查表估算初始 SOC，
     * 替代硬编码的 50%，提高 SOC 初始准确性。
     */
    {
        BQ76940_Data_t boot_data;
        memset(&boot_data, 0, sizeof(boot_data));
        BQ76940_Read_All_Data(&boot_data);
        uint8_t valid_cells = 0;
        uint32_t sum_mv = 0;
        uint8_t i;
        for (i = 0; i < 15; i++) {
            if (boot_data.cell_voltage[i] > 0) {
                valid_cells++;
                sum_mv += boot_data.cell_voltage[i];
            }
        }
        uint8_t init_soc = BQ76940_Calculate_SOC((uint16_t)sum_mv, valid_cells);
        if (init_soc == 0 && sum_mv == 0) {
            init_soc = (uint8_t)BMS_SOC_INITIAL_PERCENT;  /* 读取失败则回退默认值 */
        }

        g_bms_state.coulomb_mAh          = BMS_PACK_NOMINAL_CAPACITY_MAH * (init_soc / 100.0f);
        g_bms_state.learned_capacity_mAh = BMS_PACK_NOMINAL_CAPACITY_MAH;
        g_bms_state.learning_phase       = 0;
        g_bms_state.learning_start_soc_pct = 0.0f;
        g_bms_state.soc_pct              = init_soc;
    }
    g_bms_state.soh_pct              = 100U;
    g_bms_state.charge_request       = 1U;
    g_bms_state.discharge_request    = 1U;
    g_bms_state.charge_enabled       = 0U;
    g_bms_state.discharge_enabled    = 0U;
    g_bms_state.run_state            = BMS_STATE_IDLE;

    /* 创建测量任务：周期性读 BQ 并发布测量事件 */
    if (xTaskCreate(BMS_Task_Measure,
                    "Meas",
                    configMINIMAL_STACK_SIZE * 3U, /* 适度放大栈，避免堆栈溢出 */
                    NULL,
                    tskIDLE_PRIORITY + 3U,
                    NULL) != pdPASS) {
        /* 创建失败：进入死循环，等待看门狗复位或调试处理 */
        for (;;) { }
    }

    /* 创建故障任务：优先级最高，保证故障能尽快被检测和处理 */
    if (xTaskCreate(BMS_Task_Fault,
                    "Fault",
                    configMINIMAL_STACK_SIZE * 2U,
                    NULL,
                    tskIDLE_PRIORITY + 4U,
                    NULL) != pdPASS) {
        for (;;) { }
    }

    /* 创建 SOC 任务：负责库仑积分、容量学习、SOC/SOH 计算 */
    if (xTaskCreate(BMS_Task_SOC,
                    "SOC",
                    configMINIMAL_STACK_SIZE * 2U,
                    NULL,
                    tskIDLE_PRIORITY + 2U,
                    NULL) != pdPASS) {
        for (;;) { }
    }

    /* 创建均衡任务：优先级最低，均衡动作一般不需要非常实时 */
    if (xTaskCreate(BMS_Task_Balance,
                    "Balance",
                    configMINIMAL_STACK_SIZE * 2U,
                    NULL,
                    tskIDLE_PRIORITY + 1U,
                    NULL) != pdPASS) {
        for (;;) { }
    }

    /* 创建控制任务：根据测量/故障/SOC/主机请求综合控制 CHG/DSG FET */
    if (xTaskCreate(BMS_Task_Control,
                    "Ctrl",
                    configMINIMAL_STACK_SIZE * 2U,
                    NULL,
                    tskIDLE_PRIORITY + 3U,
                    NULL) != pdPASS) {
        for (;;) { }
    }
}


/*─────────────────────────────────────────────────────────────────────────────
 * 函数：BMS_RequestChargeEnable / BMS_RequestDischargeEnable
 * 功能：上位机/外部模块请求打开或关闭充电/放电
 *
 * 设计思路：
 *   - 这里只修改“请求位” charge_request / discharge_request，不直接操作 FET；
 *   - 真正是否打开 CHG/DSG FET 由 Control 任务统一决策，
 *     再调用 BQ76940_Enable_CHG / BQ76940_Enable_DSG 来控制。
 *   - 修改请求后，通过 BMS_EVT_HOST_CMD 事件通知 Control 任务尽快重新评估。
 *─────────────────────────────────────────────────────────────────────────────
 */
void BMS_RequestChargeEnable(uint8_t enable)
{
    /* 锁定状态结构体，保证读写原子性 */
    if (BMS_LockState(portMAX_DELAY) == pdTRUE) {
        /* 任何非 0 值都视为“请求允许充电” */
        g_bms_state.charge_request = (enable != 0U) ? 1U : 0U;
        BMS_UnlockState();

        /* 通知控制任务“主机命令已更新” */
        xEventGroupSetBits(s_events, BMS_EVT_HOST_CMD);
    }
}

void BMS_RequestDischargeEnable(uint8_t enable)
{
    if (BMS_LockState(portMAX_DELAY) == pdTRUE) {
        g_bms_state.discharge_request = (enable != 0U) ? 1U : 0U;
        BMS_UnlockState();
        /* 同样通知控制任务重新评估放电允许情况 */
        xEventGroupSetBits(s_events, BMS_EVT_HOST_CMD);
    }
}

/*─────────────────────────────────────────────────────────────────────────────
 * 函数：BMS_GetTelemetry
 * 功能：线程安全地读取当前 BMS 遥测数据
 *
 * 使用方式：
 *   - 上位机调用此函数，将当前 BMS 运行数据一次性拷贝到 out 结构体中；
 *   - 避免上位机多次跨任务读不同字段造成不一致。
 *
 * 注意：
 *   - 调用者不得传入 NULL 指针；
 *   - 函数内部使用状态互斥锁保护 g_bms_state；
 *   - 若当前存在故障锁存，则在返回的 fault_flags 中额外 OR 上 BMS_FAULT_LATCHED 位，
 *     方便上位机区分“当前有瞬时故障”与“系统处于锁存故障状态”。
 *─────────────────────────────────────────────────────────────────────────────
 */
BaseType_t BMS_GetTelemetry(BMS_Telemetry_t *out)
{
    // 调用者如果传入空指针，直接返回 pdFALSE，防止空指针解引用
    if (out == NULL) {
        return pdFALSE;
    }
    // 尝试在 20ms 内获取状态锁
    if (BMS_LockState(pdMS_TO_TICKS(20)) != pdTRUE) {
        return pdFALSE;
    }
    // 先清零输出结构体，保证未赋值的字段为 0，避免上位机读到随机值
    memset(out, 0, sizeof(*out));

    // 拷贝原始测量值和核心运行状态
    out->timestamp_ms        = g_bms_state.timestamp_ms;
    out->pack_voltage_mv     = g_bms_state.measurement.pack_mv;
    out->current_ma          = g_bms_state.measurement.current_ma;
    out->cell_count          = g_bms_state.measurement.cell_count;
    memcpy(out->cell_mv,
           g_bms_state.measurement.cell_mv,
           sizeof(out->cell_mv));
    memcpy(out->temperature_dC,
           g_bms_state.measurement.temperature_dC,
           sizeof(out->temperature_dC));

    // FET 及 SOC/SOH 信息
    out->charge_enabled          = g_bms_state.charge_enabled;
    out->discharge_enabled       = g_bms_state.discharge_enabled;
    out->soc_pct                 = g_bms_state.soc_pct;
    out->soh_pct                 = g_bms_state.soh_pct;
    out->cycle_count             = (uint16_t)(g_bms_state.cycle_count & 0xFFFFu);
    out->learned_capacity_10mAh  = g_bms_state.learned_capacity_10mAh;
    out->remaining_capacity_10mAh= g_bms_state.remaining_capacity_10mAh;

    // 故障标志
    out->fault_flags = g_bms_state.fault_mask;
    if (g_bms_state.fault_latched) {
        out->fault_flags |= BMS_FAULT_LATCHED; /* 显式标记锁存态，便于上位机显示 */
    }

    // 当前运行状态（IDLE/CHARGING/DISCHARGING/FAULT）
    out->state = g_bms_state.run_state;

    BMS_UnlockState();
    return pdTRUE;
}

/*─────────────────────────────────────────────────────────────────────────────
 * 函数：BMS_BuildFaultMask
 * 功能：将细分故障标志（结构体形式）汇总为统一的 16bit fault_mask 位图
 *
 * 设计原因：
 *   - 上层上位机/控制逻辑更适合处理一个“整型位图”，而不是一堆分散的 bool 标志；
 *   - 便于通过按位或运算合并、保存、传输。
 *─────────────────────────────────────────────────────────────────────────────
 */
static uint16_t BMS_BuildFaultMask(const BMS_FaultFlags_t *f)
{
    uint16_t mask = 0;
    if (f->cell_ov)       { mask |= BMS_FAULT_CELL_OV; }
    if (f->cell_uv)       { mask |= BMS_FAULT_CELL_UV; }
    if (f->pack_ov)       { mask |= BMS_FAULT_PACK_OV; }
    if (f->pack_uv)       { mask |= BMS_FAULT_PACK_UV; }
    if (f->ocd)           { mask |= BMS_FAULT_OCD; }
    if (f->scd)           { mask |= BMS_FAULT_SCD; }
    if (f->charge_ot)     { mask |= BMS_FAULT_CHG_OT; }
    if (f->charge_ut)     { mask |= BMS_FAULT_CHG_UT; }
    if (f->discharge_ot)  { mask |= BMS_FAULT_DSG_OT; }
    if (f->discharge_ut)  { mask |= BMS_FAULT_DSG_UT; }
    if (f->bq_xready)     { mask |= BMS_FAULT_BQ_XREADY; }
    if (f->bq_ovrd_alert) { mask |= BMS_FAULT_BQ_OVRD_ALERT; }
    return mask;
}

/*─────────────────────────────────────────────────────────────────────────────
 * 任务：BMS_Task_Measure（测量任务）
 *
 * 职责：
 *   - 周期性（例如每 100ms）读取 BQ76940 的数据：
 *       整包电压、电流、各单体电压、温度等；
 *   - 将原始数据规整为本地统一的数据结构 BMS_MeasurementSnapshot_t：
 *       - 单位转换：温度 float ℃ → int16_t 0.1℃；
 *       - 无效值屏蔽：无效温度标记为 INT16_MIN，无效电芯电压记 -1；
 *   - 更新 g_bms_state.measurement 和时间戳；
 *   - 通过事件组投递 BMS_EVT_MEASUREMENT，通知 SOC/Fault/Balance/Control 等任务。
 *
 * 并发注意：
 *   - 访问 BQ 前必须加 BMS_LockBQ()，避免多个任务同时访问 I2C；
 *   - 更新 g_bms_state 必须加 BMS_LockState()。
 *─────────────────────────────────────────────────────────────────────────────
 */
void BMS_Task_Measure(void *pvParameters)
{
    (void)pvParameters;

    /* 上电后先延时一段时间，确保 BQ 芯片和 I2C 总线稳定 */
    vTaskDelay(pdMS_TO_TICKS(50));

    /* 上一次有效测量值，用于跳变过滤（I2C 毛刺防护） */
    static int16_t  prev_cell_mv[BMS_MAX_CELLS];
    static uint16_t prev_pack_mv  = 0;
    static int16_t  prev_current  = 0;
    static uint8_t  prev_valid    = 0;  /* 首次运行时不过滤 */
    memset(prev_cell_mv, 0, sizeof(prev_cell_mv));

    /* 跳变过滤阈值：单次 100ms 内变化超过此值视为 I2C 读取错误 */
    #define CELL_GLITCH_MV   200   /* 电芯电压跳变门限 */
    #define PACK_GLITCH_MV   1500  /* 整包电压跳变门限（9串×~150mV） */
    #define CURR_GLITCH_MA   3000  /* 电流跳变门限 */

    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(BMS_MEASUREMENT_PERIOD_MS));

        BQ76940_Data_t raw;
        memset(&raw, 0, sizeof(raw));

        if (BMS_LockBQ(pdMS_TO_TICKS(20)) == pdTRUE) {
            BQ76940_Read_All_Data(&raw);
            BMS_UnlockBQ();
        } else {
            continue;
        }

        BMS_MeasurementSnapshot_t meas;
        memset(&meas, 0, sizeof(meas));
        meas.cell_count = 0U;

        /* 处理各单体电压，带 I2C 毛刺过滤 */
        for (uint8_t i = 0; i < BMS_MAX_CELLS; i++) {
            int16_t mv = raw.cell_voltage[i];
            
            /* 跳变过滤：如果与上次有效值差 > CELL_GLITCH_MV，
             * 认为是 I2C 读取错误，使用上次有效值。
             * 注意：必须放在 > 0 判断外，防止读到 0（剧烈跌落）时漏过过滤，
             * 否则会导致 cell_count 减少，进而误触发整包过压(PACK_OV)。*/
            if (prev_valid && prev_cell_mv[i] > 0) {
                int16_t delta = mv - prev_cell_mv[i];
                if (delta < 0) delta = -delta;
                if (delta > CELL_GLITCH_MV &&
                    mv > (int16_t)BMS_CELL_UV_THRESHOLD_MV &&
                    mv < (int16_t)BMS_CELL_OV_THRESHOLD_MV) {
                    mv = prev_cell_mv[i];  /* 拒绝异常值，保留上次 */
                }
            }
            
            if (mv > 0) {
                meas.cell_mv[i] = mv;
                prev_cell_mv[i] = mv;
                meas.cell_count++;
            } else {
                meas.cell_mv[i] = -1;
            }
        }

        /* 整包电压毛刺过滤 */
        {
            uint16_t pv = raw.pack_voltage;
            if (prev_valid && prev_pack_mv > 0 && pv > 0) {
                int32_t delta = (int32_t)pv - (int32_t)prev_pack_mv;
                if (delta < 0) delta = -delta;
                uint8_t n_cells = (meas.cell_count > 0U) ? meas.cell_count : BMS_MAX_CELLS;
                uint32_t pack_ov_mv = (uint32_t)BMS_CELL_OV_THRESHOLD_MV * n_cells;
                uint32_t pack_uv_mv = (uint32_t)BMS_CELL_UV_THRESHOLD_MV * n_cells;
                uint8_t dangerous_pack = (pv >= pack_ov_mv || pv <= pack_uv_mv) ? 1U : 0U;
                if (delta > PACK_GLITCH_MV && dangerous_pack == 0U) {
                    pv = prev_pack_mv;
                }
            }
            meas.pack_mv = pv;
            if (pv > 0) prev_pack_mv = pv;
        }

        /* 电流毛刺过滤 */
        {
            int16_t cur = raw.current;
            if (prev_valid) {
                int32_t delta = (int32_t)cur - (int32_t)prev_current;
                if (delta < 0) delta = -delta;
                if (delta > CURR_GLITCH_MA &&
                    cur > -(int16_t)BMS_CURRENT_OC_DISCHARGE_MA) {
                    cur = prev_current;
                }
            }
            meas.current_ma = cur;
            prev_current = cur;
        }

        prev_valid = 1;  /* 首次完成后启用过滤 */

        /* 温度处理：原始为 float 摄氏度，这里转换为 0.1℃ 的 int16_t
         * 超出合理范围 [-50, +120] 的温度记为 INT16_MIN，表示无效。
         * 这可以过滤掉因NTC未连接而读取到的极限低温（例如 -76.9 °C）。
         */
        for (uint8_t i = 0; i < BMS_TEMP_SENSOR_COUNT; i++) {
            float t = raw.temperature[i];
            if (t > -50.0f && t < 120.0f) {
                meas.temperature_dC[i] = (int16_t)(t * 10.0f); /* 转换为 0.1℃ */
            } else {
                meas.temperature_dC[i] = INT16_MIN;            /* 标记无效温度 */
            }
        }

        /* 提交到共享状态，更新时间戳与 tick 计数 */
        if (BMS_LockState(pdMS_TO_TICKS(20)) == pdTRUE) {
            g_bms_state.measurement      = meas;
            g_bms_state.measurement_tick = xTaskGetTickCount();
            g_bms_state.timestamp_ms     = (uint32_t)g_bms_state.measurement_tick *
                                           1000UL / configTICK_RATE_HZ;
            BMS_UnlockState();
        }

        /* 通知 SOC/Fault/Balance/Control 等任务：有新的测量数据可用 */
        xEventGroupSetBits(s_events, BMS_EVT_MEASUREMENT);
    }
}

/*--------------------------------------------------------------------------
 * 任务：BMS_Task_Fault（故障检测任务）
 *
 * 设计要点：
 *   - 等待测量事件 BMS_EVT_MEASUREMENT，意味着有新一轮电压/电流/温度数据；
 *   - 读取 BQ76940 的状态寄存器（一次性硬件故障位），并清除；
 *   - 再结合软件设定阈值对单体/整包电压、电流、温度做二次判定；
 *   - 将结果写入 g_bms_state.faults 和 fault_mask，并维护 fault_latched 与 recovery_counter；
 *   - 广播 BMS_EVT_FAULTS_UPDATED 通知控制任务更新运行状态。
 *
 * 故障锁存逻辑：
 *   - 一旦检测到任何故障 mask != 0，则立即将 fault_latched 置 1，并清 recovery_counter；
 *   - 当连续多个周期（BMS_RECOVERY_CYCLES_REQUIRED）都没有故障时，才解除锁存。
 *--------------------------------------------------------------------------*/
void BMS_Task_Fault(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        /* 等待 MEASUREMENT 事件，超时时间 100ms；
         * 若超时未收到，则跳过本轮循环（不更新故障）。
         */
        EventBits_t bits = xEventGroupWaitBits(s_events,
                                               BMS_EVT_MEAS_FAULT,
                                               pdTRUE,   /* 读取后清除该 bit */
                                               pdFALSE,  /* 任意一个 bit 即可返回 */
                                               pdMS_TO_TICKS(100));

        if ((bits & BMS_EVT_MEAS_FAULT) == 0U) {
            continue; /* 超时：本轮没有新的测量数据，跳过 */
        }


        BMS_FaultFlags_t faults = {0};      // 细分故障标志结构体
        BMS_MeasurementSnapshot_t meas;
        BMS_Thresholds_t thresholds;
        memset(&meas, 0, sizeof(meas));

        /* 复制最新测量快照和阈值快照（缩短持锁时间），后续在无锁状态下使用 */
        if (BMS_LockState(pdMS_TO_TICKS(20)) == pdTRUE) {
            meas = g_bms_state.measurement;
            thresholds = g_thresholds;
            BMS_UnlockState();
        } else {
            continue;
        }

        /* 读取并清除 BQ76940 粘滞状态位。
         * 先把硬件 COV/CUV/OCD/SCD 映射到软件故障，再清 SYS_STAT。
         * 只清 SYS_STAT，不写 SYS_CTRL2，避免重启 CC 转换周期。
         */
        uint8_t hw_status = 0U;
        if (BMS_LockBQ(pdMS_TO_TICKS(20)) == pdTRUE) {
            hw_status = BQ76940_Read_Status();
            BQ76940_Clear_Status();
            BMS_UnlockBQ();
        }

        if ((hw_status & SYS_STAT_SCD) != 0U)          { faults.scd = 1U; }
        if ((hw_status & SYS_STAT_OCD) != 0U)          { faults.ocd = 1U; }
        if ((hw_status & SYS_STAT_COV) != 0U)          { faults.cell_ov = 1U; }
        if ((hw_status & SYS_STAT_CUV) != 0U)          { faults.cell_uv = 1U; }
        if ((hw_status & SYS_STAT_DEVICE_XREADY) != 0U){ faults.bq_xready = 1U; }
        if ((hw_status & SYS_STAT_OVRD_ALERT) != 0U)   { faults.bq_ovrd_alert = 1U; }

        /* 2. 软件阈值二次判定：单体/整包电压、电流、温度 */
        int16_t min_cell = INT16_MAX;
        int16_t max_cell = INT16_MIN;

        /* 遍历各 cell 电压 */
        for (uint8_t i = 0; i < BMS_MAX_CELLS; i++) {
            int16_t mv = meas.cell_mv[i];
            if (mv > 0) { /* >0 视为有效值 */
                if (mv < min_cell) { min_cell = mv; }
                if (mv > max_cell) { max_cell = mv; }

                /* 单体软过压/欠压判定 */
                if (mv >= (int16_t)thresholds.cell_ov_mv)            { faults.cell_ov = 1U; }
                if (mv > 0 && mv <= (int16_t)thresholds.cell_uv_mv) { faults.cell_uv = 1U; }
            }
        }

        /* 整包软过压/欠压判定（pack_mv>0 视为有效）
         * 使用实际检测到的电芯数 meas.cell_count 而非 BMS_MAX_CELLS，
         * 避免电芯数少于 15 串时误触发整包欠压/过压保护。
         */
        uint8_t n_cells = (meas.cell_count > 0U) ? meas.cell_count : 1U;
        if (meas.pack_mv >= (uint32_t)thresholds.cell_ov_mv * n_cells) {
            faults.pack_ov = 1U;
        }
        if (meas.pack_mv > 0 &&
            meas.pack_mv <= (uint32_t)thresholds.cell_uv_mv * n_cells) {
            faults.pack_uv = 1U;
        }

        /* 电流软过流/短路判定：约定放电为负电流 */
        if (meas.current_ma < 0) {
            int32_t discharge_ma = -meas.current_ma; // 取绝对值方便比较
            if (discharge_ma >= BMS_CURRENT_SHORT_CIRCUIT_MA) {
                faults.scd = 1U;
            } else if (discharge_ma >= (int32_t)thresholds.ocd_ma) {
                faults.ocd = 1U;
            }
        }

        /* 温度判定：扫描所有有效温度，找最大/最小值 */
        int16_t temp_max = INT16_MIN;
        int16_t temp_min = INT16_MAX;
        for (uint8_t i = 0; i < BMS_TEMP_SENSOR_COUNT; i++) {
            int16_t t = meas.temperature_dC[i];
            if (t == INT16_MIN) { continue; } /* 跳过无效温度 */
            if (t > temp_max) { temp_max = t; }
            if (t < temp_min) { temp_min = t; }
        }

        /* 最大温度超限：充电过温/放电过温 */
        if (temp_max != INT16_MIN) {
            if (temp_max >= thresholds.chg_ot_dC) { faults.charge_ot    = 1U; }
            if (temp_max >= thresholds.dsg_ot_dC) { faults.discharge_ot = 1U; }
        }

        /* 最小温度过低：充电/放电低温 */
        if (temp_min != INT16_MAX) {
            if (temp_min <= thresholds.chg_ut_dC) { faults.charge_ut    = 1U; }
            if (temp_min <= thresholds.dsg_ut_dC) { faults.discharge_ut = 1U; }
        }

        /* 3. 将细分故障汇总为 fault_mask，并维护锁存/恢复计数 */
        uint16_t mask = BMS_BuildFaultMask(&faults);

        if (BMS_LockState(pdMS_TO_TICKS(20)) == pdTRUE) {
            g_bms_state.faults      = faults;
            g_bms_state.fault_mask  = mask;

            if (mask != 0U) {
                uint8_t severe_fault = (faults.scd || faults.ocd || faults.bq_xready) ? 1U : 0U;
                g_bms_state.recovery_counter = 0U;
                if (severe_fault) {
                    g_bms_state.fault_confirm_count = 2U;
                    g_bms_state.fault_latched = 1U;
                } else {
                    /* 普通故障防抖：需要连续 2 次检测到故障才锁存 */
                    if (g_bms_state.fault_confirm_count < 2U) {
                        g_bms_state.fault_confirm_count++;
                    }
                    if (g_bms_state.fault_confirm_count >= 2U) {
                        g_bms_state.fault_latched = 1U;
                    }
                }
            } else {
                /* 当前无故障：清零确认计数，累加"无故障周期计数" */
                g_bms_state.fault_confirm_count = 0U;
                if (g_bms_state.recovery_counter < BMS_RECOVERY_CYCLES_REQUIRED) {
                    g_bms_state.recovery_counter++;
                }
                /* 当连续无故障周期达到设定次数，解除故障锁存 */
                if (g_bms_state.recovery_counter >= BMS_RECOVERY_CYCLES_REQUIRED) {
                    g_bms_state.fault_latched = 0U;
                }
            }
            BMS_UnlockState();
        }

        /* 广播：故障状态已更新，通知 Control 等任务重新评估 */
        xEventGroupSetBits(s_events, BMS_EVT_FAULTS_UPDATED);
    }
}

/*─────────────────────────────────────────────────────────────────────────────
 * 任务：BMS_Task_SOC（库仑计积分与容量学习）
 *
 * 触发：等待 BMS_EVT_MEASUREMENT（每次测量有新电流值时触发）
 *
 * 职责：
 *   - 利用电流与时间步长 dt_hours 计算 ΔQ(mAh)，对库仑计 coulomb_mAh 做积分；
 *   - 记录 throughput（总吞吐电量）与 discharge_accum（累计放电量）；
 *   - 实现一个简单的容量学习机制：
 *       从低 SOC（≤ BMS_SOC_LEARN_LOW_PERCENT，例如 20%）开始记录充入量，
 *       一直到高 SOC（≥ BMS_SOC_LEARN_HIGH_PERCENT，例如 95%）结束，
 *       用本段 SOC 跨度换算满容量，再以 10% 权重融合更新学习容量；
 *   - 根据累计放电量与容量估算循环次数（每放掉一倍容量记作一个 cycle）；
 *   - 计算 SOH = 学得容量 / 标称容量 × 100%，并限制在 [10%, 100%]；
 *   - 将 SOC/SOH/剩余容量转换为 uint8/uint16 供上位机和 Control 任务使用。
 *
 * 注意：
 *   - dt_hours 使用 configTICK_RATE_HZ 从 tick 换算为小时；
 *   - 通过 clampf_range / clamp_u8 / clamp_u16 避免数值越界或不合理值。
 *─────────────────────────────────────────────────────────────────────────────
 */
void BMS_Task_SOC(void *pvParameters)
{
    (void)pvParameters;

    TickType_t last_tick = xTaskGetTickCount();
    uint32_t idle_ms = 0;        /* 连续"小电流"静置时间，用于 OCV 校准 */

    for (;;) {
        /* 等待测量事件（有新电流值），此处为阻塞等待 */
        xEventGroupWaitBits(s_events,
                            BMS_EVT_MEAS_SOC,
                            pdTRUE,   /* 读取后清除事件位 */
                            pdFALSE,
                            portMAX_DELAY);

        /* 拷贝与 SOC 计算相关的状态，缩短持锁时间 */
        BMS_MeasurementSnapshot_t meas;
        TickType_t meas_tick;
        float coulomb;
        float capacity;
        float discharge_accum;
        float throughput;
        float learning_acc;
        float learning_start_soc;
        uint8_t learning_phase;
        uint32_t cycle_count;

        if (BMS_LockState(pdMS_TO_TICKS(20)) != pdTRUE) {
            continue;
        }

        meas            = g_bms_state.measurement;
        meas_tick       = g_bms_state.measurement_tick;
        coulomb         = g_bms_state.coulomb_mAh;
        capacity        = g_bms_state.learned_capacity_mAh;
        discharge_accum = g_bms_state.discharge_accum_mAh;
        throughput      = g_bms_state.throughput_mAh;
        learning_acc    = g_bms_state.learning_accum_mAh;
        learning_start_soc = g_bms_state.learning_start_soc_pct;
        learning_phase  = g_bms_state.learning_phase;
        cycle_count     = g_bms_state.cycle_count;

        BMS_UnlockState();

        /* 1. 计算时间步长 dt_hours */
        TickType_t delta_ticks = meas_tick - last_tick;
        last_tick = meas_tick;
        uint32_t dt_ms = (uint32_t)(((uint32_t)delta_ticks * 1000UL) / configTICK_RATE_HZ);
        float dt_hours = (float)delta_ticks / (3600.0f * (float)configTICK_RATE_HZ);

        if (dt_hours <= 0.0f) {
            dt_hours = (float)BMS_MEASUREMENT_PERIOD_MS / 3600000.0f;
            dt_ms = BMS_MEASUREMENT_PERIOD_MS;
        }
        if (dt_ms == 0U) {
            dt_ms = BMS_MEASUREMENT_PERIOD_MS;
        }

        /* 2. 计算本周期电量变化 */
        float current_ma = (float)meas.current_ma;
        if (fabsf(current_ma) < BMS_SOC_CURRENT_DEADBAND_MA) {
            current_ma = 0.0f;
        }
        float delta_mAh  = current_ma * dt_hours;
        if (delta_mAh > 0.0f) {
            delta_mAh *= BMS_SOC_CHARGE_EFFICIENCY;
        }
        coulomb += delta_mAh;

        /* 2.5 OCV 静置校准：
         * 当 |I| 持续低于静置阈值后，认为电池处于近似静置状态，
         * 此时电压接近 OCV，用电压查表结果融合修正库仑计，
         * 纠正纯库仑积分的长期累积误差。
         */
        if (fabsf(current_ma) < BMS_SOC_IDLE_CURRENT_MA) {
            if (idle_ms < BMS_SOC_OCV_IDLE_TIME_MS) {
                idle_ms += dt_ms;
                if (idle_ms > BMS_SOC_OCV_IDLE_TIME_MS) {
                    idle_ms = BMS_SOC_OCV_IDLE_TIME_MS;
                }
            }
        } else {
            idle_ms = 0U;
        }

        if (idle_ms >= BMS_SOC_OCV_IDLE_TIME_MS) {
            uint32_t cell_sum_mv = 0U;
            uint8_t valid_cells = 0U;

            for (uint8_t i = 0; i < BMS_MAX_CELLS; i++) {
                if (meas.cell_mv[i] > 0) {
                    cell_sum_mv += (uint16_t)meas.cell_mv[i];
                    valid_cells++;
                }
            }

            if (valid_cells > 0U && cell_sum_mv <= 0xFFFFU) {
                uint8_t ocv_soc = BQ76940_Calculate_SOC((uint16_t)cell_sum_mv, valid_cells);
                float ocv_coulomb = capacity * (ocv_soc / 100.0f);
                coulomb = (1.0f - BMS_SOC_OCV_BLEND) * coulomb +
                          BMS_SOC_OCV_BLEND * ocv_coulomb;
            }
            idle_ms = 0U;  /* 校准完成后重新计时 */
        }

        /* 3. 库仑计与容量边界限制 */
        if (capacity < BMS_SOC_MIN_CAPACITY_MAH) {
            capacity = BMS_SOC_MIN_CAPACITY_MAH;
        }
        coulomb = clampf_range(coulomb, 0.0f, capacity);

        /* 4. 吞吐量与累计放电量（只在放电电流较大时统计） */
        throughput += fabsf(delta_mAh);
        if (current_ma < -50.0f) { // 放电电流小于 -50mA 视为有效放电
            discharge_accum += (-current_ma) * dt_hours;
        }

        /* 5. 计算 SOC 百分比：SOC = (coulomb / capacity) × 100% */
        float soc = (capacity > 0.0f) ? (coulomb / capacity * 100.0f) : 0.0f;
        soc = clampf_range(soc, 0.0f, 100.0f);

        /* 6. 容量学习逻辑：
         *    - 当 SOC 低于设定下限（例如 20%）且当前不在学习阶段，则开始学习（phase=1）；
         *    - 在学习阶段，记录充电电流产生的正 ΔQ；
         *    - 当 SOC 高于设定上限（例如 95%）时，按本轮 SOC 跨度把充入量折算成满容量，
         *      再以 BLEND 系数（例如 10%）更新学习容量；
         *    - 学习结束后清空 learning_acc，恢复 learning_phase=0。
         */
        if (soc <= (float)BMS_SOC_LEARN_LOW_PERCENT && learning_phase == 0U) {
            learning_phase = 1U;
            learning_acc   = 0.0f;  // 从低 SOC 开始，清空本轮学习累积
            learning_start_soc = soc;
        }

        if (learning_phase == 1U) {
            /* 仅在充电电流为正时累加学习量 */
            if (current_ma > 0.0f) {
                learning_acc += (delta_mAh > 0.0f) ? delta_mAh : 0.0f;
            }
            /* 当 SOC 达到高 SOC 阈值（例如 ≥95%）时结束本轮学习 */
            if (soc >= (float)BMS_SOC_LEARN_HIGH_PERCENT) {
                float learn_span_pct = soc - learning_start_soc;
                if (learn_span_pct < 1.0f) {
                    learn_span_pct = (float)(BMS_SOC_LEARN_HIGH_PERCENT -
                                             BMS_SOC_LEARN_LOW_PERCENT);
                }
                if (learning_acc > (0.5f * capacity * learn_span_pct / 100.0f)) {
                    float measured_capacity = learning_acc * 100.0f / learn_span_pct;
                    /* 足够大的一次充电周期，用 BLEND（10%）比例更新容量：
                     * capacity_new = 90% * old + 10% * measured
                     * 这样可以避免一次学习导致容量剧烈跳变。
                     */
                    capacity = (1.0f - BMS_SOC_LEARN_BLEND) * capacity +
                               BMS_SOC_LEARN_BLEND * measured_capacity;
                }
                learning_phase = 0U;   // 学习结束
                learning_acc   = 0.0f; // 清空累积量
                learning_start_soc = 0.0f;
            }
        }

        /* 7. 再次对容量进行合理夹域（避免学习结果过小或过大） */
        if (capacity < BMS_SOC_MIN_CAPACITY_MAH) {
            capacity = BMS_SOC_MIN_CAPACITY_MAH;
        }
        if (capacity > (BMS_PACK_NOMINAL_CAPACITY_MAH * 1.5f)) {
            capacity = BMS_PACK_NOMINAL_CAPACITY_MAH * 1.5f;
        }
        coulomb = clampf_range(coulomb, 0.0f, capacity);
        soc = (capacity > 0.0f) ? (coulomb / capacity * 100.0f) : 0.0f;
        soc = clampf_range(soc, 0.0f, 100.0f);

        /* 8. 循环次数估算：
         *    当累计放电量达到“容量”时，认为完成了一次完整循环（近似）。
         */
        if (capacity > 0.0f && discharge_accum >= capacity) {
            cycle_count++;
            discharge_accum -= capacity; /* 扣掉已计入的一轮容量，保留剩余部分 */
        }

        /* 9. SOH 估算：SOH = 学习容量 / 标称容量 × 100%，限制在 10%~100% */
        float soh = (capacity / BMS_PACK_NOMINAL_CAPACITY_MAH) * 100.0f;
        soh = clampf_range(soh, 10.0f, 100.0f);

        /* 10. 将 SOC/SOH/容量转换为整数形式：
         *     - 剩余容量、学习容量转换为 10mAh 单位的 uint16；
         *     - SOC/SOH 百分比转换为 uint8。
         */
        uint16_t remaining_10mAh = clamp_u16(coulomb  / 10.0f);
        uint16_t capacity_10mAh  = clamp_u16(capacity / 10.0f);
        uint8_t  soc_u8          = clamp_u8(soc);
        uint8_t  soh_u8          = clamp_u8(soh);

        /* 11. 写回全局状态（带互斥保护） */
        if (BMS_LockState(pdMS_TO_TICKS(20)) == pdTRUE) {
            g_bms_state.coulomb_mAh              = coulomb;
            g_bms_state.learned_capacity_mAh     = capacity;
            g_bms_state.discharge_accum_mAh      = discharge_accum;
            g_bms_state.throughput_mAh           = throughput;
            g_bms_state.learning_accum_mAh       = learning_acc;
            g_bms_state.learning_start_soc_pct   = learning_start_soc;
            g_bms_state.learning_phase           = learning_phase;
            g_bms_state.cycle_count              = cycle_count;
            g_bms_state.soc_pct                  = soc_u8;
            g_bms_state.soh_pct                  = soh_u8;
            g_bms_state.remaining_capacity_10mAh = remaining_10mAh;
            g_bms_state.learned_capacity_10mAh   = capacity_10mAh;
            BMS_UnlockState();
        }

        /* 12. 广播“本轮 SOC 已就绪”，通知 Control 任务可以基于最新 SOC 做决策 */
        xEventGroupSetBits(s_events, BMS_EVT_SOC_READY);
    }
}

/*─────────────────────────────────────────────────────────────────────────────
 * 任务：BMS_Task_Balance（均衡任务）
 *
 * 触发：每次 BMS_EVT_MEASUREMENT（有新电芯电压数据）
 *
 * 启动条件（全部满足才允许均衡）：
 *   - 当前无任何故障（fault_mask == 0）；
 *   - 充电 FET 已打开（charge_enabled == 1，表示处于充电路径允许状态）；
 *   - 至少有一个有效电芯（meas.cell_count > 0）。
 *
 * 均衡策略：
 *   - 统计所有有效电芯电压的最小值 min_mv 和最大值 max_mv；
 *   - 压差 diff = max_mv - min_mv；
 *   - 当 diff ≥ BMS_BALANCE_ENABLE_DIFF_MV（如 30mV）时：
 *       对所有满足 "cell_mv >= min_mv + 阈值" 的电芯开启均衡；
 *   - 当 diff ≤ BMS_BALANCE_DISABLE_DIFF_MV（如 15mV）时：
 *       关闭全部均衡；
 *   - 其它处于中间区间时：
 *       保持当前均衡掩码不变，形成滞回，避免频繁开关。
 *
 * 实施细节：
 *   - 使用 active_mask 记录当前已开启均衡的电芯掩码；
 *   - 仅当 new_mask != active_mask 时才通过 I2C 向 BQ 下发具体开关命令，降低总线负担；
 *   - 在不满足均衡条件的场景（有故障、未充电、无有效电芯）下，确保均衡全部关闭。
 *─────────────────────────────────────────────────────────────────────────────
 */
void BMS_Task_Balance(void *pvParameters)
{
    (void)pvParameters;
    uint16_t active_mask = 0U; /* 当前已开启均衡的电芯掩码（位图） */

    for (;;) {
        /* 等待新测量事件，这是均衡计算的触发点 */
        xEventGroupWaitBits(s_events,
                            BMS_EVT_MEAS_BALANCE,
                            pdTRUE,
                            pdFALSE,
                            portMAX_DELAY);

        BMS_MeasurementSnapshot_t meas;
        uint16_t fault_mask;
        uint8_t fault_latched;
        uint8_t charge_enabled;

        /* 读取当前测量数据、故障状态和充电状态 */
        if (BMS_LockState(pdMS_TO_TICKS(20)) != pdTRUE) {
            continue;
        }
        meas           = g_bms_state.measurement;
        fault_mask     = g_bms_state.fault_mask;
        fault_latched  = g_bms_state.fault_latched;
        charge_enabled = g_bms_state.charge_enabled;
        BMS_UnlockState();

        /* 不满足均衡前置条件：确保均衡全关，并跳过本轮 */
        if (fault_mask != 0U ||
            fault_latched != 0U ||
            charge_enabled == 0U ||
            meas.cell_count == 0U) {
            if (active_mask != 0U && BMS_LockBQ(pdMS_TO_TICKS(20)) == pdTRUE) {
                BQ76940_Disable_All_Balance();  // 关闭所有均衡开关
                BMS_UnlockBQ();
                active_mask = 0U;               // 本地记录也清零
            }
            continue;
        }

        /* 统计当前采样中各电芯的最小/最大电压 */
        int16_t min_mv = INT16_MAX;
        int16_t max_mv = INT16_MIN;
        for (uint8_t i = 0; i < BMS_MAX_CELLS; i++) {
            int16_t mv = meas.cell_mv[i];
            if (mv > 0) {  // >0 视为有效电压
                if (mv < min_mv) { min_mv = mv; }
                if (mv > max_mv) { max_mv = mv; }
            }
        }

        if (min_mv == INT16_MAX || max_mv == INT16_MIN) {
            /* 没有任何有效电压读数，跳过 */
            continue;
        }

        int16_t diff = max_mv - min_mv; // 计算最大值和最小值的差值（压差）
        uint16_t new_mask = 0U;         // 目标均衡掩码

        /* 带滞回的目标掩码计算 */
        if (diff >= BMS_BALANCE_ENABLE_DIFF_MV) { // 若压差≥开启阈值，打开高电压电芯均衡
            for (uint8_t i = 0; i < BMS_MAX_CELLS; i++) {
                int16_t mv = meas.cell_mv[i];
                if (mv > 0 && mv >= (min_mv + BMS_BALANCE_ENABLE_DIFF_MV)) {
                    new_mask |= (1U << i);   // 对电压较高的电芯打开均衡
                }
            }
        } else if (diff <= BMS_BALANCE_DISABLE_DIFF_MV) { // 差距≤关闭阈值，全部关闭
            new_mask = 0U;
        } else { // 处于滞回区间，例如 15mV~30mV，维持原有均衡状态，避免抖动
            new_mask = active_mask;
        }

        /* 掩码变化 → 通过 I2C 实际下发命令到 BQ76940 */
        if (new_mask != active_mask) {
            if (BMS_LockBQ(pdMS_TO_TICKS(20)) == pdTRUE) {
                BQ76940_Disable_All_Balance(); /* 先全关，以避免旧状态影响 */
                for (uint8_t i = 0; i < BMS_MAX_CELLS; i++) {
                    if (new_mask & (1U << i)) {
                        /* BQ 通道编号从 1 开始，因此 i 需要 +1 */
                        BQ76940_Enable_Balance((uint8_t)(i + 1U), 1U);
                    }
                }
                BMS_UnlockBQ();
                active_mask = new_mask; /* 本地保存当前实际掩码 */
            }
        }
    }
}

/*─────────────────────────────────────────────────────────────────────────────
 * 任务：BMS_Task_Control（控制任务）
 *
 * 触发事件：
 *   - BMS_EVT_MEASUREMENT    ：有新测量数据；
 *   - BMS_EVT_HOST_CMD       ：主机对充/放电请求做了修改；
 *   - BMS_EVT_FAULTS_UPDATED ：故障状态已更新；
 *   - BMS_EVT_SOC_READY      ：SOC/SOH 计算完成；
 *   - 另外设置了 200ms 超时，即使没有新事件也会周期性醒来检查。
 *
 * 职责：
 *   - 综合判断系统是否允许充电/放电：
 *       主机允许 + 无故障锁存 + 无相关故障 + SOC 不到极限；
 *   - 如期望状态与当前 FET 状态不一致，则通过 BQ API 执行，
 *       并以 BQ 返回的实际 FET 状态为准；
 *   - 根据 FET 状态与电流幅度，更新 run_state（FAULT > CHARGING > DISCHARGING > IDLE）。
 *
 * 说明：
 *   - 这是整个 BMS 的“调度中枢”和“状态机核心”，
 *     将来自测量、故障、SOC、主机命令的信息统一起来，做出 FET 控制决策。
 *─────────────────────────────────────────────────────────────────────────────
 */
void BMS_Task_Control(void *pvParameters)
{
    (void)pvParameters;
    static uint8_t chg_inhibit = 0U;  /* OV 触发后抑制充电，直到电压降到 release 阈值 */

    for (;;) {
        /* 等待任意一个相关事件，或 200ms 超时
         * 注意：使用 OR 逻辑（pdFALSE，非“全都满足”），只要任意事件到来就会被唤醒。
         */
        xEventGroupWaitBits(s_events,
                            BMS_EVT_MEAS_CONTROL |
                            BMS_EVT_HOST_CMD |
                            BMS_EVT_FAULTS_UPDATED |
                            BMS_EVT_SOC_READY,
                            pdTRUE,            /* 读取后清除这些事件位 */
                            pdFALSE,           /* 任一事件即可返回 */
                            pdMS_TO_TICKS(200));

        BMS_MeasurementSnapshot_t meas;
        BMS_FaultFlags_t faults;
        uint16_t fault_mask;
        uint8_t fault_latched;
        uint8_t host_charge;
        uint8_t host_discharge;
        uint8_t soc;
        uint8_t charge_enabled;
        uint8_t discharge_enabled;

        /* 拷贝所需状态（缩短持锁时间） */
        if (BMS_LockState(pdMS_TO_TICKS(20)) != pdTRUE) {
            continue;
        }

        meas              = g_bms_state.measurement;
        faults            = g_bms_state.faults;
        fault_mask        = g_bms_state.fault_mask;
        fault_latched     = g_bms_state.fault_latched;
        host_charge       = g_bms_state.charge_request;
        host_discharge    = g_bms_state.discharge_request;
        soc               = g_bms_state.soc_pct;
        charge_enabled    = g_bms_state.charge_enabled;
        discharge_enabled = g_bms_state.discharge_enabled;

        BMS_UnlockState();

        /* ── 充电过压滢回逻辑 ──
         * 一旦 cell_ov 触发，设置 chg_inhibit=1 抑制充电；
         * 直到所有电芯电压降到 BMS_CHG_RESUME_CELL_MV 以下才恢复。
         * 这避免了电压在 4200mV 边界波动时反复开关 CHG FET。
         */
        {
            int16_t max_cell_mv = INT16_MIN;
            uint8_t i;
            for (i = 0; i < BMS_MAX_CELLS; i++) {
                if (meas.cell_mv[i] > 0 && meas.cell_mv[i] > max_cell_mv)
                    max_cell_mv = meas.cell_mv[i];
            }
            if (faults.cell_ov) {
                chg_inhibit = 1U;
            } else if (chg_inhibit && max_cell_mv < (int16_t)BMS_CHG_RESUME_CELL_MV) {
                chg_inhibit = 0U;
            }
        }

        uint16_t charge_block_mask = BMS_FAULT_CELL_OV |
                                     BMS_FAULT_PACK_OV |
                                     BMS_FAULT_OCD |
                                     BMS_FAULT_SCD |
                                     BMS_FAULT_CHG_OT |
                                     BMS_FAULT_CHG_UT |
                                     BMS_FAULT_BQ_XREADY |
                                     BMS_FAULT_BQ_OVRD_ALERT;
        uint16_t discharge_block_mask = BMS_FAULT_CELL_UV |
                                        BMS_FAULT_PACK_UV |
                                        BMS_FAULT_OCD |
                                        BMS_FAULT_SCD |
                                        BMS_FAULT_DSG_OT |
                                        BMS_FAULT_DSG_UT |
                                        BMS_FAULT_BQ_XREADY |
                                        BMS_FAULT_BQ_OVRD_ALERT;

        /* 1. 判定是否“可以充电”：
         *      - 主机允许 + 无故障锁存 + 无过压抑制 + 无温度越界 + SOC<100%
         */
        uint8_t can_charge = host_charge &&
                             !fault_latched &&
                             !chg_inhibit &&
                             ((fault_mask & charge_block_mask) == 0U) &&
                             soc < 100U;

        /* 2. 判定是否“可以放电”：
         *      - 主机允许 host_discharge != 0；
         *      - 没有处于锁存故障状态；
         *      - 无单体/整包欠压故障；
         *      - 无放电温度过高/过低故障；
         *      - SOC > 0%（避免过放）。
         */
        uint8_t can_discharge = host_discharge &&
                                !fault_latched &&
                                ((fault_mask & discharge_block_mask) == 0U) &&
                                soc > 0U;

        uint8_t new_charge_state    = charge_enabled;
        uint8_t new_discharge_state = discharge_enabled;

        /* 3 & 4. 获取硬件实际状态，并与期望状态进行对比。
         * 如果硬件因瞬间保护（如插拔浪涌触发的 SCD/OCD）自发关闭了 FET，
         * 而软件期望打开，则在此处重新下发打开指令进行“对齐”。
         */
        if (BMS_LockBQ(pdMS_TO_TICKS(20)) == pdTRUE) {
            uint8_t hw_chg = 0, hw_dsg = 0;
            BQ76940_Get_CHG_DSG_Status(&hw_chg, &hw_dsg);

            // 检查充电 FET
            if (can_charge != hw_chg) {
                uint8_t fet = 0;
                BQ76940_Enable_CHG(can_charge, &fet);
                new_charge_state = fet;
            } else {
                new_charge_state = hw_chg;
            }

            // 检查放电 FET
            if (can_discharge != hw_dsg) {
                uint8_t fet = 0;
                BQ76940_Enable_DSG(can_discharge, &fet);
                new_discharge_state = fet;
            } else {
                new_discharge_state = hw_dsg;
            }
            BMS_UnlockBQ();
        }

        /* 5. 更新运行状态机：
         *    优先级：Fault > Charging > Discharging > Idle
         *    - 若有任何故障或处于锁存状态，统归为 BMS_STATE_FAULT；
         *    - 否则若充电 FET 打开，且电流大于设定充电阈值 → CHARGING；
         *    - 否则若放电 FET 打开，且电流小于设定放电阈值（负方向） → DISCHARGING；
         *    - 否则认为系统处于 Idle（空闲/待机）状态。
         */
        BMS_RunState_t new_state = BMS_STATE_IDLE;
        if (fault_latched || fault_mask != 0U) {
            new_state = BMS_STATE_FAULT;
        } else if (new_charge_state && meas.current_ma > BMS_CURRENT_ACTIVE_CHG_MA) {
            new_state = BMS_STATE_CHARGING;
        } else if (new_discharge_state && meas.current_ma < -BMS_CURRENT_ACTIVE_DSG_MA) {
            new_state = BMS_STATE_DISCHARGING;
        } else {
            new_state = BMS_STATE_IDLE;
        }

        /* 6. 将最终 FET 状态和运行状态写回全局状态 */
        if (BMS_LockState(pdMS_TO_TICKS(20)) == pdTRUE) {
            g_bms_state.charge_enabled    = new_charge_state;
            g_bms_state.discharge_enabled = new_discharge_state;
            g_bms_state.run_state         = new_state;
            BMS_UnlockState();
        }
    }
}
