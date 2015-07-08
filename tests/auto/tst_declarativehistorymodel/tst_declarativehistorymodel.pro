TARGET = tst_declarativehistorymodel
MOCK_WEBPAGE=1
include(../test_common.pri)
include(../common/testobject.pri)
include(../common/declarativewebpage_mock.pri)
include(../common/webview.pri)
include(../../../src/bookmarks.pri)

SOURCES += tst_declarativehistorymodel.cpp
