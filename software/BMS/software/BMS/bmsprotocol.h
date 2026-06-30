#pragma once
#include <QtGlobal>
#include <QByteArray>
#include <QVector>
#include <QMetaType>

namespace BmsProto {

// Command identifiers (match Communicate.h on the MCU)
enum : quint8 {
    CMD_PING          = 0x01,
    CMD_STREAM_START  = 0x02,
    CMD_STREAM_STOP   = 0x03,

    CMD_CHG_ON        = 0x10,
    CMD_CHG_OFF       = 0x11,
    CMD_DSG_ON        = 0x12,
    CMD_DSG_OFF       = 0x13,

    CMD_READ_ONCE     = 0x20,
    CMD_SET_THRESHOLDS = 0x30,
    CMD_ACK           = 0x81,
    CMD_TELEMETRY     = 0x90
};

enum : quint8 {
    COMM_OK = 0x00
};

static const quint8 MAGIC0 = 0x55;
static const quint8 MAGIC1 = 0xAA;

// CRC16-IBM (Modbus)
quint16 crc16IBM(const quint8* data, int len);

// Frame helper
QByteArray makeFrame(quint8 cmd, quint8 seq, const QByteArray& payload);

// Telemetry data container
struct Telemetry {
    quint8  chg_enable = 0;
    quint8  dsg_enable = 0;
    quint8  soc = 0;
    quint8  soh = 0;

    quint16 pack_voltage = 0;          // mV; 0xFFFF means invalid
    qint16  current      = -32768;     // mA; 0x8000 means invalid
    qint16  t0_x10       = -32768;     // 0.1 deg C
    qint16  t1_x10       = -32768;
    qint16  t2_x10       = -32768;

    quint16 cycle_count = 0;
    quint16 learned_capacity_10mAh = 0;
    quint16 remaining_capacity_10mAh = 0;

    QVector<qint16> cell_mv;           // per-cell voltages in mV (negative => invalid)

    quint16 alerts = 0;                // fault/alert bitmask
    quint8  state = 0;                 // run-state enum from firmware
    quint32 timestamp_ms = 0;          // firmware tick timestamp
};

Telemetry parseTelemetry(const QByteArray& payload);

inline bool isInvalid(qint16 v)  { return v == qint16(0x8000); }
inline bool isInvalid(quint16 v) { return v == 0xFFFFu; }
inline bool isInvalidCell(qint16 v) { return v <= 0; }

} // namespace BmsProto

Q_DECLARE_METATYPE(BmsProto::Telemetry)
