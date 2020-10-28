TEMPLATE = aux

qmlpages.path = /usr/share/jolla-settings/pages/browser
qmlpages.files = browser.qml

plugin_entry.path = /usr/share/jolla-settings/entries
plugin_entry.files = browser.json

INSTALLS += plugin_entry qmlpages

OTHER_FILES += *.qml *.json

