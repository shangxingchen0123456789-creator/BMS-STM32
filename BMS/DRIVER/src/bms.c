#include "bms_app.h"

#include <math.h>
#include <string.h>
#include <limits.h>

#include "bq76940.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"

//定义互斥量和事件组
static SemaphoreHandle_t   s_state_mutex = NULL;     /* 保护 g_bms_state */
static SemaphoreHandle_t   s_bq_mutex = NULL;        /* 保护 BQ76940 I2C 访问 */
static EventGroupHandle_t  s_events = NULL;          /* system-wide event flags */
// ───────── 公共接口实现 ─────────
void BMS_APP_Init(void)
{
    // 创建互斥锁
    s_state_mutex = xSemaphoreCreateMutex(); 
    s_bq_mutex = xSemaphoreCreateMutex();  
    s_events = xEventGroupCreate();
    
    memset(&g_bms_state,0,sizeof(g_bms_state));//清零全局状态结构体
    
    /* 初始库仑计 = 标称容量 × 初始SOC(%)；学习容量 = 标称容量 */
    g_bms_state.coulomb_mAh = BMS_PACK_NOMINAL_CAPACITY_MAH * (BMS_SOC_INITIAL_PERCENT / 100.0f);
    g_bms_state.learned_capacity_mAh = BMS_PACK_NOMINAL_CAPACITY_MAH;
    g_bms_state.learning_phase = 0;
    g_bms_state.soc_pct = (uint8_t)BMS_SOC_INITIAL_PERCENT;
    g_bms_state.soh_pct = 100U;
    g_bms_state.charge_request = 1U;      /* 默认允许充 */
    g_bms_state.discharge_request = 1U;   /* 默认允许放 */
    g_bms_state.charge_enabled = 0U;      /* FET 初始关闭 */
    g_bms_state.discharge_enabled = 0U;
    g_bms_state.run_state = BMS_STATE_IDLE;

    /* 创建测量任务：周期性读 BQ 并发布测量事件 */
    if (xTaskCreate(BMS_Task_Measure,
                    "Meas",
                    configMINIMAL_STACK_SIZE * 3U, /* 适度放大栈 */
                    NULL,
                    tskIDLE_PRIORITY + 3U,
                    NULL) != pdPASS) {
        for (;;) { /* 创建失败：停机等待看门狗 */ }
    }
    /* 创建故障任务：快速响应故障评估，优先级最高 */
    if (xTaskCreate(BMS_Task_Fault,
                    "Fault",
                    configMINIMAL_STACK_SIZE * 2U,
                    NULL,
                    tskIDLE_PRIORITY + 4U,
                    NULL) != pdPASS) {
        for (;;) { }
    }
    /* 创建 SOC 任务：基于测量进行库仑积算与学习 */
    if (xTaskCreate(BMS_Task_SOC,
                    "SOC",
                    configMINIMAL_STACK_SIZE * 2U,
                    NULL,
                    tskIDLE_PRIORITY + 2U,
                    NULL) != pdPASS) {
        for (;;) { }
    }
    //创建均衡任务
    if (xTaskCreate(BMS_Task_Balance,
                    "Bal",
                    configMINIMAL_STACK_SIZE * 2U,
                    NULL,
                    tskIDLE_PRIORITY + 1U,
                    NULL) != pdPASS) {
        for (;;) { }
    }
    /* 创建控制任务：综合条件开关 FET 与状态机更新 */
    if (xTaskCreate(BMS_Task_Control,
                    "Ctrl",
                    configMINIMAL_STACK_SIZE * 2U,
                    NULL,
                    tskIDLE_PRIORITY + 3U,
                    NULL) != pdPASS) {
        for (;;) { }
    }

}
/* 保护 BMS 状态结构体的互斥锁接口 */
Base_type_t BMS_lockState(TickType_t timeout)
{
    if (s_state_mutex == NULL) {
        return pdFALSE;
    }
    return xSemaphoreTake(s_state_mutex, timeout);
}
void BMS_UnlockState(void)
{
    if (s_state_mutex != NULL) {
        xSemaphoreGive(s_state_mutex);
    }
}
/* 保护 BQ76940 访问的互斥锁接口 */
Base_type_t BMS_LockBQ(TickType_t timeout)
{
    if (s_bq_mutex == NULL) {
        return pdFALSE;
    }
    return xSemaphoreTake(s_bq_mutex, timeout);
}
void BMS_UnlockBQ(void)
{
    if (s_bq_mutex != NULL) {
        xSemaphoreGive(s_bq_mutex);
    }
}
/*─────────────────────────────────────────────────────────────────────────────
 * 任务：BMS_Task_Fault
 * 触发：等待 BMS_EVT_MEASUREMENT（有新测量值）
 * 职责：
 *   - 读取 BQ 状态寄存器并清除一次性位
 *   - 依据状态寄存器与软件阈值，更新细分故障标志
 *   - 汇总为 fault_mask 并维护锁存/恢复计数
 *   - 广播 BMS_EVT_FAULTS_UPDATED（故障状态已更新）
 *─────────────────────────────────────────────────────────────────────────────
 */
void BMS_Task_Fault(void *pvParameters)
{