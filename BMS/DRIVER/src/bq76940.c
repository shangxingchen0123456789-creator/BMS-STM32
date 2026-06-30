#include "stm32f10x.h"
#include "bq76940.h"
#include "iic.h"
#include "delay.h"
#include <math.h>
#include <stddef.h>
#include "usart.h"
#include <stdio.h>
#include <limits.h>

#define BQ76940_CC_LSB_UV      8.44f
#define BQ76940_RSENSE_MOHM    4.0f


static int   ADC_offset = 0;         // ADCOFFSET（mV 单位补偿）
static int   GAIN_uV    = 0;         // LSB 电压增益（单位: µV/LSB）
static float ADC_GAIN_raw = 0;        // 从寄存器拼出的增益修正值（0..7）

// 用户可设置的“哪些cell接入”的掩码，bit0->Cell1 ... bit14->Cell15
static uint16_t s_cell_present_mask = 0x4E73; // 9串：Cell 1,2,5,6,7,10,11,12,15

// 初始化寄存器配置
static const uint8_t BQ76940_Init_Reg[12] = {
    SYS_STAT, CELLBAL1, CELLBAL2, CELLBAL3,
    SYS_CTRL1, SYS_CTRL2, PROTECT1, PROTECT2,
    PROTECT3, OV_TRIP,   UV_TRIP,  CC_CFG
};

static const uint8_t BQ76940_Init_Data[12] = {
    0x7F, 0x00, 0x00, 0x00,   // 清除故障位(不清CC_READY), 关闭均衡
    0x18, 0x43, 0x00, 0x00,   // SYS_CTRL1=0x18(ADC+温度), SYS_CTRL2=0x43(CHG+DSG+CC_EN)
    0x00, 0x00, 0x00, 0x19    // 保护 & 库仑计配置（占位，实际保护值后面单独写）
};

/*=============================
 * CRC-8 校验（BQ76940 I2C 协议必需）
 * 多项式: x^8 + x^2 + x + 1 = 0x07
 *=============================*/
