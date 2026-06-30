#ifndef __BQ76940_H
#define __BQ76940_H
/*
 * File: bq76940.h
 * Brief: TI BQ76940（3~15串）电池监测/保护IC 的寄存器与接口封装
 * Note :
 *   1) 本文件仅提供寄存器地址、位定义、默认保护参数与上层接口声明，
 *      具体 I2C 读写在 .c 文件中实现。
 *   2) 编码：UTF-8。若出现乱码，请确认工程字符集。
 *   3) 本项目对 SYS_STAT 的位定义与 TI 官方手册存在差异（见下文“参考位图”）。
 *      为避免破坏现有代码逻辑，保留项目位图；你可据手册校对后按需切换。
 */

#include "stm32f10x.h"

/******************************************************************************
 * GPIO 相关：唤醒引脚
 * 说明：
 *   - MCU_WAKE_BQ 接到 BQ76940 的 TS1/WAKE/或外部唤醒相关逻辑（视硬件而定）。
 *   - MCU_WAKE_BQ_ONOFF(x)：输出电平控制宏；x 取 BitAction(0/1)。
 ******************************************************************************/
#define GPIOA_RCC                 RCC_APB2Periph_GPIOA
#define GPIOA_PORT                GPIOA
#define MCU_WAKE_BQ               GPIO_Pin_8
#define MCU_WAKE_BQ_ONOFF(x)      GPIO_WriteBit(GPIOA_PORT, MCU_WAKE_BQ, (BitAction)(x))

/******************************************************************************
 * BQ76940 I2C 地址与读写字节
 * 说明：
 *   - 7bit 设备地址为 0x08。
 *   - 8bit 地址：写 0x10，读 0x11（= 0x08<<1 | R/W）。
 ******************************************************************************/
#define BQ76940_ADDR              0x08
#define BQ76940_I2C_WRITE         0x10
#define BQ76940_I2C_READ          0x11

/******************************************************************************
 * 配置/状态寄存器地址映射（参考 BQ769x0 系列数据手册）
 ******************************************************************************/
#define SYS_STAT                  0x00    /* 系统状态标志寄存器（写1清除相应标志）           */
#define CELLBAL1                  0x01    /* 均衡控制1（Cell 1~5）                             */
#define CELLBAL2                  0x02    /* 均衡控制2（Cell 6~10）                            */
#define CELLBAL3                  0x03    /* 均衡控制3（Cell 11~15）                           */
#define SYS_CTRL1                 0x04    /* 系统控制1（ADC、温度通道选择、关断A/B 等）         */
#define SYS_CTRL2                 0x05    /* 系统控制2（电流计、一次转换、充放 FET 控制等）     */
#define PROTECT1                  0x06    /* 保护设置1（SCD/OCD 延时、阈值相关 LSB）           */
#define PROTECT2                  0x07    /* 保护设置2                                         */
#define PROTECT3                  0x08    /* 保护设置3                                         */
#define OV_TRIP                   0x09    /* 过压阈值设置（COV 阈值，单位见手册）               */
#define UV_TRIP                   0x0A    /* 欠压阈值设置（CUV 阈值）                           */
#define CC_CFG                    0x0B    /* 电流计配置（采样累加/采样模式等）                  */

/* 单体电压寄存器（VC1~VC15，高低字节），单位：见手册转换（一般 mV 需结合增益/偏置） */
#define VC1_HI                    0x0C
#define VC1_LO                    0x0D
#define VC2_HI                    0x0E
#define VC2_LO                    0x0F
#define VC3_HI                    0x10
#define VC3_LO                    0x11
#define VC4_HI                    0x12
#define VC4_LO                    0x13
#define VC5_HI                    0x14
#define VC5_LO                    0x15
#define VC6_HI                    0x16
#define VC6_LO                    0x17
#define VC7_HI                    0x18
#define VC7_LO                    0x19
#define VC8_HI                    0x1A
#define VC8_LO                    0x1B
#define VC9_HI                    0x1C
#define VC9_LO                    0x1D
#define VC10_HI                   0x1E
#define VC10_LO                   0x1F
#define VC11_HI                   0x20
#define VC11_LO                   0x21
#define VC12_HI                   0x22
#define VC12_LO                   0x23
#define VC13_HI                   0x24
#define VC13_LO                   0x25
#define VC14_HI                   0x26
#define VC14_LO                   0x27
#define VC15_HI                   0x28
#define VC15_LO                   0x29

/* 电池包总电压（BAT），为 VC 采样经内部求和/分压得到，换算见手册 */
#define BAT_HI                    0x2A
#define BAT_LO                    0x2B

/* 温度通道（TS1~TS3），需根据热敏/NTC 阵列与内部参考换算成 ℃ */
#define TS1_HI                    0x2C
#define TS1_LO                    0x2D
#define TS2_HI                    0x2E
#define TS2_LO                    0x2F
#define TS3_HI                    0x30
#define TS3_LO                    0x31

/* 电流计累加值（CC），配合 CC_CFG、ADC 增益/偏置计算 mA 或 mAh */
#define CC_HI                     0x32
#define CC_LO                     0x33

