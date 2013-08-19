QT += opengl qml quick dbus concurrent
# The name of your app
TARGET = sailfish-browser

CONFIG += link_pkgconfig

TARGETPATH = /usr/bin
target.path = $$TARGETPATH

DEPLOYMENT_PATH = /usr/share/$$TARGET
qml.path = $$DEPLOYMENT_PATH

INSTALLS += target qml

DEFINES += DEPLOYMENT_PATH=\"\\\"\"$${DEPLOYMENT_PATH}/\"\\\"\"

PKGCONFIG +=  nemotransferengine-qt5 mlite5

packagesExist(qdeclarative5-boostable) {
    message("Building with qdeclarative-boostable support")
    DEFINES += HAS_BOOSTER
    PKGCONFIG += qdeclarative5-boostable
} else {
    warning("qdeclarative5-boostable not available; startup times will be slower")
}

# Include qtmozembed
isEmpty(QTEMBED_LIB) {
  PKGCONFIG += qt5embedwidget
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
    sailfishbrowser.cpp \
    declarativebookmarkmodel.cpp \
    bookmark.cpp \
    declarativewebutils.cpp \
    browserservice.cpp \
    dbusadaptor.cpp \
    declarativewebthumbnail.cpp \
    downloadmanager.cpp \
    settingmanager.cpp \
    closeeventfilter.cpp

# C++ headers
HEADERS += \
    declarativebrowsertab.h \
    declarativebookmarkmodel.h \
    bookmark.h \
    declarativewebutils.h \
    browserservice.h \
    dbusadaptor.h \
    declarativewebthumbnail.h \
    downloadmanager.h \
    settingmanager.h \
    closeeventfilter.h

# QML files and folders
qml.files = *.qml pages cover browser.qml

OTHER_FILES = pages/BrowserPage.qml \
              rpm/sailfish-browser.spec
