TARGET = tst_declarativewebcontainer

CONFIG += link_pkgconfig

PKGCONFIG += mlite5 nemotransferengine-qt5

QT += quick qml concurrent sql gui-private

include(../test_common.pri)
include(../../../src/core/core.pri)
include(../../../src/history/history.pri)
include(../../../src/qtmozembed/qtmozembed.pri)
# TODO: drop this include (decouple SettingManager from bookmarks)
include(../../../src/bookmarks/bookmarks.pri)

include(../common/declarativewebutils_mock.pri)

INCLUDEPATH += ../../../src

SOURCES += tst_declarativewebcontainer.cpp \
           ../../../src/downloadmanager.cpp

HEADERS += ../../../src/downloadmanager.h
