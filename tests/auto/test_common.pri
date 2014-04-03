# include this after TARGET name of the unit test
CONFIG += testcase

QT += quick testlib qml quick concurrent sql

INCLUDEPATH += ../../../src

include(../../src/common.pri)
include(../../src/history.pri)
include(common/downloadmanager_mock.pri)
include(common/declarativewebutils_mock.pri)

# install the test
target.path = /opt/tests/sailfish-browser/auto
INSTALLS += target
