INCLUDEPATH += $$PWD

# Include qtmozembed lib
isEmpty(QTEMBED_LIB) {
  PKGCONFIG += qt5embedwidget
} else {
  LIBS+=$$QTEMBED_LIB
}

# C++ sources
SOURCES += \
    $$PWD/declarativewebpage.cpp \
    $$PWD/declarativewebpagecreator.cpp

# C++ headers
HEADERS += \
    $$PWD/declarativewebpage.h \
    $$PWD/declarativewebpagecreator.h
