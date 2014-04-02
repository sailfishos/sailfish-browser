# C++ sources
SOURCES += \
    $$PWD/declarativetabmodel.cpp \
    $$PWD/dbmanager.cpp \
    $$PWD/dbworker.cpp \
    $$PWD/link.cpp \
    $$PWD/linkvalidator.cpp \
    $$PWD/declarativehistorymodel.cpp \
    $$PWD/tab.cpp \
    $$PWD/webpages.cpp

# C++ headers
HEADERS += \
    $$PWD/declarativetabmodel.h \
    $$PWD/dbmanager.h \
    $$PWD/dbworker.h \
    $$PWD/link.h \
    $$PWD/linkvalidator.h \
    $$PWD/declarativehistorymodel.h \
    $$PWD/tab.h \
    $$PWD/webpages.h

DEFINES += DB_NAME=\\\"sailfish-browser.sqlite\\\"
