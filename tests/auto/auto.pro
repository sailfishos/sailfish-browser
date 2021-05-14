TEMPLATE = subdirs

SUBDIRS += tst_dbmanager \
    tst_declarativebookmarkmodel \
    tst_declarativehistorymodel \
#    tst_declarativewebcontainer \
    tst_desktopbookmarkwriter \
    tst_downloadmimetypehandler \
    tst_logins \
    tst_persistenttabmodel \
#    tst_webpages \
    tst_webpagefactory \
    tst_webutils \
    tst_webview

OTHER_FILES += \
    *.xml

common.path = /opt/tests/sailfish-browser/auto
common.files = tests.xml runtests.sh
INSTALLS += common
