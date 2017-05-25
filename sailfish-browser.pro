TEMPLATE = subdirs
SUBDIRS += src tests settings

# The .desktop file
desktop.files = sailfish-browser.desktop open-url.desktop
desktop.path = /usr/share/applications

quick_actions.files = org.sailfishos.browser.conf
quick_actions.path = /usr/share/lipstick/quickactions

dbus_service.files = org.sailfishos.browser.service \
                     org.sailfishos.browser.ui.service
dbus_service.path = /usr/share/dbus-1/services

chrome_scripts.files = chrome/*.js
chrome_scripts.path = /usr/lib/mozembedlite/chrome/embedlite/content

oneshots.files = oneshot.d/browser-cleanup-startup-cache \
                 oneshot.d/browser-update-default-data
oneshots.path  = /usr/lib/oneshot.d

data.files = data/prefs.js \
             data/ua-update.json.in
data.path = /usr/share/sailfish-browser/data

INSTALLS += desktop quick_actions dbus_service chrome_scripts oneshots data

OTHER_FILES += \
    rpm/*.spec
