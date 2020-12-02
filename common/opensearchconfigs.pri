isEmpty(EMBEDLITE_CONTENT_PATH) {
  DEFINES += EMBEDLITE_CONTENT_PATH=\"\\\"$$[QT_INSTALL_LIBS]/mozembedlite/chrome/embedlite/content/\\\"\"
} else {
  DEFINES += EMBEDLITE_CONTENT_PATH=\"\\\"$$EMBEDLITE_CONTENT_PATH\\\"\"
}

isEmpty(USER_OPENSEARCH_PATH) {
  DEFINES += USER_OPENSEARCH_PATH=\"\\\"/.local/share/org.sailfishos/browser/searchEngines/\\\"\"
} else {
  DEFINES += USER_OPENSEARCH_PATH=\"\\\"$$USER_OPENSEARCH_PATH\\\"\"
}

INCLUDEPATH += $$PWD

CONFIG += c++11

# C++ sources
SOURCES += \
    $$PWD/opensearchconfigs.cpp

# C++ headers
HEADERS += \
    $$PWD/opensearchconfigs.h
