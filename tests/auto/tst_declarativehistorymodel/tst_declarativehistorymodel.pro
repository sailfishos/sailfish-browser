TARGET = tst_declarativehistorymodel

QT += quick sql

include(../test_common.pri)
include(../common/testobject.pri)
include(../mocks/declarativewebcontainer/declarativewebcontainer_mock.pri)
include(../mocks/faviconmanager/faviconmanager_mock.pri)
include(../../../apps/use_lib.pri)

SOURCES += tst_declarativehistorymodel.cpp

LIBS += -lgtest -lgmock
