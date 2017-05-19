TARGET = tst_webutils
CONFIG += link_pkgconfig

PKGCONFIG += mlite5

include(../mocks/webengine/webengine.pri)
include(../test_common.pri)
include(../../../common/opensearchconfigs.pri)
include(../../../common/paths.pri)

LIBS += -lgtest -lgmock

INCLUDEPATH += $$SRCDIR \
    $$BROWSERSRCDIR

SOURCES += tst_webutils.cpp \
           $$BROWSERSRCDIR/declarativewebutils.cpp

HEADERS += $$BROWSERSRCDIR/declarativewebutils.h

INCLUDEPATH -= $$absolute_path(../../../../qtmozembed/src)
