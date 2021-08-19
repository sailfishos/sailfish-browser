INCLUDEPATH += $$PWD

CONFIG += link_pkgconfig
PKGCONFIG += sailfishpolicy nemotransferengine-qt5

DEPLOYMENT_PATH = /usr/share/$$TARGET
DEFINES += DEPLOYMENT_PATH=\"\\\"\"$${DEPLOYMENT_PATH}/\"\\\"\"

# C++ sources
SOURCES += \
    $$PWD/browser.cpp \
    $$PWD/closeeventfilter.cpp \
    $$PWD/datafetcher.cpp \
    $$PWD/downloadmanager.cpp \
    $$PWD/downloadmimetypehandler.cpp \
    $$PWD/declarativewebcontainer.cpp \
    $$PWD/declarativewebutils.cpp \
    $$PWD/faviconmanager.cpp \
    $$PWD/inputregion.cpp \
    $$PWD/logging.cpp \
    $$PWD/secureaction.cpp \
    $$PWD/settingmanager.cpp \
    $$PWD/webpagequeue.cpp \
    $$PWD/webpages.cpp

# C++ headers
HEADERS += \
    $$PWD/browser.h \
    $$PWD/browser_p.h \
    $$PWD/closeeventfilter.h \
    $$PWD/datafetcher.h \
    $$PWD/declarativewebcontainer.h \
    $$PWD/declarativewebutils.h \
    $$PWD/downloadmanager.h \
    $$PWD/downloadmimetypehandler.h \
    $$PWD/downloadstatus.h \
    $$PWD/faviconmanager.h \
    $$PWD/inputregion.h \
    $$PWD/inputregion_p.h \
    $$PWD/logging.h \
    $$PWD/secureaction.h \
    $$PWD/settingmanager.h \
    $$PWD/webpagequeue.h \
    $$PWD/webpages.h
