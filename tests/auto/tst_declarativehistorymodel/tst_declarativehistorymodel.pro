TARGET = tst_declarativehistorymodel

QT += quick sql

INCLUDEPATH += $$PWD/../common

include(../test_common.pri)
include(../common/testobject.pri)
include(../common/declarativewebpage_mock.pri)
include(../common/declarativewebcontainer_mock.pri)
include(../../../src/history/history.pri)

SOURCES += tst_declarativehistorymodel.cpp
