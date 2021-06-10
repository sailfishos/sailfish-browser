QT += qml quick gui gui-private dbus sql
# The name of your app
TARGET = sailfish-captiveportal

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
TS_PATH = $$PWD
# Shared translations in browser.pro should be skipped from other subprojects
# to avoid duplicated ids
#TS_PATH += $$PWD/../shared
TS_FILE = $$OUT_PWD/sailfish-captiveportal.ts
EE_QM = $$OUT_PWD/sailfish-captiveportal_eng_en.qm
include(../../translations/translations.pri)

include(../../defaults.pri)
include(../../common/browserapp.pri)
include(../../common/opensearchconfigs.pri)
include(../core/core.pri)
include(../history/history.pri)
include(../qtmozembed/qtmozembed.pri)
include(../factories/factories.pri)
include(../shared/shared.pri)

# QML files and folders of captiveportal
qml.path = $$DEPLOYMENT_PATH
qml.files = qml/captiveportal.qml qml/pages
INSTALLS += qml

# Captive portal sources
SOURCES += captiveportaladaptor.cpp \
    captiveportalservice.cpp \
    main.cpp

HEADERS += captiveportaladaptor.h \
    captiveportalservice.h

OTHER_FILES = ../qml/*
