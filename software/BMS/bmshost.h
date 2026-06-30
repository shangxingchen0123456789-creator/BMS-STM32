#pragma once
#include <QObject>
#include <QSerialPort>
#include <QTimer>
#include "bmsprotocol.h"

class BmsHost : public QObject
{
    Q_OBJECT
public:
    explicit BmsHost(QObject* parent=nullptr);
    ~BmsHost();

    bool open(const QString& portName, int baud=115200);
    void close();
    bool isOpen() const { return m_port.isOpen(); }

    // 命令封装
    quint8 ping();
    quint8 streamStart(quint16 interval_ms = 500);
    quint8 streamStop();
    quint8 chgOn();
    quint8 chgOff();
    quint8 dsgOn();
    quint8 dsgOff();
    quint8 readOnce();
    quint8 sendThresholds(quint16 cellOvMv, quint16 cellOvRelease,
                          quint16 cellUvMv, quint16 cellUvRelease,
                          quint32 ocdMa,
                          qint16 chgOtDc, qint16 chgUtDc,
                          qint16 dsgOtDc, qint16 dsgUtDc);

signals:
    void ackReceived(quint8 seq, quint8 err);
    void telemetryReceived(quint8 seq, const BmsProto::Telemetry& t);
    void ackTimeout(quint8 seq);
    void logLine(const QString& line);

private slots:
    void onReadyRead();
    void onAckTimeout();

private:
    void parseRxBuf();
    void dispatch(quint8 cmd, quint8 seq, const QByteArray& payload);
    quint8 nextSeq();
    void startAckTimer(quint8 seq);

    QSerialPort m_port;
    QByteArray  m_rx;
    quint8      m_seq = 0;

    // ACK 超时检测
    QTimer      m_ackTimer;
    quint8      m_pendingAckSeq = 0;
    static constexpr int ACK_TIMEOUT_MS = 3000;
};
