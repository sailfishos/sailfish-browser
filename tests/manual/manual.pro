TEMPLATE = aux

# Data for functional tests
testdata.files = *.txt \
                 *.sh \
                 *.html \
                 *.js \
                 *.css \
                 mix-blend-mode/*.png \
                 mix-blend-mode/*.svg \
                 icon-launcher-testbrowser.png
testdata.path = /opt/tests/sailfish-browser/manual/

testnavigationdata.files = navigation/*.html
testnavigationdata.path = /opt/tests/sailfish-browser/manual/navigation/

# .desktop file used for functional testing
testdesktop.files = test-sailfish-browser.desktop
testdesktop.path = /usr/share/applications

INSTALLS += testdata testnavigationdata testdesktop

OTHER_FILES += \
    *.html \
    *.js \
    *.css \
    navigation/*.html \
    *.sh \
    *.txt
