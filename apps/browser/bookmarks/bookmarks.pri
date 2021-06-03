PKGCONFIG += sailfishwebengine

INCLUDEPATH += $$PWD

# C++ sources
SOURCES += \
    $$PWD/bookmarkfiltermodel.cpp \
    $$PWD/declarativebookmarkmodel.cpp \
    $$PWD/desktopbookmarkwriter.cpp \
    $$PWD/bookmarkmanager.cpp \
    $$PWD/bookmark.cpp

# C++ headers
HEADERS += \
    $$PWD/bookmarkfiltermodel.h \
    $$PWD/declarativebookmarkmodel.h \
    $$PWD/desktopbookmarkwriter.h \
    $$PWD/bookmarkmanager.h \
    $$PWD/bookmark.h

DEFINES += DESKTOP_FILE_PATTERN=\\\"%1/sailfish-browser-%2-%3.desktop\\\"
DEFINES += DESKTOP_FILE=\\\"sailfish-browser-%2-%3.desktop\\\"
