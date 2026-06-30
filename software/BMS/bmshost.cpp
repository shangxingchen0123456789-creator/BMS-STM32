#include "bmshost.h"
#include <QDebug>
#include <QThread>
BmsHost::BmsHost(QObject* parent) : QObject(parent)
{
    connect(&m_port, &QSerialPort::readyRead, this, &BmsHost::onReadyRead);

    // ACK 超时定时器
    m_ackTimer.setSingleShot(true);
    connect(&m_ackTimer, &QTimer::timeout, this, &BmsHost::onAckTimeout);
}

BmsHost::~BmsHost() { close(); }

bool BmsHost::open(const QString& portName, int baud)
{
    if (m_port.isOpen()) m_port.close();

    m_port.setPortName(portName);
    m_port.setBaudRate(baud);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    bool ok = m_port.open(QIODevice::ReadWrite);
    if (ok) {
        // ✅ 关键修复：清空缓冲区
        m_port.clear(QSerialPort::AllDirections);  // 清空硬件缓冲区
        QThread::msleep(50);  // 等待稳定
        m_port.readAll();  // 清空软件缓冲区

        emit logLine(QString("[PORT] Open %1 @ %2").arg(portName).arg(baud));

        // ✅ 清空接收缓冲区和序列号
        m_rx.clear();
        m_seq = 0;

        // 延迟一点再发送 PING
        QThread::msleep(100);
        ping();
    } else {
        emit logLine(QString("[PORT] Open failed: %1").arg(m_port.errorString()));
    }

    return ok;
}

void BmsHost::close()
{
    if (m_port.isOpen()) {
        m_ackTimer.stop();
        m_port.close();
        emit logLine("[PORT] Closed");
    }
}

quint8 BmsHost::nextSeq()
{
    quint8 s = m_seq;
    m_seq = quint8((m_seq + 1) & 0xFF);
    return s;
}

void BmsHost::startAckTimer(quint8 seq)
{
    m_pendingAckSeq = seq;
    m_ackTimer.start(ACK_TIMEOUT_MS);
}

void BmsHost::onAckTimeout()
{
    emit logLine(QString::fromUtf8("[TIMEOUT] 等待 ACK seq=%1 超时 (%2ms)无响应，请检查连接")
                     .arg(m_pendingAckSeq).arg(ACK_TIMEOUT_MS));
    emit ackTimeout(m_pendingAckSeq);
}

// 命令发送函数 - 添加调试输出
quint8 BmsHost::ping()
{
    auto s = nextSeq();
    QByteArray frame = BmsProto::makeFrame(BmsProto::CMD_PING, s, QByteArray());

    // 调试: 打印发送的十六进制
    QString hex;
    for (int i = 0; i < frame.size(); i++) {
        hex += QString("%1 ").arg(static_cast<quint8>(frame[i]), 2, 16, QChar('0')).toUpper();
    }
    emit logLine(QString("[TX] PING seq=%1: %2").arg(s).arg(hex));

    m_port.write(frame);
    startAckTimer(s);
    return s;
}

quint8 BmsHost::streamStart(quint16 interval_ms)
{
    auto s = nextSeq();

    // 打包 interval_ms 为 2 字节小端 payload
    QByteArray payload;
    payload.append(static_cast<char>(interval_ms & 0xFF));
    payload.append(static_cast<char>((interval_ms >> 8) & 0xFF));

    QByteArray frame = BmsProto::makeFrame(BmsProto::CMD_STREAM_START, s, payload);

    QString hex;
    for (int i = 0; i < frame.size(); i++) {
        hex += QString("%1 ").arg(static_cast<quint8>(frame[i]), 2, 16, QChar('0')).toUpper();
    }
    emit logLine(QString("[TX] STREAM_START seq=%1 interval=%2ms: %3").arg(s).arg(interval_ms).arg(hex));

    m_port.write(frame);
    startAckTimer(s);
    return s;
}

