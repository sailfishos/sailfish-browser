QT += quick

CONFIG += link_pkgconfig
PKGCONFIG += qt5embedwidget

INCLUDEPATH += $$PWD/core $$PWD/storage $$PWD/history $$PWD/qtmozembed $$PWD/../common
LIBS += -L$$PWD/lib -lsailfishbrowser
