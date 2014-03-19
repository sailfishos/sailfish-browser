TARGET = tst_webview
include(../test_common.pri)

SOURCES += tst_webview.cpp \
    webUtilsMock.cpp \
    ../../../src/declarativewebviewcreator.cpp

HEADERS += webUtilsMock.h \
    ../../../src/declarativewebviewcreator.h

OTHER_FILES = *.qml

RESOURCES = tst_webview.qrc