static uint8_t BQ76940_CRC8(uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

/*=============================
 * 私有I2C 基础读写（带 CRC8）
 *=============================*/
static inline uint8_t BQ76940_Read_Reg(uint8_t reg_addr)
{
    uint8_t data;

    IIC_Start();
    IIC_Write_Byte((BQ76940_ADDR << 1) | 0); // 写地址
    if (IIC_Wait_Ack()) { IIC_Stop(); return 0U; }
    IIC_Write_Byte(reg_addr);
    if (IIC_Wait_Ack()) { IIC_Stop(); return 0U; }

    IIC_Start();
    IIC_Write_Byte((BQ76940_ADDR << 1) | 1); // 读地址
    if (IIC_Wait_Ack()) { IIC_Stop(); return 0U; }
    data = IIC_Read_Byte(1);     // 读数据字节，ACK（还有 CRC 要读）
    IIC_Read_Byte(0);            // 读 CRC 字节，NACK（可选校验）
    IIC_Stop();

    return data;
}

static inline void BQ76940_Write_Reg(uint8_t reg_addr, uint8_t data)
{
    /* 计算 CRC8：覆盖 [从机写地址, 寄存器地址, 数据] */
    uint8_t buf[3];
    buf[0] = (BQ76940_ADDR << 1) | 0;  // 从机写地址
    buf[1] = reg_addr;
    buf[2] = data;
    uint8_t crc = BQ76940_CRC8(buf, 3);

    IIC_Start();
    IIC_Write_Byte(buf[0]);       // 从机写地址
    if (IIC_Wait_Ack()) { IIC_Stop(); return; }

    IIC_Write_Byte(reg_addr);     // 寄存器地址
    if (IIC_Wait_Ack()) { IIC_Stop(); return; }

    IIC_Write_Byte(data);         // 数据
    if (IIC_Wait_Ack()) { IIC_Stop(); return; }

    IIC_Write_Byte(crc);          // CRC8
    if (IIC_Wait_Ack()) { IIC_Stop(); return; }

    IIC_Stop();
    Delay_ms(2);
}

/*=============================
 * 设备电源管理
 *=============================*/
void WAKE_ALL_DEVICE(void)
{
    
    MCU_WAKE_BQ_ONOFF(1);
    Delay_ms(100);
    MCU_WAKE_BQ_ONOFF(0);
}

void SHIP_ALL_DEVICE(void)
{
    
    BQ76940_Write_Reg(SYS_STAT, 0xFF); // 清状态
    BQ76940_Write_Reg(SYS_CTRL1, 0x19);
    Delay_ms(20);
    BQ76940_Write_Reg(SYS_CTRL1, 0x1A);
}

/*=============================
 * 基础配置
 *=============================*/
void BQ76940_Set_Cell_Present_Mask(uint16_t mask /*bit0->cell1*/)
{
    s_cell_present_mask = (mask & 0x7FFF);
}

static void BQ_First_Config(void)
{
    for (uint8_t i = 0; i < 12; i++) {
        BQ76940_Write_Reg(BQ76940_Init_Reg[i], BQ76940_Init_Data[i]);
        Delay_ms(2);
    }
}

static void Get_offset_gain(void)
{
    uint8_t g1 = BQ76940_Read_Reg(ADCGAIN1);
    uint8_t g2 = BQ76940_Read_Reg(ADCGAIN2);

    
    ADC_GAIN_raw = (float)(((g1 & 0x0C) >> 2) | ((g2 & 0xE0) >> 3)); // 两位+三位，共5位，范围0..31

    ADC_offset = (int)((int8_t)BQ76940_Read_Reg(ADCOFFSET)); // 片内提供的有符号偏置补偿

    GAIN_uV = 365 + (int)ADC_GAIN_raw; // 单位：µV/LSB
}

static void OV_UV_PROTECT(void)
{
    
    uint8_t OV_TripVal, UV_TripVal;
    const float t = 0.377f;

    OV_TripVal = (uint8_t)((((uint16_t)((BQ40_OVP_mV - ADC_offset) / t + 0.5f)) >> 4) & 0xFF);
    UV_TripVal = (uint8_t)((((uint16_t)((BQ40_UVP_mV - ADC_offset) / t + 0.5f)) >> 4) & 0xFF);

    BQ76940_Write_Reg(OV_TRIP, OV_TripVal);
    BQ76940_Write_Reg(UV_TRIP, UV_TripVal);
}

static void OCD_SCD_PROTECT(void)
{
    /* PROTECT1 — 短路保护 (SCD)
     * bit 7   RSNS = 0         标准灵敏度（适合 4mΩ 采样电阻）
     * bit 6:5 reserved = 00
     * bit 4:3 SCD_DELAY = 10   200µs（过滤噪声，防止误触发）
     * bit 2:0 SCD_THRESH = 010 44mV → 44mV/4mΩ = 11A（约 9C for 1200mAh）
     *
     * 0b 0_00_10_010 = 0x12
     */
    BQ76940_Write_Reg(PROTECT1, 0x12);

    /* PROTECT2 — 过流保护 (OCD)
     * bit 7   reserved = 0
     * bit 6:4 OCD_DELAY = 011  80ms（允许短暂启动浪涌）
     * bit 3:0 OCD_THRESH = 0011 17mV → 17mV/4mΩ ≈ 4.25A（约 3.5C for 1200mAh）
     *
     * 0b 0_011_0011 = 0x33
     */
    BQ76940_Write_Reg(PROTECT2, 0x33);
}

void BQ76940_RCC(void)
{
		GPIO_InitTypeDef GPIO_InitStructure;
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
		GPIO_InitStructure.GPIO_Pin =MCU_WAKE_BQ;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 			 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA_PORT, &GPIO_InitStructure);
}
/*=============================
 * 初始化（无CRC，规范时序）
 *=============================*/
void BQ76940_Init(void)
{
		BQ76940_RCC();
    WAKE_ALL_DEVICE();
    Delay_ms(50);

    // 先清故障位、关均衡（不清 CC_READY）
    BQ76940_Write_Reg(SYS_STAT,  0x7F);
    BQ76940_Write_Reg(CELLBAL1,  0x00);
    BQ76940_Write_Reg(CELLBAL2,  0x00);
    BQ76940_Write_Reg(CELLBAL3,  0x00);

    // 基础配置（但 SYS_CTRL2 先不用开太多位）
    BQ_First_Config();

    // 打开 ADC（SYS_CTRL1=0x18 已经开 ADC/温度），给一点稳定时间
    Delay_ms(10);

    // 读取增益/偏置（此时 ADC/寄存器应可读）
    Get_offset_gain();

    // 配置 OVP/UVP 与 短路/过流保护
    OV_UV_PROTECT();
    OCD_SCD_PROTECT();

    // 最后再清一次故障位（不清 CC_READY）
    BQ76940_Write_Reg(SYS_STAT, 0x7F);
}

