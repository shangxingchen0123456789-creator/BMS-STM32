#include "serialport.h"
#include <QSerialPortInfo>
#include <QDebug>

SerialPortHelper::SerialPortHelper(QObject *parent)
    : QObject(parent)
{
    // 连接串口的readyRead信号到我们的槽函数
    connect(&m_port, &QSerialPort::readyRead, this, &SerialPortHelper::onReadyRead);
}

bool SerialPortHelper::openPort(const QString& portName,int baud)
{
    // 如果串口已打开，先关闭
    if (m_port.isOpen()) {
        m_port.close();
    }

    // 设置串口参数
    m_port.setPortName(portName);
    m_port.setBaudRate(baud);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);

    // 尝试以读写模式打开串口
    bool openSuccess = m_port.open(QIODevice::ReadWrite);

    // 发送日志消息通知打开结果
    emit logMessage(openSuccess ?
                        QString("打开串口成功: %1").arg(portName):
                        QString("打开串口失败: %1").arg(portName));

    return openSuccess;
}

void SerialPortHelper::closePort()
{
    if (m_port.isOpen()) {
        m_port.close();
    }
    emit logMessage("串口已关闭");
}

QStringList SerialPortHelper::availablePorts()
{
    QStringList list;
    const auto infos = QSerialPortInfo::availablePorts();
    for (const auto &info : infos) {
        list << info.portName();
    }
    return list;
}

void SerialPortHelper::onReadyRead()
{
    const QByteArray data = m_port.readAll();
    if (!data.isEmpty()) {
        emit bytesReceived(data);
    }
}

