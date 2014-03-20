TARGET = tst_webview
include(../test_common.pri)

SOURCES += tst_webview.cpp \
    webUtilsMock.cpp \
    ../../../src/declarativewebpage.cpp \
    ../../../src/declarativewebviewcreator.cpp

HEADERS += webUtilsMock.h \
    ../../../src/declarativewebpage.h \
    ../../../src/declarativewebviewcreator.h

OTHER_FILES = *.qml

RESOURCES = tst_webview.qrc
