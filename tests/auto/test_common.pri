# include this after TARGET name of the unit test
CONFIG += testcase

QT += quick testlib qml quick concurrent sql

INCLUDEPATH += ../../../src

include(../../src/history.pri)

HEADERS += ../../../src/declarativewebcontainer.h
SOURCES += ../../../src/declarativewebcontainer.cpp

CONFIG += link_pkgconfig

# WebContainer need this
PKGCONFIG += qt5embedwidget

# install the test
target.path = /opt/tests/sailfish-browser/auto
INSTALLS += target
