TEMPLATE = aux

# Data for functional tests
testdata.files = manual/*.html \
                 manual/icon-launcher-testbrowser.png
testdata.path = /opt/tests/sailfish-browser/manual/

# Autotests
testauto.files = auto/*qml
testauto.path = /opt/tests/sailfish-browser/auto/

# .desktop file used for functional testing
testdesktop.files = manual/test-sailfish-browser.desktop
testdesktop.path = /usr/share/applications

INSTALLS += testdata testdesktop testauto
