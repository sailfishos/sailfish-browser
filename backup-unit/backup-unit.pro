TARGET = vault-browser

SOURCES = browserunit.cpp

CONFIG += link_pkgconfig

PKGCONFIG += qtaround   \
             vault-unit

TARGETPATH = /usr/libexec/jolla-vault/units
target.path = $$TARGETPATH
INSTALLS += target
