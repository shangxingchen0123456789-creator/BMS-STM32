#ifndef BMS_APP_H
#define BMS_APP_H

#include <stdint.h>
#include "FreeRTOS.h"

#define BMS_MAX_CELLS             15U
#define BMS_TEMP_SENSOR_COUNT     3U

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ───────── 采样/容量初值与下限 ─────────
 * BMS_MEASUREMENT_PERIOD_MS：测量任务周期（ms）
 * BMS_SOC_INITIAL_PERCENT：上电初始 SOC（%），用于给库仑计初值
 * BMS_PACK_NOMINAL_CAPACITY_MAH：电池包标称容量（mAh）
 * BMS_SOC_MIN_CAPACITY_MAH：学习容量的下限（防止估算过小）
 * BMS_SOC_CURRENT_DEADBAND_MA：库仑积分电流死区，抑制零点漂移
 * BMS_SOC_OCV_*：静置后用 OCV 查表对库仑计做小权重校准
 */
#define BMS_MEASUREMENT_PERIOD_MS          100U
#define BMS_SOC_INITIAL_PERCENT            50.0f
#define BMS_PACK_NOMINAL_CAPACITY_MAH      1200.0f     /* 18650 单体 1200mAh */
#define BMS_SOC_MIN_CAPACITY_MAH           500.0f      /* 学习容量下限 */
#define BMS_SOC_CURRENT_DEADBAND_MA        10.0f
#define BMS_SOC_IDLE_CURRENT_MA            50.0f
#define BMS_SOC_OCV_IDLE_TIME_MS           300000UL
#define BMS_SOC_OCV_BLEND                  0.10f
#define BMS_SOC_CHARGE_EFFICIENCY          0.995f

/* ───────── 电压阈值（含释放滞回） ─────────
 * 单体过压/欠压阈值与释放阈值（形成滞回，抑制边界抖动）。
 * 整包阈值简单乘以 BMS_MAX_CELLS（见「注意点」）。
 */
#define BMS_CELL_OV_THRESHOLD_MV           4200
#define BMS_CELL_OV_RELEASE_MV             4100
#define BMS_CELL_UV_THRESHOLD_MV           2700
#define BMS_CELL_UV_RELEASE_MV             2850

/* 充电恢复滞回：OV 触发关充电后，需所有电芯降到此值才重新允许充电 */
#define BMS_CHG_RESUME_CELL_MV             4100

#define BMS_PACK_OV_THRESHOLD_MV           (BMS_CELL_OV_THRESHOLD_MV * BMS_MAX_CELLS)
#define BMS_PACK_UV_THRESHOLD_MV           (BMS_CELL_UV_THRESHOLD_MV * BMS_MAX_CELLS)
#define BMS_PACK_UV_RELEASE_MV             (BMS_CELL_UV_RELEASE_MV * BMS_MAX_CELLS)

/* ───────── 电流阈值（放电为负） ─────────
 * 过流与短路阈值（单位 mA）。在故障判定中使用 |-电流| 与阈值比较。
 */
#define BMS_CURRENT_OC_DISCHARGE_MA        4000     /* 4A ≈ 3.3C（1200mAh 电池） */
#define BMS_CURRENT_SHORT_CIRCUIT_MA       10000    /* 10A ≈ 8C（短路保护） */

/* ───────── 温度阈值（单位：0.1℃） ─────────
 * 充/放电允许的温度上下限；Fault 任务里分别对充/放做越界判定。
 */
#define BMS_TEMP_CHARGE_MAX_DECIC          (45 * 10)
#define BMS_TEMP_CHARGE_MIN_DECIC          (0 * 10)
#define BMS_TEMP_DISCHARGE_MAX_DECIC       (60 * 10)
#define BMS_TEMP_DISCHARGE_MIN_DECIC       (-20 * 10)

/* ───────── SOC 学习参数 ─────────
 * 在 SOC≤20% 进入学习相（记录充入量）；
 * 当 SOC≥95% 且这段充入量 > 现容量的 50% 时，用 10% 融合更新学习容量。
 */
#define BMS_SOC_LEARN_LOW_PERCENT          20U
#define BMS_SOC_LEARN_HIGH_PERCENT         95U
#define BMS_SOC_LEARN_BLEND                0.10f

