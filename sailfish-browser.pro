# The name of your app
TARGET = sailfish-browser

# C++ sources
SOURCES += \
    src/declarativebrowsertab.cpp \
    src/declarativeparameters.cpp \
    sailfishbrowser.cpp

# C++ headers
HEADERS += \
    src/declarativebrowsertab.h \
    src/declarativeparameters.h

# QML files and folders
qml.files = *.qml pages cover browser.qml

# The .desktop file
desktop.files = sailfish-browser.desktop

# Please do not modify the following line.
include(sailfishapplication/sailfishapplication.pri)

OTHER_FILES = rpm/sailfish-browser.yaml \
              pages/BrowserPage.qml \
              rpm/sailfish-browser.spec

