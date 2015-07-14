TARGET = tst_persistenttabmodel

CONFIG   += c++11

NO_COMMON_INCLUDES=1
include(../common/declarativewebpage_mock.pri)
include(../common/declarativewebcontainer_mock.pri)
include(../test_common.pri)
include(../../../src/history.pri)

SOURCES += tst_persistenttabmodel.cpp