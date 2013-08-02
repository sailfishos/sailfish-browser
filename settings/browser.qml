import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.configuration 1.0
import org.sailfishos.browser.settings 1.0

Page {
    Column {
        width: parent.width
        spacing: Theme.paddingLarge

        PageHeader {
            //% "Browser"
            title: qsTrId("settings_browser-ph-browser")
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            //% "Clear private data"
            text: qsTrId("settings_browser-bt-clear_private_data")

            onClicked: {
                console.log("user wants to clear private data")
                clearPrivateDataConfig.value = true
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
}
