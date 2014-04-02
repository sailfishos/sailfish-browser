TARGET = tst_webview
include(../test_common.pri)

SOURCES += tst_webview.cpp \
    ../../../src/declarativewebviewcreator.cpp

HEADERS += ../../../src/declarativewebviewcreator.h

OTHER_FILES = *.qml

RESOURCES = tst_webview.qrc
