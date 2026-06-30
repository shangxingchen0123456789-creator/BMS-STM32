#ifndef SERIALPORTHELPER_H
#define SERIALPORTHELPER_H

#include <QObject>
#include <QSerialPort>
#include <QByteArray>


class SerialPortHelper : public QObject
{
    Q_OBJECT

public:
    explicit SerialPortHelper(QObject* parent = nullptr);
    virtual ~SerialPortHelper() = default;

    // 串口操作
    bool openPort(const QString& portName,int baud);
    void closePort();
    bool isOpen() const { return m_port.isOpen(); }
    static QStringList availablePorts();

    // 发送数据
    qint64 write(const QByteArray& data) { return m_port.write(data); }



protected:
    QSerialPort m_port;

signals:
    void logMessage(const QString &msg);       // 日志信息
    void bytesReceived(const QByteArray& data);

private slots:
    void onReadyRead();


};

#endif // SERIALPORTHELPER_H
