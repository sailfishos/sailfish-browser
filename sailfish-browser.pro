TEMPLATE = subdirs
SUBDIRS += src tests translations

# The .desktop file
desktop.files = sailfish-browser.desktop
desktop.path = /usr/share/applications

dbus_service.files = org.sailfishos.browser.service
dbus_service.path = /usr/share/dbus-1/services

INSTALLS += desktop dbus_service

OTHER_FILES += \
    tests/test-definition/*.xml \
    tests/auto/*.qml \
    rpm/*.spec
