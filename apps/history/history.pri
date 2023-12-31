INCLUDEPATH += $$PWD

# Models depends on storage
include(../storage/storage.pri)

# C++ sources
SOURCES += \
    $$PWD/declarativetabmodel.cpp \
    $$PWD/persistenttabmodel.cpp \
    $$PWD/privatetabmodel.cpp \
    $$PWD/declarativetabfiltermodel.cpp \
    $$PWD/declarativehistorymodel.cpp

# C++ headers
HEADERS += \
    $$PWD/declarativetabmodel.h \
    $$PWD/persistenttabmodel.h \
    $$PWD/privatetabmodel.h \
    $$PWD/declarativetabfiltermodel.h \
    $$PWD/declarativehistorymodel.h
