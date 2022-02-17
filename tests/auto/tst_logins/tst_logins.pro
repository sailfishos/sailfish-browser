TARGET = tst_logins

CONFIG += link_pkgconfig

PKGCONFIG += nemotransferengine-qt5 mlite5 sailfishwebengine systemsettings
INCLUDEPATH += $$system(pkg-config --cflags sailfishwebengine)

QT += quick concurrent sql gui-private

include(../test_common.pri)
include(../mocks/opensearchconfigs/opensearchconfigs_mock.pri)
include(../common/testobject.pri)
include(../../../apps/browser/bookmarks/bookmarks.pri)
include(../../../apps/browser/settings/settings.pri)
include(../../../apps/browser/browser.pri)
include(../../../apps/core/core.pri)
include(../../../apps/qtmozembed/qtmozembed.pri)
include(../../../apps/factories/factories.pri)
include(../../../apps/history/history.pri)
include(../../../common/browserapp.pri)

SOURCES += tst_logins.cpp
