INCLUDEPATH += $$PWD

# C++ sources
SOURCES += \
    $$PWD/dbmanager.cpp \
    $$PWD/dbworker.cpp \
    $$PWD/link.cpp \
    $$PWD/tab.cpp

# C++ headers
HEADERS += \
    $$PWD/dbmanager.h \
    $$PWD/dbworker.h \
    $$PWD/link.h \
    $$PWD/tab.h

DEFINES += DB_NAME=\\\"sailfish-browser.sqlite\\\"
