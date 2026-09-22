TEMPLATE = subdirs
SUBDIRS += apps tests settings backup-unit

tests.depends = apps

# The .desktop file
desktop.files = sailfish-browser.desktop sailfish-captiveportal.desktop
desktop.path = /usr/share/applications

dbus_service.files = org.sailfishos.browser.service \
                     org.sailfishos.browser.ui.service \
                     org.sailfishos.captiveportal.service
dbus_service.path = /usr/share/dbus-1/services

oneshots.files = oneshot.d/browser-cleanup-startup-cache \
                 oneshot.d/browser-update-default-data
oneshots.path  = /usr/lib/oneshot.d

data.files = data/prefs.js \
             data/ua-update.json.in
data.path = /usr/share/sailfish-browser/data

INSTALLS += desktop dbus_service oneshots data

usersession.path = /usr/lib/systemd/user/user-session.target.d
usersession.files += 50-sailfish-browser.conf
INSTALLS += usersession

OTHER_FILES += \
    rpm/*.spec
