/****************************************************************************
** Meta object code from reading C++ file 'device.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../vc/device.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'device.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Device_t {
    QByteArrayData data[107];
    char stringdata0[1477];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Device_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Device_t qt_meta_stringdata_Device = {
    {
QT_MOC_LITERAL(0, 0, 6), // "Device"
QT_MOC_LITERAL(1, 7, 15), // "initializeCache"
QT_MOC_LITERAL(2, 23, 0), // ""
QT_MOC_LITERAL(3, 24, 3), // "get"
QT_MOC_LITERAL(4, 28, 3), // "key"
QT_MOC_LITERAL(5, 32, 8), // "forceGet"
QT_MOC_LITERAL(6, 41, 3), // "put"
QT_MOC_LITERAL(7, 45, 5), // "value"
QT_MOC_LITERAL(8, 51, 5), // "start"
QT_MOC_LITERAL(9, 57, 6), // "enable"
QT_MOC_LITERAL(10, 64, 7), // "disable"
QT_MOC_LITERAL(11, 72, 9), // "isStarted"
QT_MOC_LITERAL(12, 82, 9), // "isEnabled"
QT_MOC_LITERAL(13, 92, 5), // "begin"
QT_MOC_LITERAL(14, 98, 3), // "end"
QT_MOC_LITERAL(15, 102, 26), // "setAcceptorChannelProperty"
QT_MOC_LITERAL(16, 129, 9), // "Acceptors"
QT_MOC_LITERAL(17, 139, 8), // "acceptor"
QT_MOC_LITERAL(18, 148, 16), // "AcceptorChannels"
QT_MOC_LITERAL(19, 165, 7), // "channel"
QT_MOC_LITERAL(20, 173, 25), // "AcceptorChannelProperties"
QT_MOC_LITERAL(21, 199, 8), // "property"
QT_MOC_LITERAL(22, 208, 33), // "setAcceptorChannelPropertyByV..."
QT_MOC_LITERAL(23, 242, 12), // "channelValue"
QT_MOC_LITERAL(24, 255, 26), // "getAcceptorChannelProperty"
QT_MOC_LITERAL(25, 282, 33), // "getAcceptorChannelPropertyByV..."
QT_MOC_LITERAL(26, 316, 29), // "getAcceptorChannelPropertyKey"
QT_MOC_LITERAL(27, 346, 20), // "getDispenserProperty"
QT_MOC_LITERAL(28, 367, 10), // "Dispensers"
QT_MOC_LITERAL(29, 378, 9), // "dispenser"
QT_MOC_LITERAL(30, 388, 19), // "DispenserProperties"
QT_MOC_LITERAL(31, 408, 27), // "getDispenserPropertyByValue"
QT_MOC_LITERAL(32, 436, 23), // "getDispenserPropertyKey"
QT_MOC_LITERAL(33, 460, 17), // "getDeviceProperty"
QT_MOC_LITERAL(34, 478, 16), // "DeviceProperties"
QT_MOC_LITERAL(35, 495, 20), // "getDevicePropertyKey"
QT_MOC_LITERAL(36, 516, 18), // "requestCalibration"
QT_MOC_LITERAL(37, 535, 7), // "coinVal"
QT_MOC_LITERAL(38, 543, 7), // "noCoins"
QT_MOC_LITERAL(39, 551, 20), // "requestReleaseCredit"
QT_MOC_LITERAL(40, 572, 6), // "amount"
QT_MOC_LITERAL(41, 579, 9), // "dumpBills"
QT_MOC_LITERAL(42, 589, 12), // "initLogEntry"
QT_MOC_LITERAL(43, 602, 4), // "time"
QT_MOC_LITERAL(44, 607, 14), // "updateLogEntry"
QT_MOC_LITERAL(45, 622, 4), // "stop"
QT_MOC_LITERAL(46, 627, 6), // "status"
QT_MOC_LITERAL(47, 634, 11), // "getLogEntry"
QT_MOC_LITERAL(48, 646, 3), // "idx"
QT_MOC_LITERAL(49, 650, 12), // "CoinAcceptor"
QT_MOC_LITERAL(50, 663, 12), // "BillAcceptor"
QT_MOC_LITERAL(51, 676, 15), // "UnknownAcceptor"
QT_MOC_LITERAL(52, 692, 11), // "ChannelNone"
QT_MOC_LITERAL(53, 704, 8), // "Channel1"
QT_MOC_LITERAL(54, 713, 8), // "Channel2"
QT_MOC_LITERAL(55, 722, 8), // "Channel3"
QT_MOC_LITERAL(56, 731, 8), // "Channel4"
QT_MOC_LITERAL(57, 740, 8), // "Channel5"
QT_MOC_LITERAL(58, 749, 8), // "Channel6"
QT_MOC_LITERAL(59, 758, 8), // "Channel7"
QT_MOC_LITERAL(60, 767, 8), // "Channel8"
QT_MOC_LITERAL(61, 776, 8), // "Channel9"
QT_MOC_LITERAL(62, 785, 9), // "Channel10"
QT_MOC_LITERAL(63, 795, 9), // "Channel11"
QT_MOC_LITERAL(64, 805, 9), // "Channel12"
QT_MOC_LITERAL(65, 815, 9), // "Channel13"
QT_MOC_LITERAL(66, 825, 9), // "Channel14"
QT_MOC_LITERAL(67, 835, 9), // "Channel15"
QT_MOC_LITERAL(68, 845, 9), // "Channel16"
QT_MOC_LITERAL(69, 855, 10), // "StatusProp"
QT_MOC_LITERAL(70, 866, 15), // "UnitAddressProp"
QT_MOC_LITERAL(71, 882, 9), // "ValueProp"
QT_MOC_LITERAL(72, 892, 12), // "CurrencyProp"
QT_MOC_LITERAL(73, 905, 11), // "InhibitProp"
QT_MOC_LITERAL(74, 917, 8), // "PathProp"
QT_MOC_LITERAL(75, 926, 15), // "DefaultPathProp"
QT_MOC_LITERAL(76, 942, 9), // "CountProp"
QT_MOC_LITERAL(77, 952, 13), // "PathCountProp"
QT_MOC_LITERAL(78, 966, 19), // "PathSwitchLevelProp"
QT_MOC_LITERAL(79, 986, 15), // "FirstCoinHopper"
QT_MOC_LITERAL(80, 1002, 16), // "SecondCoinHopper"
QT_MOC_LITERAL(81, 1019, 13), // "BillDispenser"
QT_MOC_LITERAL(82, 1033, 16), // "UnknownDispenser"
QT_MOC_LITERAL(83, 1050, 11), // "StatusDProp"
QT_MOC_LITERAL(84, 1062, 16), // "UnitAddressDProp"
QT_MOC_LITERAL(85, 1079, 10), // "ValueDProp"
QT_MOC_LITERAL(86, 1090, 18), // "DispenseCountDProp"
QT_MOC_LITERAL(87, 1109, 14), // "CoinCountDProp"
QT_MOC_LITERAL(88, 1124, 18), // "CoinCountStatDProp"
QT_MOC_LITERAL(89, 1143, 6), // "InProp"
QT_MOC_LITERAL(90, 1150, 9), // "InBckProp"
QT_MOC_LITERAL(91, 1160, 7), // "OutProp"
QT_MOC_LITERAL(92, 1168, 10), // "OutBckProp"
QT_MOC_LITERAL(93, 1179, 10), // "CreditProp"
QT_MOC_LITERAL(94, 1190, 17), // "LastPayStatusProp"
QT_MOC_LITERAL(95, 1208, 17), // "LastPayStatusCode"
QT_MOC_LITERAL(96, 1226, 29), // "CoinAcceptorCCTalkAddressProp"
QT_MOC_LITERAL(97, 1256, 24), // "Hopper1CCTalkAddressProp"
QT_MOC_LITERAL(98, 1281, 24), // "Hopper2CCTalkAddressProp"
QT_MOC_LITERAL(99, 1306, 30), // "BillAccceptorCCTalkAddressProp"
QT_MOC_LITERAL(100, 1337, 30), // "BillDispenserCCTalkAddressProp"
QT_MOC_LITERAL(101, 1368, 21), // "HopperMaxCapacityProp"
QT_MOC_LITERAL(102, 1390, 15), // "CashBoxPathProp"
QT_MOC_LITERAL(103, 1406, 16), // "Hopper50PathProp"
QT_MOC_LITERAL(104, 1423, 17), // "Hopper100PathProp"
QT_MOC_LITERAL(105, 1441, 17), // "Hopper200PathProp"
QT_MOC_LITERAL(106, 1459, 17) // "RecyclerValueProp"

    },
    "Device\0initializeCache\0\0get\0key\0"
    "forceGet\0put\0value\0start\0enable\0disable\0"
    "isStarted\0isEnabled\0begin\0end\0"
    "setAcceptorChannelProperty\0Acceptors\0"
    "acceptor\0AcceptorChannels\0channel\0"
    "AcceptorChannelProperties\0property\0"
    "setAcceptorChannelPropertyByValue\0"
    "channelValue\0getAcceptorChannelProperty\0"
    "getAcceptorChannelPropertyByValue\0"
    "getAcceptorChannelPropertyKey\0"
    "getDispenserProperty\0Dispensers\0"
    "dispenser\0DispenserProperties\0"
    "getDispenserPropertyByValue\0"
    "getDispenserPropertyKey\0getDeviceProperty\0"
    "DeviceProperties\0getDevicePropertyKey\0"
    "requestCalibration\0coinVal\0noCoins\0"
    "requestReleaseCredit\0amount\0dumpBills\0"
    "initLogEntry\0time\0updateLogEntry\0stop\0"
    "status\0getLogEntry\0idx\0CoinAcceptor\0"
    "BillAcceptor\0UnknownAcceptor\0ChannelNone\0"
    "Channel1\0Channel2\0Channel3\0Channel4\0"
    "Channel5\0Channel6\0Channel7\0Channel8\0"
    "Channel9\0Channel10\0Channel11\0Channel12\0"
    "Channel13\0Channel14\0Channel15\0Channel16\0"
    "StatusProp\0UnitAddressProp\0ValueProp\0"
    "CurrencyProp\0InhibitProp\0PathProp\0"
    "DefaultPathProp\0CountProp\0PathCountProp\0"
    "PathSwitchLevelProp\0FirstCoinHopper\0"
    "SecondCoinHopper\0BillDispenser\0"
    "UnknownDispenser\0StatusDProp\0"
    "UnitAddressDProp\0ValueDProp\0"
    "DispenseCountDProp\0CoinCountDProp\0"
    "CoinCountStatDProp\0InProp\0InBckProp\0"
    "OutProp\0OutBckProp\0CreditProp\0"
    "LastPayStatusProp\0LastPayStatusCode\0"
    "CoinAcceptorCCTalkAddressProp\0"
    "Hopper1CCTalkAddressProp\0"
    "Hopper2CCTalkAddressProp\0"
    "BillAccceptorCCTalkAddressProp\0"
    "BillDispenserCCTalkAddressProp\0"
    "HopperMaxCapacityProp\0CashBoxPathProp\0"
    "Hopper50PathProp\0Hopper100PathProp\0"
    "Hopper200PathProp\0RecyclerValueProp"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Device[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      27,   14, // methods
       0,    0, // properties
       6,  250, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // methods: name, argc, parameters, tag, flags
       1,    0,  149,    2, 0x02 /* Public */,
       3,    1,  150,    2, 0x02 /* Public */,
       5,    1,  153,    2, 0x02 /* Public */,
       6,    2,  156,    2, 0x02 /* Public */,
       8,    0,  161,    2, 0x02 /* Public */,
       9,    0,  162,    2, 0x02 /* Public */,
      10,    0,  163,    2, 0x02 /* Public */,
      11,    0,  164,    2, 0x02 /* Public */,
      12,    0,  165,    2, 0x02 /* Public */,
      13,    0,  166,    2, 0x02 /* Public */,
      14,    0,  167,    2, 0x02 /* Public */,
      15,    4,  168,    2, 0x02 /* Public */,
      22,    4,  177,    2, 0x02 /* Public */,
      24,    3,  186,    2, 0x02 /* Public */,
      25,    3,  193,    2, 0x02 /* Public */,
      26,    3,  200,    2, 0x02 /* Public */,
      27,    2,  207,    2, 0x02 /* Public */,
      31,    2,  212,    2, 0x02 /* Public */,
      32,    2,  217,    2, 0x02 /* Public */,
      33,    1,  222,    2, 0x02 /* Public */,
      35,    1,  225,    2, 0x02 /* Public */,
      36,    2,  228,    2, 0x02 /* Public */,
      39,    1,  233,    2, 0x02 /* Public */,
      41,    0,  236,    2, 0x02 /* Public */,
      42,    2,  237,    2, 0x02 /* Public */,
      44,    2,  242,    2, 0x02 /* Public */,
      47,    1,  247,    2, 0x02 /* Public */,

 // methods: parameters
    QMetaType::Void,
    QMetaType::QString, QMetaType::QString,    4,
    QMetaType::QString, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    4,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Bool,
    QMetaType::Bool,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 16, 0x80000000 | 18, 0x80000000 | 20, QMetaType::QString,   17,   19,   21,    7,
    QMetaType::Void, 0x80000000 | 16, QMetaType::Int, 0x80000000 | 20, QMetaType::QString,   17,   23,   21,    7,
    QMetaType::QString, 0x80000000 | 16, 0x80000000 | 18, 0x80000000 | 20,   17,   19,   21,
    QMetaType::QString, 0x80000000 | 16, QMetaType::Int, 0x80000000 | 20,   17,    7,   21,
    QMetaType::QString, 0x80000000 | 16, 0x80000000 | 18, 0x80000000 | 20,   17,   19,   21,
    QMetaType::QString, 0x80000000 | 28, 0x80000000 | 30,   29,   21,
    QMetaType::QString, QMetaType::Int, 0x80000000 | 30,    7,   21,
    QMetaType::QString, QMetaType::Int, 0x80000000 | 30,    7,   21,
    QMetaType::QString, 0x80000000 | 34,   21,
    QMetaType::QString, 0x80000000 | 34,   21,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   37,   38,
    QMetaType::Void, QMetaType::QString,   40,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   43,    8,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   45,   46,
    QMetaType::QString, QMetaType::QString,   48,

 // enums: name, alias, flags, count, data
      16,   16, 0x0,    3,  280,
      18,   18, 0x0,   17,  286,
      20,   20, 0x0,   10,  320,
      28,   28, 0x0,    4,  340,
      30,   30, 0x0,    6,  348,
      34,   34, 0x0,   18,  360,

 // enum data: key, value
      49, uint(Device::CoinAcceptor),
      50, uint(Device::BillAcceptor),
      51, uint(Device::UnknownAcceptor),
      52, uint(Device::ChannelNone),
      53, uint(Device::Channel1),
      54, uint(Device::Channel2),
      55, uint(Device::Channel3),
      56, uint(Device::Channel4),
      57, uint(Device::Channel5),
      58, uint(Device::Channel6),
      59, uint(Device::Channel7),
      60, uint(Device::Channel8),
      61, uint(Device::Channel9),
      62, uint(Device::Channel10),
      63, uint(Device::Channel11),
      64, uint(Device::Channel12),
      65, uint(Device::Channel13),
      66, uint(Device::Channel14),
      67, uint(Device::Channel15),
      68, uint(Device::Channel16),
      69, uint(Device::StatusProp),
      70, uint(Device::UnitAddressProp),
      71, uint(Device::ValueProp),
      72, uint(Device::CurrencyProp),
      73, uint(Device::InhibitProp),
      74, uint(Device::PathProp),
      75, uint(Device::DefaultPathProp),
      76, uint(Device::CountProp),
      77, uint(Device::PathCountProp),
      78, uint(Device::PathSwitchLevelProp),
      79, uint(Device::FirstCoinHopper),
      80, uint(Device::SecondCoinHopper),
      81, uint(Device::BillDispenser),
      82, uint(Device::UnknownDispenser),
      83, uint(Device::StatusDProp),
      84, uint(Device::UnitAddressDProp),
      85, uint(Device::ValueDProp),
      86, uint(Device::DispenseCountDProp),
      87, uint(Device::CoinCountDProp),
      88, uint(Device::CoinCountStatDProp),
      89, uint(Device::InProp),
      90, uint(Device::InBckProp),
      91, uint(Device::OutProp),
      92, uint(Device::OutBckProp),
      93, uint(Device::CreditProp),
      94, uint(Device::LastPayStatusProp),
      95, uint(Device::LastPayStatusCode),
      96, uint(Device::CoinAcceptorCCTalkAddressProp),
      97, uint(Device::Hopper1CCTalkAddressProp),
      98, uint(Device::Hopper2CCTalkAddressProp),
      99, uint(Device::BillAccceptorCCTalkAddressProp),
     100, uint(Device::BillDispenserCCTalkAddressProp),
     101, uint(Device::HopperMaxCapacityProp),
     102, uint(Device::CashBoxPathProp),
     103, uint(Device::Hopper50PathProp),
     104, uint(Device::Hopper100PathProp),
     105, uint(Device::Hopper200PathProp),
     106, uint(Device::RecyclerValueProp),

       0        // eod
};