/*=============================
 * 电压读取
 *=============================*/
static inline int is_cell_present(uint8_t cell_num)
{
    if (cell_num < 1 || cell_num > 15) return 0;
    return (s_cell_present_mask >> (cell_num - 1)) & 0x1;
}

static uint8_t BQ76940_Cell_Present_Count(void)
{
    uint8_t count = 0U;
    uint16_t mask = s_cell_present_mask & 0x7FFFU;

    while (mask != 0U) {
        if ((mask & 0x1U) != 0U) {
            count++;
        }
        mask >>= 1;
    }
    return count;
}

int16_t BQ76940_Read_Cell_Voltage(uint8_t cell_num)
{
    if (cell_num < 1 || cell_num > 15) {
        return -1;
    }
    if (!is_cell_present(cell_num)) {
        return -1; // 未接入，直接标无效
    }

    uint8_t High_Addr = VC1_HI + (uint8_t)((cell_num - 1) * 2);
    uint8_t Low_Addr  = High_Addr + 1;

    uint16_t rawH = BQ76940_Read_Reg(High_Addr);
    uint16_t rawL = BQ76940_Read_Reg(Low_Addr);
    uint16_t raw  = (uint16_t)((rawH << 8) | rawL);

    // 原始码异常过滤
    if (raw == 0xFFFF || raw == 0x0000) {
        return -1;
    }

    // 换算：raw * (µV/LSB) / 1000 -> mV，再 +ADC_offset
    int32_t mV = ((int32_t)raw * GAIN_uV) / 1000 + ADC_offset;

     // 低值门槛（应对悬空/耦合/漏电导致的几十mV）
    if (mV < 200) return -1;

    if (mV < 0 || mV > 5000) return -1;

    // 调试打印
    // printf("Cell_Voltage_raw:0x%04X, H:%u, L:%u => %ld mV\r\n", raw, High_Addr, Low_Addr, (long)mV);

    return (int16_t)mV;
}

uint16_t BQ76940_Read_Pack_Voltage(void)
{
    uint16_t raw = ((uint16_t)BQ76940_Read_Reg(BAT_HI) << 8) |
                   BQ76940_Read_Reg(BAT_LO);

    if (raw == 0xFFFF || raw == 0x0000) {
        return 0;
    }

    {
        uint8_t cell_count = BQ76940_Cell_Present_Count();
        if (cell_count == 0U) {
            cell_count = 1U;
        }
        int32_t mV = ((int32_t)raw * GAIN_uV * 4) / 1000 +
                     (int32_t)ADC_offset * cell_count;

        if (mV < 0) {
            mV = 0;
        } else if (mV > 0xFFFF) {
            mV = 0xFFFF;
        }
        return (uint16_t)mV;
    }
}

/*=============================
 * 电流读取
 *=============================*/
int16_t BQ76940_Read_Current(void)
{
    uint16_t raw = ((uint16_t)BQ76940_Read_Reg(CC_HI) << 8) |
                   BQ76940_Read_Reg(CC_LO);

    if (raw == 0xFFFF) {
        return 0; // 读失败时返回0，按需处理
    }

    {
        int16_t cc_raw = (int16_t)raw;
        float current_ma = ((float)cc_raw * BQ76940_CC_LSB_UV) / BQ76940_RSENSE_MOHM;

        if (current_ma > (float)INT16_MAX) {
            return INT16_MAX;
        }
        if (current_ma < (float)INT16_MIN) {
            return INT16_MIN;
        }
        return (int16_t)((current_ma >= 0.0f) ? (current_ma + 0.5f)
                                             : (current_ma - 0.5f));
    }
}

/*=============================
 * 温度读取（保持原算法）
 *=============================*/
