QT += qml quick gui gui-private dbus concurrent sql
# The name of your app
TARGET = sailfish-browser

CONFIG += link_pkgconfig

TARGETPATH = /usr/bin
target.path = $$TARGETPATH

INSTALLS += target
PKGCONFIG +=  mlite5 sailfishwebengine
INCLUDEPATH += $$system(pkg-config --cflags sailfishwebengine)

packagesExist(qdeclarative5-boostable) {
    message("Building with qdeclarative-boostable support")
    DEFINES += HAS_BOOSTER
    PKGCONFIG += qdeclarative5-boostable
} else {
    warning("qdeclarative5-boostable not available; startup times will be slower")
}

# Translations
TS_PATH = $$PWD $$PWD/../shared
TS_FILE = $$OUT_PWD/sailfish-browser.ts
EE_QM = $$OUT_PWD/sailfish-browser_eng_en.qm
include(../../translations/translations.pri)

include(../../defaults.pri)
include(../../common/browserapp.pri)
include(../../common/opensearchconfigs.pri)
include(../core/core.pri)
include(../browser/browser.pri)
include(../history/history.pri)
include(../qtmozembed/qtmozembed.pri)
include(../factories/factories.pri)
include(settings/settings.pri)
include(bookmarks/bookmarks.pri)

# QML files and folders of browser
qml.path = $$DEPLOYMENT_PATH
qml.files = qml/*.qml qml/pages qml/cover
INSTALLS += qml

qmlshared.path = $$DEPLOYMENT_PATH/shared
qmlshared.files = ../shared/*
INSTALLS += qmlshared

# C++ sources
SOURCES += main.cpp
