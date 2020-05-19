TARGET = vault-browser
INCLUDEPATH += $$PWD/../src/core
HEADERS += $$PWD/../src/core/logging.h
SOURCES += browserunit.cpp \
           $$PWD/../src/core/logging.cpp

CONFIG += link_pkgconfig

PKGCONFIG += qtaround   \
             vault

TARGETPATH = /usr/libexec/jolla-vault/units
target.path = $$TARGETPATH
vault_config.files = Browser.json
vault_config.path = /usr/share/jolla-vault/units
INSTALLS += target vault_config