quint8 BmsHost::streamStop()
{
    auto s = nextSeq();
    QByteArray frame = BmsProto::makeFrame(BmsProto::CMD_STREAM_STOP, s, QByteArray());

    QString hex;
    for (int i = 0; i < frame.size(); i++) {
        hex += QString("%1 ").arg(static_cast<quint8>(frame[i]), 2, 16, QChar('0')).toUpper();
    }
    emit logLine(QString("[TX] STREAM_STOP seq=%1: %2").arg(s).arg(hex));

    m_port.write(frame);
    startAckTimer(s);
    return s;
}

quint8 BmsHost::chgOn()
{
    auto s = nextSeq();
    QByteArray frame = BmsProto::makeFrame(BmsProto::CMD_CHG_ON, s, QByteArray());

    QString hex;
    for (int i = 0; i < frame.size(); i++) {
        hex += QString("%1 ").arg(static_cast<quint8>(frame[i]), 2, 16, QChar('0')).toUpper();
    }
    emit logLine(QString("[TX] CHG_ON seq=%1: %2").arg(s).arg(hex));

    m_port.write(frame);
    startAckTimer(s);
    return s;
}

quint8 BmsHost::chgOff()
{
    auto s = nextSeq();
    QByteArray frame = BmsProto::makeFrame(BmsProto::CMD_CHG_OFF, s, QByteArray());

    QString hex;
    for (int i = 0; i < frame.size(); i++) {
        hex += QString("%1 ").arg(static_cast<quint8>(frame[i]), 2, 16, QChar('0')).toUpper();
    }
    emit logLine(QString("[TX] CHG_OFF seq=%1: %2").arg(s).arg(hex));

    m_port.write(frame);
    startAckTimer(s);
    return s;
}

quint8 BmsHost::dsgOn()
{
    auto s = nextSeq();
    QByteArray frame = BmsProto::makeFrame(BmsProto::CMD_DSG_ON, s, QByteArray());

    QString hex;
    for (int i = 0; i < frame.size(); i++) {
        hex += QString("%1 ").arg(static_cast<quint8>(frame[i]), 2, 16, QChar('0')).toUpper();
    }
    emit logLine(QString("[TX] DSG_ON seq=%1: %2").arg(s).arg(hex));

    m_port.write(frame);
    startAckTimer(s);
    return s;
}

quint8 BmsHost::dsgOff()
{
    auto s = nextSeq();
    QByteArray frame = BmsProto::makeFrame(BmsProto::CMD_DSG_OFF, s, QByteArray());

    QString hex;
    for (int i = 0; i < frame.size(); i++) {
        hex += QString("%1 ").arg(static_cast<quint8>(frame[i]), 2, 16, QChar('0')).toUpper();
    }
    emit logLine(QString("[TX] DSG_OFF seq=%1: %2").arg(s).arg(hex));

    m_port.write(frame);
    startAckTimer(s);
    return s;
}

quint8 BmsHost::readOnce()
{
    auto s = nextSeq();
    QByteArray frame = BmsProto::makeFrame(BmsProto::CMD_READ_ONCE, s, QByteArray());

    QString hex;
    for (int i = 0; i < frame.size(); i++) {
        hex += QString("%1 ").arg(static_cast<quint8>(frame[i]), 2, 16, QChar('0')).toUpper();
    }
    emit logLine(QString("[TX] READ_ONCE seq=%1: %2").arg(s).arg(hex));

    m_port.write(frame);
    startAckTimer(s);  // MCU 回 CMD_TELEMETRY（无 ACK），dispatch 中也会停定时器
    return s;
}

