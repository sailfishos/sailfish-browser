import QtQuick 1.1
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    property string text

    DialogHeader {
        //: Text on the Accept dialog button that accepts browser alert messages
        //% "Ok"
        acceptText: qsTrId("sailfish_browser-he-accept_alert")
    }

    Label {
        anchors {
            verticalCenter: parent.verticalCenter
            horizontalCenter: parent.horizontalCenter
        }
        text: dialog.text
    }
}