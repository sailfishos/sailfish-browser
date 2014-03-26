# include this after TARGET name of the unit test
CONFIG += testcase

QT += quick testlib qml quick concurrent sql

INCLUDEPATH += ../../../src

include(../../src/common.pri)
include(../../src/history.pri)

HEADERS += ../../../src/declarativewebcontainer.h \
    ../../../src/declarativewebpage.h

SOURCES += ../../../src/declarativewebcontainer.cpp \
    ../../../src/declarativewebpage.cpp

CONFIG += link_pkgconfig

# WebContainer need this
isEmpty(QTEMBED_LIB) {
  PKGCONFIG += qt5embedwidget
} else {
  LIBS+=$$QTEMBED_LIB
}

# install the test
target.path = /opt/tests/sailfish-browser/auto
INSTALLS += target
