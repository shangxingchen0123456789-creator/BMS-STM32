#include "stm32f10x.h"
#include "led.h"
#include "delay.h"
#include "usart.h"
#include "iic.h"
#include "bq76940.h"
#include "nvic.h"
#include "tim.h"
#include "Communicate.h"
#include "bms_app.h"
#include "watchdog.h"
#include "exit.h"
#include "FreeRTOS.h"
#include "task.h"

#define COMM_TASK_STACK_SIZE   (configMINIMAL_STACK_SIZE * 4U)
#define COMM_TASK_PRIORITY     (tskIDLE_PRIORITY + 2U)
#define LED_TASK_STACK_SIZE    (configMINIMAL_STACK_SIZE)
#define LED_TASK_PRIORITY      (tskIDLE_PRIORITY + 1U)

static void CommTask(void *pvParameters);
static void LedTask(void *pvParameters);

int main(void)
{
    SystemCoreClockUpdate();

    Delay_Init();
    LED_Init();
    IIC_Init();
    NvicConfig();
    TIM_Init();
    USART_init(115200);

    BQ76940_Init();       /* 先初始化 BQ76940 硬件（I2C/ADC/保护） */
    BMS_AppInit();        /* 再初始化 BMS 应用（会读 BQ76940 做 OCV 查表） */
    
    Comm_Init();
    Watchdog_ServiceInit();

    if (xTaskCreate(CommTask, "Comm", COMM_TASK_STACK_SIZE, NULL, COMM_TASK_PRIORITY, NULL) != pdPASS) {
        while (1) {
        }
    }

    if (xTaskCreate(LedTask, "LED", LED_TASK_STACK_SIZE, NULL, LED_TASK_PRIORITY, NULL) != pdPASS) {
        while (1) {
        }
    }

    vTaskStartScheduler();

    while (1) {
			
    }
}

static void CommTask(void *pvParameters)
{
    (void)pvParameters;
    const TickType_t delay_ticks = pdMS_TO_TICKS(1U);

    for (;;) {
        Comm_PeriodicTask();
        vTaskDelay(delay_ticks);
    }
}

static void LedTask(void *pvParameters)
{
    (void)pvParameters;
    const TickType_t delay_ticks = pdMS_TO_TICKS(500U);

    for (;;) {
        LED_TOGGLE();
        vTaskDelay(delay_ticks);
    }
}
