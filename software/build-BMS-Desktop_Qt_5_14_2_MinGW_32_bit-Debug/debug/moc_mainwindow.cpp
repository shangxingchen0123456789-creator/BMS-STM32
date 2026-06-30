/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.14.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../BMS/mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.14.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[24];
    char stringdata0[304];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 21), // "on_btnConnect_clicked"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 19), // "on_btnStart_clicked"
QT_MOC_LITERAL(4, 54, 18), // "on_btnStop_clicked"
QT_MOC_LITERAL(5, 73, 19), // "on_btnChgOn_clicked"
QT_MOC_LITERAL(6, 93, 20), // "on_btnChgOff_clicked"
QT_MOC_LITERAL(7, 114, 19), // "on_btnDsgOn_clicked"
QT_MOC_LITERAL(8, 134, 20), // "on_btnDsgOff_clicked"
QT_MOC_LITERAL(9, 155, 5), // "onAck"
QT_MOC_LITERAL(10, 161, 3), // "seq"
QT_MOC_LITERAL(11, 165, 3), // "err"
QT_MOC_LITERAL(12, 169, 11), // "onTelemetry"
QT_MOC_LITERAL(13, 181, 19), // "BmsProto::Telemetry"
QT_MOC_LITERAL(14, 201, 1), // "t"
QT_MOC_LITERAL(15, 203, 5), // "onLog"
QT_MOC_LITERAL(16, 209, 4), // "line"
QT_MOC_LITERAL(17, 214, 15), // "onPlotMouseMove"
QT_MOC_LITERAL(18, 230, 12), // "QMouseEvent*"
QT_MOC_LITERAL(19, 243, 5), // "event"
QT_MOC_LITERAL(20, 249, 12), // "onSimToggled"
QT_MOC_LITERAL(21, 262, 2), // "on"
QT_MOC_LITERAL(22, 265, 9), // "onSimTick"
QT_MOC_LITERAL(23, 275, 28) // "on_btnSendThresholds_clicked"

    },
    "MainWindow\0on_btnConnect_clicked\0\0"
    "on_btnStart_clicked\0on_btnStop_clicked\0"
    "on_btnChgOn_clicked\0on_btnChgOff_clicked\0"
    "on_btnDsgOn_clicked\0on_btnDsgOff_clicked\0"
    "onAck\0seq\0err\0onTelemetry\0BmsProto::Telemetry\0"
    "t\0onLog\0line\0onPlotMouseMove\0QMouseEvent*\0"
    "event\0onSimToggled\0on\0onSimTick\0"
    "on_btnSendThresholds_clicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   84,    2, 0x08 /* Private */,
       3,    0,   85,    2, 0x08 /* Private */,
       4,    0,   86,    2, 0x08 /* Private */,
       5,    0,   87,    2, 0x08 /* Private */,
       6,    0,   88,    2, 0x08 /* Private */,
       7,    0,   89,    2, 0x08 /* Private */,
       8,    0,   90,    2, 0x08 /* Private */,
       9,    2,   91,    2, 0x08 /* Private */,
      12,    2,   96,    2, 0x08 /* Private */,
      15,    1,  101,    2, 0x08 /* Private */,
      17,    1,  104,    2, 0x08 /* Private */,
      20,    1,  107,    2, 0x08 /* Private */,
      22,    0,  110,    2, 0x08 /* Private */,
      23,    0,  111,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::UChar, QMetaType::UChar,   10,   11,
    QMetaType::Void, QMetaType::UChar, 0x80000000 | 13,   10,   14,
    QMetaType::Void, QMetaType::QString,   16,
    QMetaType::Void, 0x80000000 | 18,   19,
    QMetaType::Void, QMetaType::Bool,   21,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->on_btnConnect_clicked(); break;
        case 1: _t->on_btnStart_clicked(); break;
        case 2: _t->on_btnStop_clicked(); break;
        case 3: _t->on_btnChgOn_clicked(); break;
        case 4: _t->on_btnChgOff_clicked(); break;
        case 5: _t->on_btnDsgOn_clicked(); break;
        case 6: _t->on_btnDsgOff_clicked(); break;
        case 7: _t->onAck((*reinterpret_cast< quint8(*)>(_a[1])),(*reinterpret_cast< quint8(*)>(_a[2]))); break;
        case 8: _t->onTelemetry((*reinterpret_cast< quint8(*)>(_a[1])),(*reinterpret_cast< const BmsProto::Telemetry(*)>(_a[2]))); break;
        case 9: _t->onLog((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 10: _t->onPlotMouseMove((*reinterpret_cast< QMouseEvent*(*)>(_a[1]))); break;
        case 11: _t->onSimToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 12: _t->onSimTick(); break;
        case 13: _t->on_btnSendThresholds_clicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 8:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< BmsProto::Telemetry >(); break;
            }
            break;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
