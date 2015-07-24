TARGET = tst_linkvalidator

include(../test_common.pri)

INCLUDEPATH += ../../../src/history

SOURCES += tst_linkvalidator.cpp \
    ../../../src/history/linkvalidator.cpp

HEADERS +=  ../../../src/history/linkvalidator.h
