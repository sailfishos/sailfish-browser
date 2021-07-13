TARGET = tst_dbmanager

QT += quick concurrent sql

include(../test_common.pri)
include(../../../common/browserapp.pri)
include(../../../apps/storage/storage.pri)
include(../mocks/faviconmanager/faviconmanager_mock.pri)

SOURCES += tst_dbmanager.cpp
