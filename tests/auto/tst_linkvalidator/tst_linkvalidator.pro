TARGET = tst_linkvalidator
NO_COMMON_INCLUDES=1
include(../test_common.pri)
include(../common/declarativewebpage_mock.pri)

SOURCES += tst_linkvalidator.cpp \
    ../../../src/link.cpp \
    ../../../src/linkvalidator.cpp

HEADERS += ../../../src/link.h \
    ../../../src/linkvalidator.h
