#ifndef   _DELAY_H
#define   _DELAY_H

#include "stdint.h"


/* 延时参数定义 */
#define DELAY_US_MAX  0xFFFFFF  // 最大微秒延时（受SysTick 24位限制）
#define DELAY_MS_MAX  1800      // 最大毫秒延时（安全值）

void Delay_Init(void);
void Delay_us(uint32_t nus);
void Delay_ms(uint32_t nms);
void Delay_s(uint32_t ns);

#endif
