TEMPLATE = subdirs
SUBDIRS += src tests settings

# The .desktop file
desktop.files = sailfish-browser.desktop open-url.desktop
desktop.path = /usr/share/applications

dbus_service.files = org.sailfishos.browser.service
dbus_service.path = /usr/share/dbus-1/services

chrome_scripts.files = chrome/*.js
chrome_scripts.path = /usr/lib/mozembedlite/chrome/embedlite/content

content.files = content/*
content.path = /usr/share/sailfish-browser/content

oneshots.files = cleanup-browser-startup-cache
oneshots.path  = /usr/lib/oneshot.d

INSTALLS += desktop dbus_service chrome_scripts content oneshots

OTHER_FILES += \
    rpm/*.spec \
    content/*.json
