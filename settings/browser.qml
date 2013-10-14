import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.configuration 1.0
import org.sailfishos.browser.settings 1.0

Page {

    function name2index(name) {
        switch (name) {
            case "Google": return 0
            case "Bing": return 1
            case "Yahoo": return 2
            default: return 0
        }
    }


    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        contentHeight: contentColumn.height

        Column {
            id: contentColumn

            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                //% "Browser"
                title: qsTrId("settings_browser-ph-browser")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                //% "Clear private data"
                text: qsTrId("settings_browser-bt-clear_private_data")

                onClicked: {
                    clearPrivateDataConfig.value = true
                }
            }

            ComboBox {
                id: searchEngine

                width: parent.width
                //: Label for combobox that sets search engine used in browser
                //% "Search engine"
                label: qsTrId("settings_browser-la-search_engine")
                currentIndex: name2index(searchEngineConfig.value)

                menu: ContextMenu {
                    MenuItem {
                        text: "Google"
                    }
                    MenuItem {
                        text: "Bing"
                    }
                    MenuItem {
                        text: "Yahoo"
                    }
                }

                onCurrentItemChanged: {
                    if (currentItem.text !== searchEngineConfig.value) {
                        searchEngineConfig.value = currentItem.text
                    }
                }
            }
        }
    }

    // TODO: enable it when browser daemon is implemented
    /*
    BrowserSettings {
        id: settings
    }
    */

    ConfigurationValue {
        id: clearPrivateDataConfig

        key: "/apps/sailfish-browser/actions/clear_private_data"
        defaultValue: false
    }

    ConfigurationValue {
        id: searchEngineConfig

        key: "/apps/sailfish-browser/settings/search_engine"
        defaultValue: "Google"

        onValueChanged: {
            if (searchEngine.currentItem.text !== value) {
                searchEngine.currentIndex = name2index(value)
            }
        }
    }
}