/* ADC 增益与偏置校准寄存器（用于把原始码值换算成工程量） */
#define ADCGAIN1                  0x50
#define ADCOFFSET                 0x51
#define ADCGAIN2                  0x59

/******************************************************************************
 * SYS_STAT 位定义（当前项目版）
 * 注意：
 *   - 下列位图为本项目既有宏，可能与 TI 官方手册位序不同。
 *   - 若你依据手册实现，请参阅后面的“参考：TI 官方位图”，再统一修改工程。
 ******************************************************************************/
#define SYS_STAT_CC_READY         (1 << 7)  /* Coulomb counter sample ready       */
#define SYS_STAT_DEVICE_XREADY    (1 << 6)  /* Internal device fault              */
#define SYS_STAT_OVRD_ALERT       (1 << 5)  /* Alert override                     */
#define SYS_STAT_SCD              (1 << 4)  /* Short-circuit discharge            */
#define SYS_STAT_OCD              (1 << 3)  /* Over-current discharge             */
#define SYS_STAT_CUV              (1 << 2)  /* Cell under-voltage                 */
#define SYS_STAT_COV              (1 << 1)  /* Cell over-voltage                  */
#define SYS_STAT_DEVICE_READY     (1 << 0)  /* Device ready                       */

/*------------------------------------------------------------------------------
 * 参考：TI 官方 SYS_STAT 位定义（仅供校对，勿直接替换）
 * 据常见资料：
 *   Bit7: CC_READY
 *   Bit6: DEVICE_XREADY
 *   Bit5: OVRD_ALERT
 *   Bit4: SCD
 *   Bit3: OCD
 *   Bit2: UV
 *   Bit1: OV
 *   Bit0: DEVICE_READY
 * 如与项目定义不一致，请以你的数据手册版本为准，统一修改宏与业务逻辑。
 *----------------------------------------------------------------------------*/

/******************************************************************************
 * SYS_CTRL1 位定义
 * LOAD_PRESENT：负载存在检测（若硬件支持）
 * ADC_EN      ：使能 ADC（必须置位才能读到电压温度等）
 * TEMP_SEL    ：温度通道选择（内部/外部 NTC，依手册）
 * SHUT_A/B    ：关断 A/B（配合硬件进入/退出关断）
 ******************************************************************************/
#define SYS_CTRL1_LOAD_PRESENT    (1 << 7)
#define SYS_CTRL1_ADC_EN          (1 << 4)
#define SYS_CTRL1_TEMP_SEL        (1 << 3)
#define SYS_CTRL1_SHUT_A          (1 << 1)
#define SYS_CTRL1_SHUT_B          (1 << 0)

/******************************************************************************
 * SYS_CTRL2 位定义
 * DELAY_DIS  ：关闭部分保护延时（调试慎用）
 * CC_EN      ：使能电流计（累加/采样）
 * CC_ONESHOT ：单次电流计转换
 * DSG_ON     ：放电 FET 使能
 * CHG_ON     ：充电 FET 使能
 ******************************************************************************/
#define SYS_CTRL2_DELAY_DIS       (1 << 7)
#define SYS_CTRL2_CC_EN           (1 << 6)
#define SYS_CTRL2_CC_ONESHOT      (1 << 5)
#define SYS_CTRL2_DSG_ON          (1 << 1)
#define SYS_CTRL2_CHG_ON          (1 << 0)

/******************************************************************************
 * 保护参数默认值（可按电芯规格、应用场景调整）
 * 单位说明：
 *   - 电压：mV；温度：℃；电流阈值宏名中用 mV 表示为“分流器压降等效 mV”
 *     若走分流电阻 Rsense，则 I_threshold ≈ (阈值mV) / Rsense。
 ******************************************************************************/
#define BQ40_OVP_mV               4400      /* 单体过压阈值（COV Trip）      */
#define BQ40_OVR_mV               4300      /* 单体过压恢复（COV Release）   */
#define BQ40_UVP_mV               2400      /* 单体欠压阈值（CUV Trip）      */
#define BQ40_UVR_mV               2500      /* 单体欠压恢复（CUV Release）   */

#define BQ40_OCD_THRESHOLD_mV     50        /* 放电过流阈值等效 mV（配 Rsense）*/
#define BQ40_SCD_THRESHOLD_mV     100       /* 短路阈值等效 mV                */

#define BQ40_OTP_THRESHOLD        60        /* 过温保护阈值（℃）             */
#define BQ40_OTR_THRESHOLD        50        /* 过温恢复（℃）                 */
#define BQ40_UTP_THRESHOLD        -10       /* 低温保护阈值（℃）             */
#define BQ40_UTR_THRESHOLD        0         /* 低温恢复（℃）                 */

/******************************************************************************
 * 电池参数与量程
 ******************************************************************************/
#define CELL_COUNT_MAX            15        /* 最大支持 15 串                  */
#define TEMP_SENSOR_COUNT         3         /* 最多 3 路温度传感器（TS1~TS3） */