/* ───────── 均衡阈值（带滞回） ─────────
 * 压差 ≥30mV：对 “高于(最小+30mV)” 的电芯开启均衡；
 * 压差 ≤15mV：关闭均衡；
 * 中间区域：保持现状（滞回）。
 */
#define BMS_BALANCE_ENABLE_DIFF_MV         30
#define BMS_BALANCE_DISABLE_DIFF_MV        15

/* ───────── 故障恢复门限 ─────────
 * 需要连续 N 次“无故障”采样周期后才解除锁存。
 */
#define BMS_RECOVERY_CYCLES_REQUIRED       3U

/* ───────── 活跃电流判定 ─────────
 * 控制任务中判断“正在充/正在放”的阈值，避免微小纹波引发状态抖动。
 */
#define BMS_CURRENT_ACTIVE_CHG_MA          100      /* >100mA 认为正在充电 */
#define BMS_CURRENT_ACTIVE_DSG_MA          100      /* >100mA 认为正在放电 */

/* ───────── 事件组 bit 定义 ─────────
 * MEAS_*：采样数据已更新，按消费者拆分，避免多个任务抢清同一个事件位
 * SOC_READY：SOC 任务完成一次更新
 * HOST_CMD：收到来自主机的充/放请求变更
 * FAULTS_UPDATED：故障掩码/锁存状态更新
 */
#define BMS_EVT_MEAS_FAULT       (1U << 0)
#define BMS_EVT_MEAS_SOC         (1U << 1)
#define BMS_EVT_MEAS_BALANCE     (1U << 2)
#define BMS_EVT_MEAS_CONTROL     (1U << 3)
#define BMS_EVT_MEASUREMENT      (BMS_EVT_MEAS_FAULT | BMS_EVT_MEAS_SOC | \
                                  BMS_EVT_MEAS_BALANCE | BMS_EVT_MEAS_CONTROL)
#define BMS_EVT_SOC_READY        (1U << 4)
#define BMS_EVT_HOST_CMD         (1U << 5)
#define BMS_EVT_FAULTS_UPDATED   (1U << 6)

typedef struct {
    uint8_t  cell_count; //电池节数
    int16_t  cell_mv[BMS_MAX_CELLS];//单串电压
    uint16_t pack_mv;//整包电压
    int16_t  current_ma;//电流
    int16_t  temperature_dC[BMS_TEMP_SENSOR_COUNT];//温度
} BMS_MeasurementSnapshot_t;


typedef struct {
    uint8_t cell_ov;//单串过压
    uint8_t cell_uv;//单传欠压
    uint8_t pack_ov;//整包过压
    uint8_t pack_uv;//整包欠压
    uint8_t ocd;//过流
    uint8_t scd;//短路（放电方向大电流）
    uint8_t charge_ot;//充电高温
    uint8_t charge_ut;//充电低温
    uint8_t discharge_ot;//放电高温
    uint8_t discharge_ut;//放电低温
    uint8_t bq_xready;//BQ内部异常
    uint8_t bq_ovrd_alert;//BQ ALERT 覆盖
} BMS_FaultFlags_t;


typedef uint8_t BMS_RunState_t;
#define BMS_STATE_IDLE          ((BMS_RunState_t)0)
#define BMS_STATE_CHARGING      ((BMS_RunState_t)1)
#define BMS_STATE_DISCHARGING   ((BMS_RunState_t)2)
#define BMS_STATE_FAULT         ((BMS_RunState_t)3)

typedef struct {
    BMS_MeasurementSnapshot_t measurement;
    TickType_t                measurement_tick;//拿到原始数据时的FreeRTOS的节拍
    uint32_t                  timestamp_ms;//时间戳

    BMS_FaultFlags_t          faults;
    uint16_t                  fault_mask;//故障位图
    uint8_t                   fault_latched;//故障标志
    uint8_t                   recovery_counter;//连续无故障周期技术
    uint8_t                   fault_confirm_count;//连续故障确认计数

    float                     coulomb_mAh;//实时库仑计
    float                     learned_capacity_mAh;//动态学习得到的满充容量
    float                     discharge_accum_mAh;//放电过程累计放电量，用于周期计数
    float                     throughput_mAh;//电池历史总吞吐量
    float                     learning_accum_mAh;//学习期累计充/放电量
    float                     learning_start_soc_pct;//容量学习开始时的SOC
    uint8_t                   learning_phase;//学习阶段
    uint32_t                  cycle_count;//充电周期

    uint8_t                   soc_pct;
    uint8_t                   soh_pct;
    uint16_t                  learned_capacity_10mAh;
    uint16_t                  remaining_capacity_10mAh;

    uint8_t                   charge_request;//上位机给的请求
    uint8_t                   discharge_request;
    uint8_t                   charge_enabled;//实际FET驱动状态
    uint8_t                   discharge_enabled;

    BMS_RunState_t            run_state;
} BMS_RuntimeState_t;


