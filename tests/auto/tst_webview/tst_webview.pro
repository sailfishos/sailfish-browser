TARGET = tst_webview
include(../test_common.pri)
include(../common/testobject.pri)
include(../../../src/bookmarks.pri)

CONFIG += link_pkgconfig

PKGCONFIG += mlite5

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
    ../../../src/settingmanager.cpp \
    ../../../src/webpagequeue.cpp \
    ../../../src/webpages.cpp

HEADERS += ../../../src/declarativewebcontainer.h \
    ../../../src/declarativewebpage.h \
    ../../../src/declarativewebviewcreator.h \
    ../../../src/settingmanager.h \
    ../../../src/webpagequeue.h \
    ../../../src/webpages.h

OTHER_FILES = *.qml

RESOURCES = tst_webview.qrc
