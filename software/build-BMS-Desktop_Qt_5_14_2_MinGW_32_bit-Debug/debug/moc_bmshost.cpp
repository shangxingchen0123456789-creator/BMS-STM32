/****************************************************************************
** Meta object code from reading C++ file 'bmshost.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.14.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../BMS/bmshost.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'bmshost.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.14.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_BmsHost_t {
    QByteArrayData data[13];
    char stringdata0[118];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_BmsHost_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_BmsHost_t qt_meta_stringdata_BmsHost = {
    {
QT_MOC_LITERAL(0, 0, 7), // "BmsHost"
QT_MOC_LITERAL(1, 8, 11), // "ackReceived"
QT_MOC_LITERAL(2, 20, 0), // ""
QT_MOC_LITERAL(3, 21, 3), // "seq"
QT_MOC_LITERAL(4, 25, 3), // "err"
QT_MOC_LITERAL(5, 29, 17), // "telemetryReceived"
QT_MOC_LITERAL(6, 47, 19), // "BmsProto::Telemetry"
QT_MOC_LITERAL(7, 67, 1), // "t"
QT_MOC_LITERAL(8, 69, 10), // "ackTimeout"
QT_MOC_LITERAL(9, 80, 7), // "logLine"
QT_MOC_LITERAL(10, 88, 4), // "line"
QT_MOC_LITERAL(11, 93, 11), // "onReadyRead"
QT_MOC_LITERAL(12, 105, 12) // "onAckTimeout"

    },
    "BmsHost\0ackReceived\0\0seq\0err\0"
    "telemetryReceived\0BmsProto::Telemetry\0"
    "t\0ackTimeout\0logLine\0line\0onReadyRead\0"
    "onAckTimeout"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_BmsHost[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   44,    2, 0x06 /* Public */,
       5,    2,   49,    2, 0x06 /* Public */,
       8,    1,   54,    2, 0x06 /* Public */,
       9,    1,   57,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      11,    0,   60,    2, 0x08 /* Private */,
      12,    0,   61,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::UChar, QMetaType::UChar,    3,    4,
    QMetaType::Void, QMetaType::UChar, 0x80000000 | 6,    3,    7,
    QMetaType::Void, QMetaType::UChar,    3,
    QMetaType::Void, QMetaType::QString,   10,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void BmsHost::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<BmsHost *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->ackReceived((*reinterpret_cast< quint8(*)>(_a[1])),(*reinterpret_cast< quint8(*)>(_a[2]))); break;
        case 1: _t->telemetryReceived((*reinterpret_cast< quint8(*)>(_a[1])),(*reinterpret_cast< const BmsProto::Telemetry(*)>(_a[2]))); break;
        case 2: _t->ackTimeout((*reinterpret_cast< quint8(*)>(_a[1]))); break;
        case 3: _t->logLine((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->onReadyRead(); break;
        case 5: _t->onAckTimeout(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< BmsProto::Telemetry >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (BmsHost::*)(quint8 , quint8 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BmsHost::ackReceived)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (BmsHost::*)(quint8 , const BmsProto::Telemetry & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BmsHost::telemetryReceived)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (BmsHost::*)(quint8 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BmsHost::ackTimeout)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (BmsHost::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BmsHost::logLine)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject BmsHost::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_BmsHost.data,
    qt_meta_data_BmsHost,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *BmsHost::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BmsHost::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_BmsHost.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int BmsHost::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void BmsHost::ackReceived(quint8 _t1, quint8 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void BmsHost::telemetryReceived(quint8 _t1, const BmsProto::Telemetry & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void BmsHost::ackTimeout(quint8 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void BmsHost::logLine(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
