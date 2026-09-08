TEMPLATE = subdirs

SUBDIRS += tst_dbmanager \
    tst_declarativebookmarkmodel \
    tst_declarativehistorymodel \
    tst_desktopbookmarkwriter \
    tst_logins \
    tst_persistenttabmodel \
    tst_webutils

OTHER_FILES += \
    *.xml

common.path = /opt/tests/sailfish-browser/auto
common.files = tests.xml runtests.sh
INSTALLS += common
