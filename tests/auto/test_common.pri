# include this after TARGET name of the unit test

QT += testlib

# Put a file named gcov_enabled to this directory in order to enable code coverage
exists(gcov_enabled) {
    message("GCOV instrumentalization enabled")
    QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage -O0
    QMAKE_LFLAGS += -lgcov -coverage
}

# install the test
target.path = /opt/tests/sailfish-browser/auto
INSTALLS += target
