TARGET = tst_dbmanager
NO_COMMON_INCLUDES=1
include(../test_common.pri)
include(../common/testobject.pri)

DEFINES += DB_NAME=\\\"sailfish-browser.sqlite\\\"

SOURCES += tst_dbmanager.cpp \
    ../../../src/dbmanager.cpp \
    ../../../src/dbworker.cpp \
    ../../../src/link.cpp \
    ../../../src/tab.cpp

HEADERS += ../../../src/dbmanager.h \
    ../../../src/dbworker.h \
    ../../../src/link.h \
    ../../../src/tab.h
