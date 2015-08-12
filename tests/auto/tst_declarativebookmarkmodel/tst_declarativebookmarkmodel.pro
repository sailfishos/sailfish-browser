TARGET = tst_declarativebookmarkmodel

QT += quick concurrent

CONFIG += link_pkgconfig

PKGCONFIG += mlite5

include(../test_common.pri)
include(../common/testobject.pri)
include(../../../src/bookmarks/bookmarks.pri)


# C++ sources
SOURCES += tst_declarativebookmarkmodel.cpp
