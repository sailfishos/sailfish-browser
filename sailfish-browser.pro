TEMPLATE = subdirs
SUBDIRS += src tests settings backup-unit

# The .desktop file
desktop.files = sailfish-browser.desktop sailfish-browser-captiveportal.desktop open-url.desktop
desktop.path = /usr/share/applications

dbus_service.files = org.sailfishos.browser.service \
                     org.sailfishos.browser.ui.service \
                     org.sailfishos.browser.captiveportal.service
dbus_service.path = /usr/share/dbus-1/services

chrome_scripts.files = chrome/*.js
chrome_scripts.path = $$[QT_INSTALL_LIBS]/mozembedlite/chrome/embedlite/content

oneshots.files = oneshot.d/browser-cleanup-startup-cache \
                 oneshot.d/browser-update-default-data
oneshots.path  = /usr/lib/oneshot.d

data.files = data/prefs.js \
             data/ua-update.json.in
data.path = /usr/share/sailfish-browser/data

INSTALLS += desktop dbus_service chrome_scripts oneshots data

OTHER_FILES += \
    rpm/*.spec