enum {
    BMS_FAULT_CELL_OV         = (1u << 0),
    BMS_FAULT_CELL_UV         = (1u << 1),
    BMS_FAULT_PACK_OV         = (1u << 2),
    BMS_FAULT_PACK_UV         = (1u << 3),
    BMS_FAULT_OCD             = (1u << 4),
    BMS_FAULT_SCD             = (1u << 5),
    BMS_FAULT_CHG_OT          = (1u << 6),
    BMS_FAULT_CHG_UT          = (1u << 7),
    BMS_FAULT_DSG_OT          = (1u << 8),
    BMS_FAULT_DSG_UT          = (1u << 9),
    BMS_FAULT_BQ_XREADY       = (1u << 10),
    BMS_FAULT_BQ_OVRD_ALERT   = (1u << 11),
    BMS_FAULT_RESERVED12      = (1u << 12),
    BMS_FAULT_RESERVED13      = (1u << 13),
    BMS_FAULT_RESERVED14      = (1u << 14),
    BMS_FAULT_LATCHED         = (1u << 15)
};

typedef struct {
    uint32_t timestamp_ms;
    uint16_t pack_voltage_mv;
    int16_t  current_ma;
    int16_t  cell_mv[BMS_MAX_CELLS];
    uint8_t  cell_count;
    int16_t  temperature_dC[BMS_TEMP_SENSOR_COUNT];
    uint8_t  charge_enabled;
    uint8_t  discharge_enabled;
    uint8_t  soc_pct;
    uint8_t  soh_pct;
    uint16_t cycle_count;
    uint16_t learned_capacity_10mAh;
    uint16_t remaining_capacity_10mAh;
    uint16_t fault_flags;
    BMS_RunState_t state;
} BMS_Telemetry_t;


/* ───────── 运行时可配置阈值结构体 ─────────
 * 上位机通过 CMD_SET_THRESHOLDS 命令动态修改这些值。
 * 默认值与上方 #define 宏一致，掉电后恢复默认。
 */
typedef struct {
    uint16_t cell_ov_mv;            /* 单体过压阈值 mV,   默认 4200  */
    uint16_t cell_ov_release_mv;    /* 单体过压释放 mV,   默认 4100  */
    uint16_t cell_uv_mv;            /* 单体欠压阈值 mV,   默认 2700  */
    uint16_t cell_uv_release_mv;    /* 单体欠压释放 mV,   默认 2850  */
    uint32_t ocd_ma;                /* 过流阈值 mA,       默认 100000 */
    int16_t  chg_ot_dC;             /* 充电高温上限 0.1°C, 默认 450   */
    int16_t  chg_ut_dC;             /* 充电低温下限 0.1°C, 默认 0     */
    int16_t  dsg_ot_dC;             /* 放电高温上限 0.1°C, 默认 600   */
    int16_t  dsg_ut_dC;             /* 放电低温下限 0.1°C, 默认 -200  */
} BMS_Thresholds_t;

extern BMS_Thresholds_t g_thresholds;
void BMS_SetThresholds(const BMS_Thresholds_t *t);


void BMS_AppInit(void);
void BMS_RequestChargeEnable(uint8_t enable);
void BMS_RequestDischargeEnable(uint8_t enable);
BaseType_t BMS_GetTelemetry(BMS_Telemetry_t *out);
void BMS_Task_Measure(void *pvParameters);
void BMS_Task_Fault(void *pvParameters);
void BMS_Task_SOC(void *pvParameters);
void BMS_Task_Balance(void *pvParameters);
void BMS_Task_Control(void *pvParameters);

#endif /* BMS_APP_H */
