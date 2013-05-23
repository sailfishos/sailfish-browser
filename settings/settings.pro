TEMPLATE = lib
TARGET = browsersettingsplugin
TARGET = $$qtLibraryTarget($$TARGET)

MODULENAME = org/sailfishos/browser/settings
TARGETPATH = $$[QT_INSTALL_IMPORTS]/$$MODULENAME

QT += declarative
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
qmlpages.files = browser.qml

plugin_entry.path = /usr/share/jolla-settings/entries
plugin_entry.files = browser.json

SETTINGS_TS_FILE = $$OUT_PWD/settings-sailfish-browser.ts
SETTINGS_EE_QM = $$OUT_PWD/settings-sailfish-browser_eng_en.qm

settings_ts.commands += lupdate $$PWD -ts $$SETTINGS_TS_FILE
settings_ts.CONFIG += no_check_exist no_link
settings_ts.output = $$SETTINGS_TS_FILE
settings_ts.input = .

settings_ts_install.files = $$SETTINGS_TS_FILE
settings_ts_install.path = /usr/share/translations/source
settings_ts_install.CONFIG += no_check_exist

settings_ee.commands += lrelease -idbased $$SETTINGS_TS_FILE -qm $$SETTINGS_EE_QM
settings_ee.CONFIG += no_check_exist no_link
settings_ee.depends = settings_ts
settings_ee.input = $$SETTINGS_TS_FILE
settings_ee.output = $$SETTINGS_EE_QM

settings_ee_install.path = /usr/share/translations
settings_ee_install.files = $$SETTINGS_EE_QM
settings_ee_install.CONFIG += no_check_exist

QMAKE_EXTRA_TARGETS += settings_ts settings_ee

PRE_TARGETDEPS += settings_ee

INSTALLS += target import plugin_entry qmlpages settings_ts_install settings_ee_install

OTHER_FILES += *.qml *.json
