TARGET = tst_webutils
CONFIG += link_pkgconfig
QMAKE_LFLAGS += -lgtest -lgmock
PKGCONFIG += mlite5 sailfishsilica

include(../mocks/qmozcontext/qmozcontext.pri)
include(../test_common.pri)
include(../../../common/opensearchconfigs.pri)
include(../../../common/paths.pri)

INCLUDEPATH += $$SRCDIR \
    $$BROWSERSRCDIR \
    $$system(pkg-config --cflags sailfishsilica)

SOURCES += tst_webutils.cpp \
           $$BROWSERSRCDIR/declarativewebutils.cpp

HEADERS += $$BROWSERSRCDIR/declarativewebutils.h

INCLUDEPATH -= $$absolute_path(../../../../qtmozembed/src)
