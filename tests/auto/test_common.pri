# include this after TARGET name of the unit test

QT += testlib

CONFIG += c++11

include(../../defaults.pri)

SRCDIR = $$PWD/../../apps
CORESRCDIR = $$SRCDIR/core
BROWSERSRCDIR = $$SRCDIR/browser

CONFIG(gcov) {
    message("GCOV instrumentalization enabled")
    QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage -O0
    LIBS += -lgcov -coverage
}

# install the test
target.path = /opt/tests/sailfish-browser/auto
INSTALLS += target
