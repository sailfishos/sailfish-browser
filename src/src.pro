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

PKGCONFIG +=  nemotransferengine-qt5 mlite5 sailfishwebengine
INCLUDEPATH += $$system(pkg-config --cflags sailfishwebengine)

packagesExist(qdeclarative5-boostable) {
    message("Building with qdeclarative-boostable support")
    DEFINES += HAS_BOOSTER
    PKGCONFIG += qdeclarative5-boostable
} else {
    warning("qdeclarative5-boostable not available; startup times will be slower")
}

# Translations
TS_PATH = $$PWD
TS_FILE = $$OUT_PWD/sailfish-browser.ts
EE_QM = $$OUT_PWD/sailfish-browser_eng_en.qm
include(../translations/translations.pri)

include(../defaults.pri)
include(../common/browserapp.pri)
include(../common/opensearchconfigs.pri)
include(../common/paths.pri)
include(core/core.pri)
include(browser/browser.pri)
include(history/history.pri)
include(bookmarks/bookmarks.pri)
include(qtmozembed/qtmozembed.pri)
include(factories/factories.pri)

# C++ sources
SOURCES += main.cpp

OTHER_FILES = *.qml \
              cover/*.qml \
              pages/*.qml \
              pages/components/*.qml \
              pages/components/*.js \
              rpm/sailfish-browser.spec