float BQ76940_Read_Temperature(uint8_t ts_num)
{
    uint8_t High_Addr, Low_Addr;

    switch (ts_num) {
        case 1: High_Addr = TS1_HI; Low_Addr = TS1_LO; break;
        case 2: High_Addr = TS2_HI; Low_Addr = TS2_LO; break;
        case 3: High_Addr = TS3_HI; Low_Addr = TS3_LO; break;
        default: return -273.15f;
    }

    uint16_t raw = ((uint16_t)BQ76940_Read_Reg(High_Addr) << 8) |
                    BQ76940_Read_Reg(Low_Addr);

    if (raw == 0xFFFF || raw == 0x0000) {
        return -273.15f;
    }

    float voltage_mv = raw * 0.382f;
    float rt = (10000.0f * voltage_mv) / (3300.0f - voltage_mv);

    float T2 = 273.15f + 25.0f;
    float Bx = 3380.0f;
    float Ka = 273.15f;

    float temp_c = 1.0f / (1.0f / T2 + logf(rt / 10000.0f) / Bx) - Ka;
    return temp_c;
}

/*=============================
 * CHG/DSG 控制
 *=============================*/
void BQ76940_Enable_CHG(uint8_t enable,uint8_t *chg_enable)
{
    /* 读当前 DSG 状态以保留，但始终显式置 CC_EN(bit6)=1，
     * 避免 read-modify-write 丢失 CC_EN 导致电流采样不工作 */
    uint8_t ctrl = BQ76940_Read_Reg(SYS_CTRL2);
    uint8_t dsg_bit = ctrl & 0x02;          /* 保留 DSG 位 */
    uint8_t new_ctrl = 0x40 | dsg_bit;      /* CC_EN 始终置 1 */
    if (enable) {
        new_ctrl |= 0x01;
        if (chg_enable) {
            *chg_enable = 0x01;
        }
    } else {
        if (chg_enable) {
            *chg_enable = 0x00;
        }
    }
    BQ76940_Write_Reg(SYS_CTRL2, new_ctrl);
    ctrl = BQ76940_Read_Reg(SYS_CTRL2);
    if (chg_enable) {
        *chg_enable = (ctrl & 0x01U) ? 1U : 0U;
    }
}

void BQ76940_Enable_DSG(uint8_t enable,uint8_t *dsg_enable)
{
    /* 读当前 CHG 状态以保留，但始终显式置 CC_EN(bit6)=1 */
    uint8_t ctrl = BQ76940_Read_Reg(SYS_CTRL2);
    uint8_t chg_bit = ctrl & 0x01;          /* 保留 CHG 位 */
    uint8_t new_ctrl = 0x40 | chg_bit;      /* CC_EN 始终置 1 */
    if (enable) {
        new_ctrl |= 0x02;
        if (dsg_enable) {
            *dsg_enable = 0x01;
        }
    } else {
        if (dsg_enable) {
            *dsg_enable = 0x00;
        }
    }
    BQ76940_Write_Reg(SYS_CTRL2, new_ctrl);
    ctrl = BQ76940_Read_Reg(SYS_CTRL2);
    if (dsg_enable) {
        *dsg_enable = (ctrl & 0x02U) ? 1U : 0U;
    }
}

void BQ76940_Enable_CHG_DSG(uint8_t chg_en, uint8_t dsg_en)
{
    uint8_t ctrl = BQ76940_Read_Reg(SYS_CTRL2);
    ctrl &= 0xFC;                 /* 清除 bit0(CHG) 和 bit1(DSG) */
    ctrl |= 0x40;                 /* 始终保持 CC_EN(bit6) 使能 */
    if (chg_en) ctrl |= 0x01;
    if (dsg_en) ctrl |= 0x02;
    BQ76940_Write_Reg(SYS_CTRL2, ctrl);
}

void BQ76940_Get_CHG_DSG_Status(uint8_t *chg_status, uint8_t *dsg_status)
{
    uint8_t ctrl = BQ76940_Read_Reg(SYS_CTRL2);
    if (chg_status) *chg_status = (ctrl & 0x01) ? 1 : 0;
    if (dsg_status) *dsg_status = (ctrl & 0x02) ? 1 : 0;
}



/*=============================
 * 均衡控制
 *=============================*/
