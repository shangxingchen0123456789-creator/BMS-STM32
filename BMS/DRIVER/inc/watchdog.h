#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <stdint.h>

void Watchdog_Init(uint32_t timeout_ms);
void Watchdog_Kick(void);
void Watchdog_ServiceInit(void);

#endif /* WATCHDOG_H */
