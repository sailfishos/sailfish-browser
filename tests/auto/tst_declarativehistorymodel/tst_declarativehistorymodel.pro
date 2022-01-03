TARGET = tst_declarativehistorymodel

QT += quick sql

include(../test_common.pri)
include(../common/testobject.pri)
include(../mocks/declarativewebpage/declarativewebpage_mock.pri)
include(../mocks/declarativewebcontainer/declarativewebcontainer_mock.pri)
include(../mocks/faviconmanager/faviconmanager_mock.pri)
include(../../../common/browserapp.pri)
include(../../../apps/history/history.pri)

SOURCES += tst_declarativehistorymodel.cpp

LIBS += -lgtest -lgmock