void BQ76940_Enable_Balance(uint8_t cell_num, uint8_t enable)
{
    if (cell_num < 1 || cell_num > 15) return;
    if (!is_cell_present(cell_num))   return;

    uint8_t reg_addr;
    uint8_t bit_pos;

    if (cell_num <= 5) {
        reg_addr = CELLBAL1; bit_pos = cell_num - 1;
    } else if (cell_num <= 10) {
        reg_addr = CELLBAL2; bit_pos = cell_num - 6;
    } else {
        reg_addr = CELLBAL3; bit_pos = cell_num - 11;
    }

    uint8_t bal = BQ76940_Read_Reg(reg_addr);
    if (enable) bal |=  (1U << bit_pos);
    else        bal &= ~(1U << bit_pos);
    BQ76940_Write_Reg(reg_addr, bal);
}

void BQ76940_Disable_All_Balance(void)
{
    BQ76940_Write_Reg(CELLBAL1, 0x00);
    BQ76940_Write_Reg(CELLBAL2, 0x00);
    BQ76940_Write_Reg(CELLBAL3, 0x00);
}

/*=============================
 * 状态读写
 *=============================*/
uint8_t BQ76940_Read_Status(void)
{
    return BQ76940_Read_Reg(SYS_STAT);
}

void BQ76940_Clear_Status(void)
{
    /* 只清除故障位(bit0~bit6)，不清除 CC_READY(bit7)，
     * 避免重启 CC 转换周期导致电流永远读不到有效值 */
    BQ76940_Write_Reg(SYS_STAT, 0x7F);
}

/*=============================
 * 批量读取
 *=============================*/
void BQ76940_Read_All_Data(BQ76940_Data_t *data)
{
    if (!data) return;

    int32_t pack_sum = 0;
    uint8_t valid_cells = 0U;
    for (uint8_t i = 0; i < 15; i++) {
        int16_t cell = BQ76940_Read_Cell_Voltage(i + 1);
        data->cell_voltage[i] = cell;
        if (cell > 0) {
            pack_sum += cell;
            valid_cells++;
        }
    }

    uint16_t sum_pack_mv = (pack_sum > 65535L) ? 65535U : (uint16_t)pack_sum;
    uint16_t pack_mv = BQ76940_Read_Pack_Voltage();
    if (pack_mv == 0 && pack_sum > 0) {
        pack_mv = sum_pack_mv;
    }
    data->pack_voltage = pack_mv;
    data->current      = BQ76940_Read_Current();
    data->soc          = BQ76940_Calculate_SOC(sum_pack_mv, valid_cells);
    data->temperature[0]= BQ76940_Read_Temperature(1);
    data->temperature[1]= BQ76940_Read_Temperature(2);
    data->temperature[2]= BQ76940_Read_Temperature(3);

}

/*=============================
 * 简单基于电压的SOC
 *=============================*/
uint8_t BQ76940_Calculate_SOC(uint16_t pack_voltage, uint8_t cell_count)
{
    if (cell_count == 0) return 0;
    uint16_t avg_cell_voltage = (uint16_t)(pack_voltage / cell_count);

    /* 18650/NMC 静置 OCV-SOC 查找表。
     * 只能在小电流静置后作为校准参考；带载电压不能直接强校准 SOC。
     */
    typedef struct { int16_t voltage; uint8_t soc; } SOC_Table_t;
    static const SOC_Table_t soc_table[] = {
        {4200, 100},
        {4160,  95},
        {4110,  90},
        {4080,  85},
        {4020,  80},
        {3980,  75},
        {3940,  70},
        {3900,  65},
        {3860,  60},
        {3820,  55},
        {3790,  50},
        {3760,  45},
        {3730,  40},
        {3700,  35},
        {3670,  30},
        {3640,  25},
        {3610,  20},
        {3570,  15},
        {3500,  10},
        {3400,   5},
        {3000,   0}
    };
    const uint8_t n = sizeof(soc_table)/sizeof(soc_table[0]);

    uint8_t i;
    for (i = 0; i < n - 1; i++) {
        if (avg_cell_voltage >= soc_table[i].voltage) return soc_table[i].soc;
        if (avg_cell_voltage >  soc_table[i+1].voltage) {
            int16_t vdiff = soc_table[i].voltage - soc_table[i+1].voltage;
            uint8_t sdiff = soc_table[i].soc     - soc_table[i+1].soc;
            int16_t voff  = soc_table[i].voltage - avg_cell_voltage;
            return soc_table[i].soc - (uint8_t)((voff * sdiff) / vdiff);
        }
    }
    return 0;
}
