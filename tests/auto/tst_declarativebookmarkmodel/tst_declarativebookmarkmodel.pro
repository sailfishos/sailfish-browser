TARGET = tst_declarativebookmarkmodel

QT += network concurrent

CONFIG += link_pkgconfig

PKGCONFIG += mlite5

include(../test_common.pri)
include(../../../apps/browser/bookmarks/bookmarks.pri)
include(../../../common/browserapp.pri)
include(../../../common/opensearchconfigs.pri)

# C++ sources
SOURCES += tst_declarativebookmarkmodel.cpp
