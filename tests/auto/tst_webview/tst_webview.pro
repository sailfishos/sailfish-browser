TARGET = tst_webview
include(../test_common.pri)

CONFIG += link_pkgconfig

# WebContainer need this
isEmpty(QTEMBED_LIB) {
  PKGCONFIG += qt5embedwidget
} else {
  LIBS+=$$QTEMBED_LIB
}

SOURCES += tst_webview.cpp \
    ../../../src/declarativewebcontainer.cpp \
    ../../../src/declarativewebpage.cpp \
    ../../../src/declarativewebviewcreator.cpp \
    ../../../src/webpages.cpp

HEADERS += ../../../src/declarativewebcontainer.h \
    ../../../src/declarativewebpage.h \
    ../../../src/declarativewebviewcreator.h \
    ../../../src/webpages.h

OTHER_FILES = *.qml

RESOURCES = tst_webview.qrc
