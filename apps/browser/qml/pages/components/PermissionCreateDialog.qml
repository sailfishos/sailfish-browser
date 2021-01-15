import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.WebView.Controls 1.0

Dialog {
    property alias label: valueButton.label
    property alias iconSource: valueButton.iconSource
    readonly property int capability: indexToCapability(valueButton.capability)
    property alias uri: textField.text
    readonly property var _regExp: new RegExp("^(http|https):\/\/")

    function indexToCapability(capability) {
        if (capability === "allow") {
            return PermissionManager.Allow
        } else {
            return PermissionManager.Deny
        }
    }

    canAccept: (textField.text !== "" && !textField.errorHighlight)

    DialogHeader {
        id: header

        //: Accept button text for adding a new permission
        //% "Add"
        acceptText: qsTrId("sailfish_browser-he-add")
    }

    Column {
        width: parent.width
        anchors.top: header.bottom

        TextField {
            id: textField

            width: parent.width
            //% "Enter the exact address of the site you want to have permission exception."
            placeholderText: qsTrId("sailfish_browser-he-website-address")
            text: "https://"
            label: {
                if (errorHighlight) {
                    //% "Must begin with http:// or https://"
                    return qsTrId("sailfish_browser-he-error-website")
                } else if (text == "") {
                    return ""
                } else {
                    return placeholderText
                }
            }
            inputMethodHints: Qt.ImhUrlCharactersOnly

            errorHighlight: !_regExp.test(text) && text !== ""
        }

        PermissionListItem {
            id: valueButton

            property string capability

            value: {
                if (capability === "allow") {
                    return qsTrId("sailfish_browser-me-allow")
                } else {
                    return qsTrId("sailfish_browser-me-block")
                }
            }

            menu: ContextMenu {
                MenuItem {
                    //% "Allow"
                    text: qsTrId("sailfish_browser-me-allow")
                    onClicked: valueButton.capability = "allow"
                }
                MenuItem {
                    //% "Block"
                    text: qsTrId("sailfish_browser-me-block")
                    onClicked: valueButton.capability = "block"
                }
            }
        }
    }
}
