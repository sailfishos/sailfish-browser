QT += qml quick gui gui-private dbus concurrent sql
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
  qml.files = *.qml pages cover


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

packagesExist(sailfishsilica) {
    DEFINES += SCALABLE_UI
    PKGCONFIG += sailfishsilica
}

# Include qtmozembed
isEmpty(QTEMBED_LIB) {
  PKGCONFIG += qt5embedwidget
} else {
  LIBS+=$$QTEMBED_LIB
}

include(common.pri)

# Translations
TS_PATH = $$PWD
TS_FILE = $$OUT_PWD/sailfish-browser.ts
EE_QM = $$OUT_PWD/sailfish-browser_eng_en.qm
include(../translations/translations.pri)
include(history.pri)
include(bookmarks.pri)

# C++ sources
SOURCES += \
    inputregion.cpp \
    sailfishbrowser.cpp \
    declarativewebcontainer.cpp \
    declarativewebpage.cpp \
    declarativewebutils.cpp \
    declarativewebviewcreator.cpp \
    browserservice.cpp \
    dbusadaptor.cpp \
    downloadmanager.cpp \
    iconfetcher.cpp \
    settingmanager.cpp \
    closeeventfilter.cpp \
    webpagequeue.cpp \
    webpages.cpp

# C++ headers
HEADERS += \
    inputregion.h \
    inputregion_p.h \
    declarativewebcontainer.h \
    declarativewebpage.h \
    declarativewebutils.h \
    declarativewebviewcreator.h \
    browserservice.h \
    dbusadaptor.h \
    downloadmanager.h \
    iconfetcher.h \
    settingmanager.h \
    closeeventfilter.h \
    webpagequeue.h \
    webpages.h \
    declarativefileuploadmode.h \
    declarativefileuploadfilter.h

OTHER_FILES = *.qml \
              cover/*.qml \
              pages/*.qml \
              pages/components/*.qml \
              pages/components/*.js \
              rpm/sailfish-browser.spec
