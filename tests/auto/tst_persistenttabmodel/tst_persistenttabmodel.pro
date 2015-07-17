TARGET = tst_persistenttabmodel

CONFIG += c++11

QT += quick qml sql

INCLUDEPATH += $$PWD/../common
include(../test_common.pri)
include(../common/declarativewebpage_mock.pri)
include(../common/declarativewebcontainer_mock.pri)

include(../../../src/history/history.pri)

SOURCES += tst_persistenttabmodel.cpp