void Device::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Device *_t = static_cast<Device *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->initializeCache(); break;
        case 1: { QString _r = _t->get((*reinterpret_cast< QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 2: { QString _r = _t->forceGet((*reinterpret_cast< QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 3: _t->put((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 4: _t->start(); break;
        case 5: _t->enable(); break;
        case 6: _t->disable(); break;
        case 7: { bool _r = _t->isStarted();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 8: { bool _r = _t->isEnabled();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 9: _t->begin(); break;
        case 10: _t->end(); break;
        case 11: _t->setAcceptorChannelProperty((*reinterpret_cast< Acceptors(*)>(_a[1])),(*reinterpret_cast< AcceptorChannels(*)>(_a[2])),(*reinterpret_cast< AcceptorChannelProperties(*)>(_a[3])),(*reinterpret_cast< QString(*)>(_a[4]))); break;
        case 12: _t->setAcceptorChannelPropertyByValue((*reinterpret_cast< Acceptors(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< AcceptorChannelProperties(*)>(_a[3])),(*reinterpret_cast< QString(*)>(_a[4]))); break;
        case 13: { QString _r = _t->getAcceptorChannelProperty((*reinterpret_cast< Acceptors(*)>(_a[1])),(*reinterpret_cast< AcceptorChannels(*)>(_a[2])),(*reinterpret_cast< AcceptorChannelProperties(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 14: { QString _r = _t->getAcceptorChannelPropertyByValue((*reinterpret_cast< Acceptors(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< AcceptorChannelProperties(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 15: { QString _r = _t->getAcceptorChannelPropertyKey((*reinterpret_cast< Acceptors(*)>(_a[1])),(*reinterpret_cast< AcceptorChannels(*)>(_a[2])),(*reinterpret_cast< AcceptorChannelProperties(*)>(_a[3])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 16: { QString _r = _t->getDispenserProperty((*reinterpret_cast< Dispensers(*)>(_a[1])),(*reinterpret_cast< DispenserProperties(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 17: { QString _r = _t->getDispenserPropertyByValue((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< DispenserProperties(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 18: { QString _r = _t->getDispenserPropertyKey((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< DispenserProperties(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 19: { QString _r = _t->getDeviceProperty((*reinterpret_cast< DeviceProperties(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 20: { QString _r = _t->getDevicePropertyKey((*reinterpret_cast< DeviceProperties(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 21: _t->requestCalibration((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 22: _t->requestReleaseCredit((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 23: _t->dumpBills(); break;
        case 24: _t->initLogEntry((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 25: _t->updateLogEntry((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 26: { QString _r = _t->getLogEntry((*reinterpret_cast< QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject Device::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_Device.data,
    qt_meta_data_Device,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *Device::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Device::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Device.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Device::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 27)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 27;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 27)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 27;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
