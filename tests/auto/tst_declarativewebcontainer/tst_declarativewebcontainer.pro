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

include(../mocks/declarativewebutils/declarativewebutils_mock.pri)
include(../mocks/downloadmanager/downloadmanager_mock.pri)

SOURCES += tst_declarativewebcontainer.cpp
