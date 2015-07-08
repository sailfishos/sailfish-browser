CONFIG += link_pkgconfig

PKGCONFIG += mlite5

# WebContainer need this
isEmpty(QTEMBED_LIB) {
  PKGCONFIG += qt5embedwidget
} else {
  LIBS+=$$QTEMBED_LIB
}

QT += gui-private

SOURCES += ../../../src/declarativewebcontainer.cpp \
    ../../../src/declarativewebpagecreator.cpp \
    ../../../src/settingmanager.cpp \
    ../../../src/webpagequeue.cpp \
    ../../../src/webpages.cpp

HEADERS += ../../../src/declarativewebcontainer.h \
    ../../../src/declarativewebpagecreator.h \
    ../../../src/settingmanager.h \
    ../../../src/webpagequeue.h \
    ../../../src/webpages.h

isEmpty(MOCK_WEBPAGE) {
    SOURCES += ../../../src/declarativewebpage.cpp
    HEADERS += ../../../src/declarativewebpage.h
}
