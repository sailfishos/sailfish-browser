# C++ sources
SOURCES += \
    $$PWD/declarativebookmarkmodel.cpp \
    $$PWD/desktopbookmarkwriter.cpp \
    $$PWD/bookmarkmanager.cpp \
    $$PWD/bookmark.cpp

# C++ headers
HEADERS += \
    $$PWD/declarativebookmarkmodel.h \
    $$PWD/desktopbookmarkwriter.h \
    $$PWD/bookmarkmanager.h \
    $$PWD/bookmark.h

DEFINES += DESKTOP_FILE_PATTERN=\\\"%1/sailfish-browser-%2-%3.desktop\\\"
DEFINES += DESKTOP_FILE=\\\"sailfish-browser-%2-%3.desktop\\\"
DEFINES += DEFAULT_DESKTOP_BOOKMARK_ICON=\\\"icon-launcher-bookmark\\\"
DEFINES += BASE64_IMAGE=\\\"data\:image\/png\;base64,%1\\\"
