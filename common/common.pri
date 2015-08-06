isEmpty(EMBEDLITE_CONTENT_PATH) {
  DEFINES += EMBEDLITE_CONTENT_PATH=\"\\\"/usr/lib/mozembedlite/chrome/embedlite/content/\\\"\"
} else {
  DEFINES += EMBEDLITE_CONTENT_PATH=\"\\\"$$EMBEDLITE_CONTENT_PATH\\\"\"
}

isEmpty(USER_OPENSEARCH_PATH) {
  DEFINES += USER_OPENSEARCH_PATH=\"\\\"/.local/share/org.sailfishos/sailfish-browser/searchEngines/\\\"\"
} else {
  DEFINES += USER_OPENSEARCH_PATH=\"\\\"$$USER_OPENSEARCH_PATH\\\"\"
}

isEmpty(DEFAULT_USER_AGENT) {
  DEFINES += DEFAULT_USER_AGENT='"\\\"Mozilla/5.0 (Maemo; Linux; U; Jolla; Sailfish; Mobile; rv:31.0) Gecko/31.0 Firefox/31.0 SailfishBrowser/1.0\\\""'
} else {
  DEFINES += DEFAULT_USER_AGENT=\"\\\"$$DEFAULT_USER_AGENT\\\"\"
}

INCLUDEPATH += $$PWD

CONFIG += c++11

# C++ sources
SOURCES += \
    $$PWD/opensearchconfigs.cpp

# C++ headers
HEADERS += \
    $$PWD/opensearchconfigs.h
