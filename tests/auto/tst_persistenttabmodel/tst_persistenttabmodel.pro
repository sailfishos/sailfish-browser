TARGET = tst_persistenttabmodel

QMAKE_LFLAGS += -lgtest -lgmock

QT += quick qml sql

include(../test_common.pri)
include(../mocks/declarativewebpage/declarativewebpage_mock.pri)
include(../mocks/declarativewebcontainer/declarativewebcontainer_mock.pri)

include(../../../src/history/history.pri)

SOURCES += tst_persistenttabmodel.cpp
