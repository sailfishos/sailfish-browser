# TODO: Change this to subdirs once we get first C++ test
TEMPLATE = subdirs

SUBDIRS += tst_dbmanager \
    tst_declarativebookmarkmodel \
    tst_declarativehistorymodel \
    tst_declarativetabmodel \
    tst_declarativewebcontainer \
    tst_persistenttabmodel \
    tst_desktopbookmarkwriter \
    tst_linkvalidator \
    tst_webview

OTHER_FILES += \
    *.xml

common.path = /opt/tests/sailfish-browser/auto
common.files = tests.xml runtests.sh
INSTALLS += common
