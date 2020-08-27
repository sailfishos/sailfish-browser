INCLUDEPATH += $$PWD

# C++ sources
SOURCES += \
    $$PWD/bookmarkfiltermodel.cpp \
    $$PWD/declarativebookmarkmodel.cpp \
    $$PWD/desktopbookmarkwriter.cpp \
    $$PWD/bookmarkmanager.cpp \
    $$PWD/bookmark.cpp \
    $$PWD/iconfetcher.cpp

# C++ headers
HEADERS += \
    $$PWD/bookmarkfiltermodel.h \
    $$PWD/declarativebookmarkmodel.h \
    $$PWD/desktopbookmarkwriter.h \
    $$PWD/bookmarkmanager.h \
    $$PWD/bookmark.h \
    $$PWD/iconfetcher.h

DEFINES += DESKTOP_FILE_PATTERN=\\\"%1/sailfish-browser-%2-%3.desktop\\\"
DEFINES += DESKTOP_FILE=\\\"sailfish-browser-%2-%3.desktop\\\"
DEFINES += DEFAULT_DESKTOP_BOOKMARK_ICON=\\\"icon-launcher-bookmark\\\"
DEFINES += BASE64_IMAGE=\\\"data\:image\/png\;base64,%1\\\"
