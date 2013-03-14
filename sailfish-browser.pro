QT += opengl declarative
# The name of your app
TARGET = sailfish-browser

contains(QT_MAJOR_VERSION, 4) {
  PKGCONFIG += QJson
}

# Include qtmozembed
CONFIG += link_pkgconfig
isEmpty(QTEMBED_LIB) {
  PKGCONFIG += qtembedwidget x11
} else {
  LIBS+=$$QTEMBED_LIB
}

isEmpty(DEFAULT_COMPONENT_PATH) {
  DEFINES += DEFAULT_COMPONENTS_PATH=\"\\\"/usr/lib/mozembedlite/\\\"\"
} else {
  DEFINES += DEFAULT_COMPONENTS_PATH=\"\\\"$$DEFAULT_COMPONENT_PATH\\\"\"
}

# C++ sources
SOURCES += \
    src/declarativebrowsertab.cpp \
    src/declarativeparameters.cpp \
    sailfishbrowser.cpp \
    src/declarativebookmarkmodel.cpp

# C++ headers
HEADERS += \
    src/declarativebrowsertab.h \
    src/declarativeparameters.h \
    src/declarativebookmarkmodel.h

# QML files and folders
qml.files = *.qml pages cover browser.qml

# The .desktop file
desktop.files = sailfish-browser.desktop

# Please do not modify the following line.
include(sailfishapplication/sailfishapplication.pri)

OTHER_FILES = pages/BrowserPage.qml \
              rpm/sailfish-browser.spec
