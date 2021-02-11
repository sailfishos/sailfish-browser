INCLUDEPATH += $$PWD

CONFIG += link_pkgconfig
PKGCONFIG += sailfishpolicy

# C++ sources
SOURCES += \
    $$PWD/browsersettings.cpp \
    $$PWD/searchenginemodel.cpp

# C++ headers
HEADERS += \
    $$PWD/browsersettings.h \
    $$PWD/searchenginemodel.h
