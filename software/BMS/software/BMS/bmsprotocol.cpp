#include "bmsprotocol.h"
#include <stdexcept>
#include <QDebug>

namespace BmsProto {

quint16 crc16IBM(const quint8* data, int len)
{
    quint16 crc = 0xFFFF;
    for (int i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

QByteArray makeFrame(quint8 cmd, quint8 seq, const QByteArray& payload)
{
    quint16 plen = static_cast<quint16>(payload.size());

    if (plen > 100) {
        qWarning() << "Payload too large:" << plen;
        return QByteArray();
    }

    QByteArray out;
    out.reserve(8 + plen);

    out.append(static_cast<char>(MAGIC0));
    out.append(static_cast<char>(MAGIC1));
    out.append(static_cast<char>(cmd));
    out.append(static_cast<char>(seq));
    out.append(static_cast<char>(plen & 0xFF));
    out.append(static_cast<char>((plen >> 8) & 0xFF));
    out.append(payload);

    const quint8* crcStart = reinterpret_cast<const quint8*>(out.constData() + 2);
    const int crcLen = 4 + plen;
    quint16 crc = crc16IBM(crcStart, crcLen);

    out.append(static_cast<char>(crc & 0xFF));
    out.append(static_cast<char>((crc >> 8) & 0xFF));

    qDebug() << "makeFrame: CMD=" << Qt::hex << cmd
             << "SEQ=" << seq
             << "LEN=" << Qt::dec << plen
             << "CRC=" << Qt::hex << crc;

    return out;
}

static inline qint16 rdS16LE(const quint8* p)
{
    return static_cast<qint16>(p[0] | (static_cast<quint16>(p[1]) << 8));
}

static inline quint16 rdU16LE(const quint8* p)
{
    return p[0] | (static_cast<quint16>(p[1]) << 8);
}

static inline quint32 rdU32LE(const quint8* p)
{
    return static_cast<quint32>(p[0]) |
           (static_cast<quint32>(p[1]) << 8) |
           (static_cast<quint32>(p[2]) << 16) |
           (static_cast<quint32>(p[3]) << 24);
}

Telemetry parseTelemetry(const QByteArray& payload)
{
    qDebug() << "parseTelemetry: payload size =" << payload.size();

    constexpr int kFixedBytes = 4  /* ctrl flags */
                                + 2 /* pack */
                                + 2 /* current */
                                + 6 /* temps */
                                + 6 /* cycle/capacity/remaining */
                                + 2 /* alerts */
                                + 1 /* state */
                                + 4 /* timestamp */;

    if (payload.size() < kFixedBytes) {
        qWarning() << "Telemetry payload too short:" << payload.size();
        throw std::runtime_error("Telemetry payload too short");
    }

    Telemetry t;
    const quint8* p = reinterpret_cast<const quint8*>(payload.constData());
    int idx = 0;

    t.chg_enable = p[idx++];
    t.dsg_enable = p[idx++];
    t.soc = p[idx++];
    t.soh = p[idx++];

    if (idx + 2 > payload.size()) throw std::runtime_error("Telemetry truncated (pack_voltage)");
    t.pack_voltage = rdU16LE(p + idx); idx += 2;

    if (idx + 2 > payload.size()) throw std::runtime_error("Telemetry truncated (current)");
    t.current = rdS16LE(p + idx); idx += 2;

    if (idx + 6 > payload.size()) throw std::runtime_error("Telemetry truncated (temps)");
    t.t0_x10 = rdS16LE(p + idx); idx += 2;
    t.t1_x10 = rdS16LE(p + idx); idx += 2;
    t.t2_x10 = rdS16LE(p + idx); idx += 2;

    if (idx + 6 > payload.size()) throw std::runtime_error("Telemetry truncated (capacity block)");
    t.cycle_count = rdU16LE(p + idx); idx += 2;
    t.learned_capacity_10mAh = rdU16LE(p + idx); idx += 2;
    t.remaining_capacity_10mAh = rdU16LE(p + idx); idx += 2;

    int bytesLeft = payload.size() - idx;
    if (bytesLeft < 7) {
        qWarning() << "Telemetry payload missing tail fields:" << payload.size();
        throw std::runtime_error("Telemetry payload missing tail fields");
    }

    int cellBytes = bytesLeft - 7; // subtract alert(2) + state(1) + timestamp(4)
    if (cellBytes < 0 || (cellBytes % 2) != 0) {
        qWarning() << "Telemetry cell section invalid: bytes=" << cellBytes;
        throw std::runtime_error("Telemetry cell section invalid");
    }

    int cellCount = cellBytes / 2;
    t.cell_mv.reserve(cellCount);
    for (int i = 0; i < cellCount; ++i) {
        if (idx + 2 > payload.size()) {
            throw std::runtime_error("Telemetry truncated while reading cells");
        }
        t.cell_mv.push_back(rdS16LE(p + idx));
        idx += 2;
    }

    if (idx + 7 > payload.size()) {
        throw std::runtime_error("Telemetry truncated (tail)");
    }

    t.alerts = rdU16LE(p + idx); idx += 2;
    t.state = p[idx++];
    t.timestamp_ms = rdU32LE(p + idx); idx += 4;

    qDebug() << "Telemetry parsed: SOC=" << t.soc
             << "SOH=" << t.soh
             << "Pack=" << t.pack_voltage << "mV"
             << "Current=" << t.current << "mA"
             << "Cells=" << cellCount
             << "Alerts=0x" << Qt::hex << t.alerts
             << "State=" << t.state
             << "Timestamp=" << Qt::dec << t.timestamp_ms;

    return t;
}

} // namespace BmsProto
