TARGET = tst_portalflows
QT += core dbus qml testlib
CONFIG += testcase console c++11
CONFIG -= app_bundle
SOURCES += tst_portalflows.cpp ../../../apps/captiveportal/portalflowservice.cpp
HEADERS += ../../../apps/captiveportal/portalflowservice.h
INCLUDEPATH += ../../../apps/captiveportal
target.path = /opt/tests/sailfish-browser/auto
INSTALLS += target

SOURCES += ../../../apps/captiveportal/captiveportalservice.cpp \
           ../../../apps/captiveportal/captiveportaladaptor.cpp
HEADERS += ../../../apps/captiveportal/captiveportalservice.h \
           ../../../apps/captiveportal/captiveportaladaptor.h
