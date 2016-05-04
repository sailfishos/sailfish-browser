TARGET = tst_dbmanager

QT += quick concurrent sql

include(../test_common.pri)
include(../../../common/paths.pri)
include(../../../src/storage/storage.pri)

SOURCES += tst_dbmanager.cpp
