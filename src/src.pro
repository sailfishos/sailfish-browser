QT += opengl declarative
# The name of your app
TARGET = sailfish-browser

lessThan(QT_MAJOR_VERSION, 5) {
  CONFIG += link_pkgconfig
  PKGCONFIG += QJson
}

# Include qtmozembed
isEmpty(QTEMBED_LIB) {
  CONFIG += link_pkgconfig
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
    declarativebrowsertab.cpp \
    declarativeparameters.cpp \
    sailfishbrowser.cpp \
    declarativebookmarkmodel.cpp \
    bookmark.cpp \
    declarativewebutils.cpp

# C++ headers
HEADERS += \
    declarativebrowsertab.h \
    declarativeparameters.h \
    declarativebookmarkmodel.h \
    bookmark.h \
    declarativewebutils.h

# QML files and folders
qml.files = *.qml pages cover browser.qml

# Please do not modify the following line.
include(../sailfishapplication/sailfishapplication.pri)

OTHER_FILES = pages/BrowserPage.qml \
              rpm/sailfish-browser.spec
