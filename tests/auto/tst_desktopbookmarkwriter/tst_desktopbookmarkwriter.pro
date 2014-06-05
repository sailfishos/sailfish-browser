TARGET = tst_desktopbookmarkwriter

NO_COMMON_INCLUDES=1
include(../test_common.pri)
include(../../../src/bookmarks.pri)

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

INSTALLS += target testData
