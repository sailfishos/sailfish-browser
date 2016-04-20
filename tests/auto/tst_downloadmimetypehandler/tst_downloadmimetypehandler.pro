TARGET = tst_downloadmimetypehandler

QT -= gui

include(../test_common.pri)
include(../../../common/paths.pri)

INCLUDEPATH += $$BROWSERSRCDIR

SOURCES += tst_downloadmimetypehandler.cpp \
           $$BROWSERSRCDIR/downloadmimetypehandler.cpp

HEADERS += \
           $$BROWSERSRCDIR/downloadmimetypehandler.h