#define CELL_VOLTAGE_MAX          4200      /* 单体电压上限（mV，用于软件限幅）*/
#define CELL_VOLTAGE_MIN          2500      /* 单体电压下限（mV）             */
#define CELL_VOLTAGE_NOMINAL      3700      /* 名义电压（mV，用于估算 SOC）   */

/******************************************************************************
 * 数据类型定义：一次性打包读取所需的关键量
 ******************************************************************************/
typedef struct {
    int16_t  cell_voltage[CELL_COUNT_MAX];  /* 单体电压(mV)，负值表示无效/未接 */
    uint16_t pack_voltage;                  /* 包电压(mV)                      */
    int16_t  current;                       /* 电流(mA，充电为正/放电为负)     */
    float    temperature[TEMP_SENSOR_COUNT];/* 温度(℃)                         */
    uint8_t  soc;                           /* 估算荷电状态(%)                 */
    uint8_t  chg_enable;                    /* 充电 FET 使能状态               */
    uint8_t  dsg_enable;                    /* 放电 FET 使能状态               */
} BQ76940_Data_t;

/******************************************************************************
 * 错误码
 ******************************************************************************/
typedef enum {
    BQ76940_OK = 0,
    BQ76940_ERROR,
    BQ76940_ERROR_I2C,
    BQ76940_ERROR_PARAM,
    BQ76940_ERROR_TIMEOUT,
    BQ76940_ERROR_CRC
} BQ76940_Status_t;

/******************************************************************************
 * 函数声明 —— 初始化与配置
 * BQ76940_Init        ：芯片上电后的基础初始化（ADC 使能、清状态、配置保护等）
 * WAKE_ALL_DEVICE     ：通过外设/引脚把芯片从 SHIP/休眠唤醒（依硬件）
 * SHIP_ALL_DEVICE     ：进入出厂/运输模式（极低功耗，需唤醒序列才能恢复）
 * BQ76940_RCC         ：与本 MCU 相关的 RCC 使能封装（GPIO/I2C 时钟等）
 * BQ76940_Set_Cell_Present_Mask：配置实际接入的电芯掩码
 ******************************************************************************/
void BQ76940_Init(void);
void WAKE_ALL_DEVICE(void);
void SHIP_ALL_DEVICE(void);
void BQ76940_RCC(void);
void BQ76940_Set_Cell_Present_Mask(uint16_t mask);

/******************************************************************************
 * 函数声明 —— 电压读取
 * BQ76940_Read_Cell_Voltage：读取指定单体（1~15）电压（mV）
 * BQ76940_Read_Pack_Voltage：读取整包电压（mV）
 ******************************************************************************/
int16_t  BQ76940_Read_Cell_Voltage(uint8_t cell_num);
uint16_t BQ76940_Read_Pack_Voltage(void);

/******************************************************************************
 * 函数声明 —— 电流与温度
 * Current：mA，需结合增益/偏置与 Rsense 换算
 * Temperature：℃，根据 NTC 分压/内部参考计算
 ******************************************************************************/
int16_t BQ76940_Read_Current(void);
float   BQ76940_Read_Temperature(uint8_t ts_num);

/******************************************************************************
 * 函数声明 —— 开关管控制
 * Enable_CHG/DSG     ：打开/关闭充电或放电 FET，并回传状态
 * Enable_CHG_DSG     ：同时控制充/放
 * Get_CHG_DSG_Status ：读取当前 FET 状态
 ******************************************************************************/
void BQ76940_Enable_CHG(uint8_t enable, uint8_t *chg_enable);
void BQ76940_Enable_DSG(uint8_t enable, uint8_t *dsg_enable);
void BQ76940_Enable_CHG_DSG(uint8_t chg_en, uint8_t dsg_en);
void BQ76940_Get_CHG_DSG_Status(uint8_t *chg_status, uint8_t *dsg_status);

/******************************************************************************
 * 函数声明 —— 均衡控制
 * Enable_Balance     ：对指定单体开启/关闭被动均衡（注意热设计与占空）
 * Disable_All_Balance：关闭所有均衡位
 ******************************************************************************/
void BQ76940_Enable_Balance(uint8_t cell_num, uint8_t enable);
void BQ76940_Disable_All_Balance(void);

/******************************************************************************
 * 函数声明 —— 状态与告警
 * Read_Status  ：读取 SYS_STAT
 * Clear_Status ：写 1 清除相应告警位（读 -> 置位 -> 写回）
 ******************************************************************************/
uint8_t BQ76940_Read_Status(void);
void    BQ76940_Clear_Status(void);

/******************************************************************************
 * 函数声明 —— 批量操作/便捷接口
 * Read_All_Data ：一次性读取电压、电流、温度、FET 状态并估算 SOC
 * Calculate_SOC ：基于包电压与串数的“电压法”粗略估算 SOC（需根据电芯曲线调整）
 ******************************************************************************/
void    BQ76940_Read_All_Data(BQ76940_Data_t *data);
uint8_t BQ76940_Calculate_SOC(uint16_t pack_voltage, uint8_t cell_count);

#endif // __BQ76940_H
