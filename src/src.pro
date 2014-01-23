QT += qml quick dbus concurrent sql
# The name of your app
TARGET = sailfish-browser

CONFIG += link_pkgconfig

TARGETPATH = /usr/bin
target.path = $$TARGETPATH

INSTALLS += target

isEmpty(USE_RESOURCES) {
  DEPLOYMENT_PATH = /usr/share/$$TARGET
  # QML files and folders
  qml.path = $$DEPLOYMENT_PATH
  qml.files = *.qml pages


  DEFINES += DEPLOYMENT_PATH=\"\\\"\"$${DEPLOYMENT_PATH}/\"\\\"\"

  INSTALLS += qml
} else {
  DEFINES += USE_RESOURCES
  RESOURCES = sailfish-browser.qrc
}

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

# Translations
TS_PATH = $$PWD
TS_FILE = $$OUT_PWD/sailfish-browser.ts
EE_QM = $$OUT_PWD/sailfish-browser_eng_en.qm
include(../translations/translations.pri)
include(history.pri)

# C++ sources
SOURCES += \
    sailfishbrowser.cpp \
    declarativebookmarkmodel.cpp \
    bookmark.cpp \
    declarativewebcontainer.cpp \
    declarativewebutils.cpp \
    browserservice.cpp \
    dbusadaptor.cpp \
    downloadmanager.cpp \
    settingmanager.cpp \
    closeeventfilter.cpp

# C++ headers
HEADERS += \
    declarativebookmarkmodel.h \
    bookmark.h \
    declarativewebcontainer.h \
    declarativewebutils.h \
    browserservice.h \
    dbusadaptor.h \
    downloadmanager.h \
    settingmanager.h \
    closeeventfilter.h

OTHER_FILES = *.qml \
              pages/*.qml \
              pages/components/*.qml \
              pages/components/*.js \
              rpm/sailfish-browser.spec
