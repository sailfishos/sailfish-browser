# include this after TARGET name of the unit test
CONFIG += testcase

QT += quick testlib qml quick concurrent sql

INCLUDEPATH += ../../../src

include(../../src/history.pri)

# install the test
target.path = /opt/tests/sailfish-browser/auto
INSTALLS += target
