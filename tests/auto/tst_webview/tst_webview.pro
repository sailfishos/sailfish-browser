TARGET = tst_webview
include(../test_common.pri)
include(../common/testobject.pri)
include(../common/webview.pri)
include(../../../src/bookmarks.pri)

SOURCES += tst_webview.cpp

OTHER_FILES = *.qml

RESOURCES = tst_webview.qrc
