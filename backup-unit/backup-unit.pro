TARGET = vault-browser
INCLUDEPATH += $$PWD/../apps/core
HEADERS += $$PWD/../apps/core/logging.h
SOURCES += browserunit.cpp \
           $$PWD/../apps/core/logging.cpp

CONFIG += link_pkgconfig

PKGCONFIG += vault

TARGETPATH = /usr/libexec/jolla-vault/units
target.path = $$TARGETPATH
vault_config.files = Browser.json
vault_config.path = /usr/share/jolla-vault/units
INSTALLS += target vault_config
