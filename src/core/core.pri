INCLUDEPATH += $$PWD

include(../../common/common.pri)

# C++ sources
SOURCES += \
    $$PWD/declarativewebcontainer.cpp \
    $$PWD/inputregion.cpp \
    $$PWD/settingmanager.cpp \
    $$PWD/webpagequeue.cpp \
    $$PWD/webpages.cpp

# C++ headers
HEADERS += \
    $$PWD/declarativefileuploadfilter.h \
    $$PWD/declarativefileuploadmode.h \
    $$PWD/declarativewebcontainer.h \
    $$PWD/inputregion.h \
    $$PWD/inputregion_p.h \
    $$PWD/settingmanager.h \
    $$PWD/webpagequeue.h \
    $$PWD/webpages.h
