TARGET = tst_downloadmimetypehandler

QT -= gui

include(../test_common.pri)
include(../../../common/paths.pri)

INCLUDEPATH += ../../../src/

SOURCES += tst_downloadmimetypehandler.cpp \
           ../../../src/downloadmimetypehandler.cpp

HEADERS += \
           ../../../src/downloadmimetypehandler.h
