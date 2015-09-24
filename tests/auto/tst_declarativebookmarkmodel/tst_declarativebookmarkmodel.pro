TARGET = tst_declarativebookmarkmodel

QT += quick concurrent

include(../test_common.pri)
include(../common/testobject.pri)
include(../../../src/bookmarks/bookmarks.pri)

# C++ sources
SOURCES += tst_declarativebookmarkmodel.cpp

# Bookmarks data
unitTestData.files = data/bookmarks.json
INSTALLS += unitTestData
