TARGET = tst_webutils
CONFIG += link_pkgconfig

PKGCONFIG += mlite5

include(../mocks/webengine/webengine.pri)
include(../test_common.pri)
include(../../../common/opensearchconfigs.pri)
include(../../../common/browserapp.pri)

LIBS += -lgtest -lgmock

INCLUDEPATH += $$SRCDIR \
    $$CORESRCDIR

SOURCES += tst_webutils.cpp \
           $$CORESRCDIR/declarativewebutils.cpp

HEADERS += $$CORESRCDIR/declarativewebutils.h

INCLUDEPATH -= $$absolute_path(../../../../qtmozembed/src)
