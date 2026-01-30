/****************************************************************************
** Meta object code from reading C++ file 'graph_list_widget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.18)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../src/graph_list_widget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'graph_list_widget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.18. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_GraphListWidget_t {
    QByteArrayData data[14];
    char stringdata0[185];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_GraphListWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_GraphListWidget_t qt_meta_stringdata_GraphListWidget = {
    {
QT_MOC_LITERAL(0, 0, 15), // "GraphListWidget"
QT_MOC_LITERAL(1, 16, 13), // "graphSelected"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 9), // "GraphInfo"
QT_MOC_LITERAL(4, 41, 5), // "graph"
QT_MOC_LITERAL(5, 47, 18), // "graphDoubleClicked"
QT_MOC_LITERAL(6, 66, 18), // "onSelectionChanged"
QT_MOC_LITERAL(7, 85, 19), // "onItemDoubleClicked"
QT_MOC_LITERAL(8, 105, 16), // "QListWidgetItem*"
QT_MOC_LITERAL(9, 122, 4), // "item"
QT_MOC_LITERAL(10, 127, 15), // "onBrowseClicked"
QT_MOC_LITERAL(11, 143, 16), // "onRefreshClicked"
QT_MOC_LITERAL(12, 160, 19), // "onFilterTextChanged"
QT_MOC_LITERAL(13, 180, 4) // "text"

    },
    "GraphListWidget\0graphSelected\0\0GraphInfo\0"
    "graph\0graphDoubleClicked\0onSelectionChanged\0"
    "onItemDoubleClicked\0QListWidgetItem*\0"
    "item\0onBrowseClicked\0onRefreshClicked\0"
    "onFilterTextChanged\0text"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_GraphListWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x06 /* Public */,
       5,    1,   52,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    0,   55,    2, 0x08 /* Private */,
       7,    1,   56,    2, 0x08 /* Private */,
      10,    0,   59,    2, 0x08 /* Private */,
      11,    0,   60,    2, 0x08 /* Private */,
      12,    1,   61,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   13,

       0        // eod
};

void GraphListWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<GraphListWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->graphSelected((*reinterpret_cast< const GraphInfo(*)>(_a[1]))); break;
        case 1: _t->graphDoubleClicked((*reinterpret_cast< const GraphInfo(*)>(_a[1]))); break;
        case 2: _t->onSelectionChanged(); break;
        case 3: _t->onItemDoubleClicked((*reinterpret_cast< QListWidgetItem*(*)>(_a[1]))); break;
        case 4: _t->onBrowseClicked(); break;
        case 5: _t->onRefreshClicked(); break;
        case 6: _t->onFilterTextChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (GraphListWidget::*)(const GraphInfo & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GraphListWidget::graphSelected)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (GraphListWidget::*)(const GraphInfo & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&GraphListWidget::graphDoubleClicked)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject GraphListWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_GraphListWidget.data,
    qt_meta_data_GraphListWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *GraphListWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GraphListWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_GraphListWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int GraphListWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void GraphListWidget::graphSelected(const GraphInfo & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void GraphListWidget::graphDoubleClicked(const GraphInfo & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
