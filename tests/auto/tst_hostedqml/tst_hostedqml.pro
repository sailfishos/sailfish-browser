TARGET = tst_hostedqml

QT += qml quick dbus

include(../test_common.pri)

SOURCES += tst_hostedqml.cpp
HEADERS += mockportalbus.h

RESOURCES += tst_hostedqml.qrc
