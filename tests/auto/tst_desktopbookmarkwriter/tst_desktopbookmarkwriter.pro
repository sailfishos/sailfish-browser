TARGET = tst_desktopbookmarkwriter

QT += concurrent network

include(../test_common.pri)
include(../../../src/bookmarks/bookmarks.pri)

SOURCES += tst_desktopbookmarkwriter.cpp

CONFIG += link_pkgconfig
PKGCONFIG += mlite5

unitTestData.files = content/*.png

INSTALLS += unitTestData
