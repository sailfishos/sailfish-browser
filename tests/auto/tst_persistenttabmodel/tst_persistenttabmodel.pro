TARGET = tst_persistenttabmodel

QT += qml sql

include(../test_common.pri)
include(../mocks/declarativewebpage/declarativewebpage_mock.pri)
include(../mocks/declarativewebcontainer/declarativewebcontainer_mock.pri)

include(../../../common/paths.pri)
include(../../../src/history/history.pri)

SOURCES += tst_persistenttabmodel.cpp

LIBS += -lgtest -lgmock
