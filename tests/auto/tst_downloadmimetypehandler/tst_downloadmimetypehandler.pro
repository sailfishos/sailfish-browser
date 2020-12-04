TARGET = tst_downloadmimetypehandler

QT -= gui

include(../test_common.pri)
include(../../../common/browserapp.pri)

INCLUDEPATH += $$CORESRCDIR

SOURCES += tst_downloadmimetypehandler.cpp \
           $$CORESRCDIR/downloadmimetypehandler.cpp

HEADERS += \
           $$CORESRCDIR/downloadmimetypehandler.h