quint8 BmsHost::sendThresholds(quint16 cellOvMv, quint16 cellOvRelease,
                               quint16 cellUvMv, quint16 cellUvRelease,
                               quint32 ocdMa,
                               qint16 chgOtDc, qint16 chgUtDc,
                               qint16 dsgOtDc, qint16 dsgUtDc)
{
    auto s = nextSeq();

    // 组装 20 字节小端 payload
    QByteArray payload;
    payload.reserve(20);

    auto wrU16 = [&](quint16 v) {
        payload.append(static_cast<char>(v & 0xFF));
        payload.append(static_cast<char>((v >> 8) & 0xFF));
    };
    auto wrS16 = [&](qint16 v) { wrU16(static_cast<quint16>(v)); };
    auto wrU32 = [&](quint32 v) {
        payload.append(static_cast<char>(v & 0xFF));
        payload.append(static_cast<char>((v >> 8) & 0xFF));
        payload.append(static_cast<char>((v >> 16) & 0xFF));
        payload.append(static_cast<char>((v >> 24) & 0xFF));
    };

    wrU16(cellOvMv);
    wrU16(cellOvRelease);
    wrU16(cellUvMv);
    wrU16(cellUvRelease);
    wrU32(ocdMa);
    wrS16(chgOtDc);
    wrS16(chgUtDc);
    wrS16(dsgOtDc);
    wrS16(dsgUtDc);

    QByteArray frame = BmsProto::makeFrame(BmsProto::CMD_SET_THRESHOLDS, s, payload);

    QString hex;
    for (int i = 0; i < frame.size(); i++) {
        hex += QString("%1 ").arg(static_cast<quint8>(frame[i]), 2, 16, QChar('0')).toUpper();
    }
    emit logLine(QString("[TX] SET_THRESHOLDS seq=%1: %2").arg(s).arg(hex));

    m_port.write(frame);
    startAckTimer(s);
    return s;
}

void BmsHost::onReadyRead()
{
    QByteArray newData = m_port.readAll();

    // 调试: 打印接收的原始数据
    if (!newData.isEmpty()) {
        QString hex;
        for (int i = 0; i < newData.size(); i++) {
            hex += QString("%1 ").arg(static_cast<quint8>(newData[i]), 2, 16, QChar('0')).toUpper();
        }
        emit logLine(QString("[RX] Raw: %1").arg(hex));
    }

    m_rx.append(newData);
    parseRxBuf();
}

void BmsHost::parseRxBuf()
{
    while (m_rx.size() >= 8) {
        // ✅ 每次循环重新获取指针
        const quint8* buf = reinterpret_cast<const quint8*>(m_rx.constData());

        // ✅ 打印缓冲区前16字节用于调试
        QString bufHex;
        for(int i = 0; i < qMin(16, m_rx.size()); i++) {
            bufHex += QString("%1 ").arg(buf[i], 2, 16, QChar('0')).toUpper();
        }
        emit logLine(QString("[PARSE] Buffer(%1): %2").arg(m_rx.size()).arg(bufHex));

        // 检查帧头
        if (!(buf[0] == BmsProto::MAGIC0 && buf[1] == BmsProto::MAGIC1)) {
            emit logLine(QString("[SYNC] Skip byte: 0x%1").arg(buf[0], 2, 16, QChar('0')));
            m_rx.remove(0, 1);
            continue;  // ✅ 继续下一次循环
        }

        // 读取长度
        quint16 plen = buf[4] | (quint16(buf[5]) << 8);
        quint16 frame_len = quint16(8 + plen);

        emit logLine(QString("[PARSE] Magic OK! CMD=0x%1 SEQ=%2 PLEN=%3 FrameLen=%4")
                         .arg(buf[2], 2, 16, QChar('0'))
                         .arg(buf[3])
                         .arg(plen)
                         .arg(frame_len));

        // 合法性检查
        if (frame_len > 128 || plen > 100) {
            emit logLine(QString("[ERR] Invalid frame length: %1").arg(frame_len));
            m_rx.remove(0, 1);
            continue;
        }

        // 等待完整帧
        if (m_rx.size() < frame_len) {
            emit logLine(QString("[WAIT] Need %1 bytes, have %2 - waiting...").arg(frame_len).arg(m_rx.size()));
            return;  // ✅ 等待更多数据
        }

        // 提取字段
        quint8 cmd = buf[2];
        quint8 seq = buf[3];
        QByteArray payload = m_rx.mid(6, plen);
        quint16 crc_recv = buf[6 + plen] | (quint16(buf[7 + plen]) << 8);

        // CRC 验证
        const quint8* crcStart = buf + 2;
        int crcLen = 4 + plen;
        quint16 crc_calc = BmsProto::crc16IBM(crcStart, crcLen);

        // ✅ 打印 CRC 计算区域
        QString crcHex;
        for(int i = 0; i < crcLen && i < 20; i++) {
            crcHex += QString("%1 ").arg(crcStart[i], 2, 16, QChar('0')).toUpper();
        }
        emit logLine(QString("[CRC] Data: %1").arg(crcHex));
        emit logLine(QString("[CRC] Recv=0x%1 Calc=0x%2 %3")
                         .arg(crc_recv, 4, 16, QChar('0'))
                         .arg(crc_calc, 4, 16, QChar('0'))
                         .arg(crc_recv == crc_calc ? "✓ PASS" : "✗ FAIL"));

        if (crc_recv == crc_calc) {
            emit logLine(QString("[OK] Dispatching CMD=0x%1 SEQ=%2")
                             .arg(cmd, 2, 16, QChar('0'))
                             .arg(seq));

            dispatch(cmd, seq, payload);
            m_rx.remove(0, frame_len);
        } else {
            emit logLine(QString("[FAIL] CRC mismatch, skipping 1 byte"));
            m_rx.remove(0, 1);
        }
    }
}

