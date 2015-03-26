# C++ sources
SOURCES += \
    $$PWD/declarativetabmodel.cpp \
    $$PWD/persistenttabmodel.cpp \
    $$PWD/privatetabmodel.cpp \
    $$PWD/dbmanager.cpp \
    $$PWD/dbworker.cpp \
    $$PWD/link.cpp \
    $$PWD/linkvalidator.cpp \
    $$PWD/declarativehistorymodel.cpp \
    $$PWD/tab.cpp

# C++ headers
HEADERS += \
    $$PWD/declarativetabmodel.h \
    $$PWD/persistenttabmodel.h \
    $$PWD/privatetabmodel.h \
    $$PWD/dbmanager.h \
    $$PWD/dbworker.h \
    $$PWD/link.h \
    $$PWD/linkvalidator.h \
    $$PWD/declarativehistorymodel.h \
    $$PWD/tab.h

DEFINES += DB_NAME=\\\"sailfish-browser.sqlite\\\"
