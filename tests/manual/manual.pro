TEMPLATE = aux

# Data for functional tests
testdata.files = *.html \
                 *.css \
                 navigation/*.html \
                 icon-launcher-testbrowser.png
testdata.path = /opt/tests/sailfish-browser/manual/

# .desktop file used for functional testing
testdesktop.files = test-sailfish-browser.desktop
testdesktop.path = /usr/share/applications

INSTALLS += testdata testdesktop

OTHER_FILES += \
    *.html \
    *.css \
    navigation/*.html
