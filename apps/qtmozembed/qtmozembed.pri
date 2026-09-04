INCLUDEPATH += $$PWD

QT += concurrent

CONFIG += link_pkgconfig
PKGCONFIG += qt5embedwidget

# C++ sources
SOURCES += \
    $$PWD/declarativewebpage.cpp

# C++ headers
HEADERS += \
    $$PWD/declarativewebpage.h
