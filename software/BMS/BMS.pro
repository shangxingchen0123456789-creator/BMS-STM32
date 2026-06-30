QT       += core gui serialport printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
DEFINES += QCUSTOMPLOT_COMPILE_LIBRARY

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    bmshost.cpp \
    main.cpp \
    mainwindow.cpp \
    bmsprotocol.cpp \
    qcustomplot.cpp \
    batterywidget.cpp \
    ledindicator.cpp

HEADERS += \
    bmshost.h \
    mainwindow.h \
    bmsprotocol.h \
    qcustomplot.h \
    batterywidget.h \
    ledindicator.h

FORMS += \
    mainwindow.ui

RESOURCES += \
    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
