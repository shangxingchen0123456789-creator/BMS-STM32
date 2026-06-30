#include "delay.h"
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"

#ifndef DWT_BASE
#define DWT_BASE (0xE0001000UL)
#endif

#ifndef DWT
typedef struct
{
    __IO uint32_t CTRL;
    __IO uint32_t CYCCNT;
    __IO uint32_t CPICNT;
    __IO uint32_t EXCCNT;
    __IO uint32_t SLEEPCNT;
    __IO uint32_t LSUCNT;
    __IO uint32_t FOLDCNT;
    __I  uint32_t PCSR;
} DWT_Type;
#define DWT ((DWT_Type *)DWT_BASE)
#endif

#ifndef DWT_CTRL_CYCCNTENA_Msk
#define DWT_CTRL_CYCCNTENA_Msk (1UL << 0)
#endif

#ifndef CoreDebug_DEMCR_TRCENA_Msk
#define CoreDebug_DEMCR_TRCENA_Msk (1UL << 24)
#endif

static uint32_t s_cycles_per_us = 0;
static uint8_t s_dwt_ready = 0;

static void Delay_DWT_Init(void)
{
    if (s_dwt_ready) {
        return;
    }

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    if (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) {
        s_cycles_per_us = SystemCoreClock / 1000000UL;
        if (s_cycles_per_us == 0) {
            s_cycles_per_us = 1;
        }
        s_dwt_ready = 1;
    }
}

void Delay_Init(void)
{
    Delay_DWT_Init();
}

void Delay_us(uint32_t nus)
{
    if (nus == 0) {
        return;
    }

    if (!s_dwt_ready) {
        Delay_DWT_Init();
    }

    if (s_dwt_ready) {
        const uint32_t start = DWT->CYCCNT;
        const uint32_t wait_cycles = (uint32_t)((uint64_t)s_cycles_per_us * nus);
        while ((uint32_t)(DWT->CYCCNT - start) < wait_cycles) {
        }
    } else {
        const uint32_t cycles_per_loop = SystemCoreClock / 1000000UL;
        uint64_t total = (uint64_t)cycles_per_loop * nus;
        while (total--) {
            __NOP();
        }
    }
}

void Delay_ms(uint32_t nms)
{
    if (nms == 0) {
        return;
    }

    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        vTaskDelay(pdMS_TO_TICKS(nms));
        return;
    }

    while (nms--) {
        Delay_us(1000UL);
    }
}

void Delay_s(uint32_t ns)
{
    while (ns--) {
        Delay_ms(1000UL);
    }
}
