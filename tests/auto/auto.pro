# TODO: Change this to subdirs once we get first C++ test
TEMPLATE = subdirs

SUBDIRS += tst_declarativehistorymodel \
    tst_declarativetabmodel \
    tst_linkvalidator \
    tst_webview

OTHER_FILES += \
    *.xml

common.path = /opt/tests/sailfish-browser/auto
common.files = tests.xml
INSTALLS += common
