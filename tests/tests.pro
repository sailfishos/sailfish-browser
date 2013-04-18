TEMPLATE = aux

# Data for functional tests
testdata.files = manual/testpage.html manual/icon-launcher-testbrowser.png
testdata.path = /opt/tests/sailfish-browser/manual/

# .desktop file used for functional testing
testdesktop.files = manual/test-sailfish-browser.desktop
testdesktop.path = /usr/share/applications

INSTALLS += testdata testdesktop
