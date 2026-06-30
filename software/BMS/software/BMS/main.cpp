#include "mainwindow.h"

#include <QApplication>
#include <QMetaType>
#include <QFile>
#include "bmsprotocol.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<BmsProto::Telemetry>("BmsProto::Telemetry");

    // 加载深色主题样式表
    QFile qss(":/styles/bms_dark.qss");
    if (qss.open(QFile::ReadOnly | QFile::Text)) {
        a.setStyleSheet(qss.readAll());
        qss.close();
    }

    MainWindow w;
    w.show();
    return a.exec();
}
