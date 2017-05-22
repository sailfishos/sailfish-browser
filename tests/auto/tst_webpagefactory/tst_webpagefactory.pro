TARGET = tst_webpagefactory

QT += qml quick sql concurrent

include(../mocks/declarativewebcontainer/declarativewebcontainer_mock.pri)
include(../mocks/declarativewebpage/declarativewebpage_mock.pri)

include(../test_common.pri)
include(../../../common/paths.pri)
include(../../../src/storage/storage.pri)
include(../../../src/factories/factories.pri)

SOURCES += tst_webpagefactory.cpp

LIBS += -lgtest -lgmock
