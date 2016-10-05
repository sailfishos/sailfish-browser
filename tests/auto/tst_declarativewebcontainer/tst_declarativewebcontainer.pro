TARGET = tst_declarativewebcontainer

CONFIG += link_pkgconfig

QMAKE_LFLAGS += -lgtest -lgmock

PKGCONFIG += mlite5 nemotransferengine-qt5 sailfishwebengine
INCLUDEPATH += $$system(pkg-config --cflags sailfishwebengine)

QT += quick qml concurrent sql gui-private

include(../mocks/qmozcontext/qmozcontext.pri)
include(../mocks/qmozwindow/qmozwindow.pri)
include(../mocks/webpagefactory/webpagefactory.pri)
include(../mocks/declarativewebpage/declarativewebpage_mock.pri)
include(../mocks/declarativewebutils/declarativewebutils_mock.pri)
include(../mocks/downloadmanager/downloadmanager_mock.pri)
include(../mocks/opensearchconfigs/opensearchconfigs_mock.pri)

include(../test_common.pri)
include(../../../common/paths.pri)
include(../../../src/core/core.pri)
include(../../../src/history/history.pri)

SOURCES += tst_declarativewebcontainer.cpp

# Avoid inclusion of qtmozembed headers in devel environment
INCLUDEPATH -= $$absolute_path(../../../../qtmozembed/src)
