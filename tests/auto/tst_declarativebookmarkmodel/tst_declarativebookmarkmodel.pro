TARGET = tst_declarativebookmarkmodel

QT += network concurrent

CONFIG += link_pkgconfig

PKGCONFIG += mlite5

include(../test_common.pri)
include(../../../src/bookmarks/bookmarks.pri)
include(../../../common/paths.pri)

# C++ sources
SOURCES += tst_declarativebookmarkmodel.cpp
