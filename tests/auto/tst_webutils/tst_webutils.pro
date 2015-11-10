TARGET = tst_webutils
CONFIG += link_pkgconfig
QMAKE_LFLAGS += -lgtest -lgmock
PKGCONFIG += mlite5

include(../mocks/qmozcontext/qmozcontext.pri)
include(../mocks/settingmanager/settingmanager_mock.pri)
include(../test_common.pri)
include(../../../common/opensearchconfigs.pri)
include(../../../common/paths.pri)

INCLUDEPATH += ../../../src/

SOURCES += tst_webutils.cpp \
           ../../../src/declarativewebutils.cpp

HEADERS += \
           ../../../src/declarativewebutils.h

INCLUDEPATH -= $$absolute_path(../../../../qtmozembed/src)