void BmsHost::dispatch(quint8 cmd, quint8 seq, const QByteArray& payload)
{
    if (cmd == BmsProto::CMD_ACK) {
        // 收到 ACK，停止超时定时器
        m_ackTimer.stop();
        quint8 err = payload.isEmpty() ? 0xFF : quint8(payload[0]);
        emit ackReceived(seq, err);

        QString errStr;
        switch(err) {
        case 0x00: errStr = "OK"; break;
        case 0x01: errStr = "UNKNOWN"; break;
        case 0x02: errStr = "BAD_LEN"; break;
        case 0x03: errStr = "PARAM"; break;
        case 0x04: errStr = "UNSUPPORT"; break;
        default:   errStr = "INVALID"; break;
        }
        emit logLine(QString("[ACK] seq=%1 err=%2 (%3)")
                         .arg(seq)
                         .arg(err, 2, 16, QChar('0'))
                         .arg(errStr));

    } else if (cmd == BmsProto::CMD_TELEMETRY) {
        // 收到遥测数据，也停止超时定时器（READ_ONCE 只回 TELEMETRY 无 ACK）
        m_ackTimer.stop();
        try {
            auto t = BmsProto::parseTelemetry(payload);
            emit telemetryReceived(seq, t);

            // 详细日志
            QString cellStr;
            for (int i = 0; i < t.cell_mv.size(); i++) {
                if (i > 0) cellStr += ", ";
                if (BmsProto::isInvalidCell(t.cell_mv[i])) {
                    cellStr += "--";
                } else {
                    cellStr += QString::number(t.cell_mv[i]);
                }
            }

            const double capAh = t.learned_capacity_10mAh / 100.0;
            const double remAh = t.remaining_capacity_10mAh / 100.0;

            emit logLine(QString("[TLV] seq=%1 SOC=%2%% SOH=%3%% Pack=%4mV I=%5mA Cycle=%6 Cap=%7Ah Rem=%8Ah Cells=[%9] Alerts=0x%10 State=%11 Time=%12ms")
                             .arg(seq)
                             .arg(t.soc)
                             .arg(t.soh)
                             .arg(t.pack_voltage)
                             .arg(t.current)
                             .arg(t.cycle_count)
                             .arg(QString::number(capAh, 'f', 2))
                             .arg(QString::number(remAh, 'f', 2))
                             .arg(cellStr)
                             .arg(t.alerts, 4, 16, QChar('0'))
                             .arg(t.state)
                             .arg(t.timestamp_ms));

        } catch (const std::exception& e) {
            emit logLine(QString("[TLV] Parse error: %1").arg(e.what()));
        }
    } else {
        emit logLine(QString("[RX] Unknown cmd=0x%1 seq=%2 len=%3")
                         .arg(cmd, 2, 16, QChar('0'))
                         .arg(seq)
                         .arg(payload.size()));
    }
}
