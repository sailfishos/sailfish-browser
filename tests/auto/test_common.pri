# include this after TARGET name of the unit test

QT += testlib

CONFIG += c++11

include(../../defaults.pri)

CONFIG(gcov) {
    message("GCOV instrumentalization enabled")
    QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage -O0
    QMAKE_LFLAGS += -lgcov -coverage
}

# install the test
target.path = /opt/tests/sailfish-browser/auto
INSTALLS += target

unitTestData.path = $$target.path/content

# define test data path
CONFIG(desktop) {
    DEFINES += TEST_DATA=\\\"$$PWD/content\\\"
} else {
    DEFINES += TEST_DATA=\\\"$$unitTestData.path\\\"
}
