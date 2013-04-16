TEMPLATE = subdirs
SUBDIRS += src tests translations

# The .desktop file
desktop.files = sailfish-browser.desktop
desktop.path = /usr/share/applications

INSTALLS += desktop