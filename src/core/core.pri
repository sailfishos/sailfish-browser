INCLUDEPATH += $$PWD

PKGCONFIG += \
        sailfishsilica

# C++ sources
SOURCES += \
    $$PWD/declarativewebcontainer.cpp \
    $$PWD/inputregion.cpp \
    $$PWD/logging.cpp \
    $$PWD/settingmanager.cpp \
    $$PWD/webpagequeue.cpp \
    $$PWD/webpages.cpp

# C++ headers
HEADERS += \
    $$PWD/declarativewebcontainer.h \
    $$PWD/inputregion.h \
    $$PWD/inputregion_p.h \
    $$PWD/logging.h \
    $$PWD/settingmanager.h \
    $$PWD/webpagequeue.h \
    $$PWD/webpages.h
