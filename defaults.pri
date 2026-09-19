QMAKE_CXXFLAGS += -Wparentheses -Wfatal-errors -Wsuggest-override

isEmpty(DEFAULT_COMPONENT_PATH) {
  DEFINES += DEFAULT_COMPONENTS_PATH=\"\\\"$$[QT_INSTALL_LIBS]/gecko-embedlite/\\\"\"
} else {
  DEFINES += DEFAULT_COMPONENTS_PATH=\"\\\"$$DEFAULT_COMPONENT_PATH\\\"\"
}

CONFIG += c++1z
