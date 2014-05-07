TEMPLATE = lib
TARGET = browsersettingsplugin
TARGET = $$qtLibraryTarget($$TARGET)

MODULENAME = org/sailfishos/browser/settings
TARGETPATH = $$[QT_INSTALL_QML]/$$MODULENAME

QT += qml
CONFIG += plugin

import.files = qmldir
import.path = $$TARGETPATH
target.path = $$TARGETPATH

SOURCES += \
        declarative_plugin.cpp \
        browsersettings.cpp

HEADERS += \
        browsersettings.h

qmlpages.path = /usr/share/jolla-settings/pages/browser
qmlpages.files = browser.qml Privacy.qml

plugin_entry.path = /usr/share/jolla-settings/entries
plugin_entry.files = browser.json

INSTALLS += target import plugin_entry qmlpages

OTHER_FILES += *.qml *.json

# Translations
TS_PATH = $$PWD
TS_FILE = $$OUT_PWD/settings-sailfish-browser.ts
EE_QM = $$OUT_PWD/settings-sailfish-browser_eng_en.qm
include(../translations/translations.pri)
