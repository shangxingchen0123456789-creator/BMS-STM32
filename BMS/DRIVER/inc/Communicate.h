#ifndef COMMUNICATE_H
#define COMMUNICATE_H

#include "stm32f10x.h"
#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* Command identifiers used by the host protocol                              */
/* -------------------------------------------------------------------------- */
enum {
    CMD_PING           = 0x01,
    CMD_STREAM_START   = 0x02,
    CMD_STREAM_STOP    = 0x03,

    CMD_CHG_ON         = 0x10,
    CMD_CHG_OFF        = 0x11,
    CMD_DSG_ON         = 0x12,
    CMD_DSG_OFF        = 0x13,

    CMD_READ_ONCE      = 0x20,
    CMD_SET_THRESHOLDS = 0x30,

    CMD_ACK            = 0x81,
    CMD_TELEMETRY      = 0x90,
    CMD_STATUS_RSP     = 0x93,
    CMD_VERSION_RSP    = 0x98
};

/* -------------------------------------------------------------------------- */
/* Error codes placed inside ACK payloads                                     */
/* -------------------------------------------------------------------------- */
typedef enum {
    COMM_OK            = 0x00,
    COMM_ERR_UNKNOWN   = 0x01,
    COMM_ERR_BAD_LEN   = 0x02,
    COMM_ERR_PARAM     = 0x03,
    COMM_ERR_UNSUPPORT = 0x04
} comm_err_t;

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */
void Comm_Init(void);
void Comm_PeriodicTask(void);
void Comm_RxCallback(uint8_t byte);
void Comm_On1msTick(void);
uint32_t get_tick(void);

uint8_t Comm_IsStreaming(void);
void Comm_SetStreamInterval(uint16_t interval_ms);
uint32_t Comm_GetRxOverflowCount(void);

void SendFrame(uint8_t cmd, uint8_t seq, const uint8_t *payload, uint16_t plen);

#endif /* COMMUNICATE_H */
