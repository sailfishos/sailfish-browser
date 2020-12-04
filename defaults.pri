QMAKE_CXXFLAGS += -Wparentheses -Werror -Wfatal-errors

isEmpty(DEFAULT_COMPONENT_PATH) {
  DEFINES += DEFAULT_COMPONENTS_PATH=\"\\\"$$[QT_INSTALL_LIBS]/mozembedlite/\\\"\"
} else {
  DEFINES += DEFAULT_COMPONENTS_PATH=\"\\\"$$DEFAULT_COMPONENT_PATH\\\"\"
}

DEFINES += BASE64_IMAGE=\\\"data\:image\/png\;base64,%1\\\"
DEFINES += DEFAULT_DESKTOP_BOOKMARK_ICON=\\\"icon-launcher-bookmark\\\"
