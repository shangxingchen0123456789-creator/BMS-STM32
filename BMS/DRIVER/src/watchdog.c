#include "watchdog.h"

#include "stm32f10x_rcc.h"
#include "stm32f10x_iwdg.h"
#include "bms_app.h"
#include "FreeRTOS.h"
#include "task.h"

#ifndef LSI_VALUE
#define LSI_VALUE 40000U
#endif

void Watchdog_Init(uint32_t timeout_ms)
{
    if (timeout_ms == 0U) {
        timeout_ms = 1U;
    }

    RCC_LSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) {
    }

    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(IWDG_Prescaler_64);

    uint32_t ticks = (LSI_VALUE / 64U) * timeout_ms / 1000U;
    if (ticks == 0U) {
        ticks = 1U;
    }
    if (ticks > 0x0FFFU) {
        ticks = 0x0FFFU;
    }
    IWDG_SetReload((uint16_t)ticks);
    IWDG_ReloadCounter();
    IWDG_Enable();
}

void Watchdog_Kick(void)
{
    IWDG_ReloadCounter();
}

static void Watchdog_Task(void *pvParameters);

void Watchdog_ServiceInit(void)
{
    Watchdog_Init(1200U);
    Watchdog_Kick();

    if (xTaskCreate(Watchdog_Task,
                    "Wdog",
                    configMINIMAL_STACK_SIZE,
                    NULL,
                    tskIDLE_PRIORITY + 4U,
                    NULL) != pdPASS) {
        for (;;) {
        }
    }
}

static void Watchdog_Task(void *pvParameters)
{
    (void)pvParameters;

    const TickType_t period = pdMS_TO_TICKS(200U);
    const uint32_t stale_threshold_ms = 1000U;
    TickType_t last = xTaskGetTickCount();
    BMS_Telemetry_t telem;

    for (;;) {
        vTaskDelayUntil(&last, period);

        if (BMS_GetTelemetry(&telem) != pdTRUE) {
            continue;
        }

        TickType_t ticks_now = xTaskGetTickCount();
        uint32_t now_ms = (uint32_t)ticks_now * 1000UL / configTICK_RATE_HZ;
        uint32_t age_ms = (now_ms >= telem.timestamp_ms) ? (now_ms - telem.timestamp_ms) : 0U;

        if (age_ms <= stale_threshold_ms) {
            Watchdog_Kick();
        }
    }
}
