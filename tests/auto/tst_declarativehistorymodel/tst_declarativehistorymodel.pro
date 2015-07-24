TARGET = tst_declarativehistorymodel

QT += quick sql

include(../test_common.pri)
include(../common/testobject.pri)
include(../mocks/declarativewebpage/declarativewebpage_mock.pri)
include(../mocks/declarativewebcontainer/declarativewebcontainer_mock.pri)
include(../../../src/history/history.pri)

SOURCES += tst_declarativehistorymodel.cpp
