TARGET = tst_desktopbookmarkwriter

QT += concurrent network

include(../test_common.pri)
include(../../../apps/browser/bookmarks/bookmarks.pri)
include(../../../common/browserapp.pri)
include(../../../common/opensearchconfigs.pri)

SOURCES += tst_desktopbookmarkwriter.cpp

CONFIG(desktop) {
    DEFINES += TEST_DATA=\\\"$$PWD/content\\\"
} else {
    DEFINES += TEST_DATA=\\\"$$target.path/content\\\"
}

CONFIG += link_pkgconfig
PKGCONFIG += mlite5

testData.path = $$target.path/content
testData.files = content/*.png

INSTALLS += testData
