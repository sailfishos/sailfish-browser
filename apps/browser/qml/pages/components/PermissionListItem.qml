import QtQuick 2.0
import Sailfish.Silica 1.0

ListItem {
    id: root

    property alias label: valueButton.label
    property alias value: valueButton.value
    property alias iconSource: icon.source

    width: parent.width
    contentHeight: Theme.itemSizeMedium
    onClicked: openMenu()

    Row {
        width: parent.width - 2*Theme.horizontalPageMargin
        x: Theme.horizontalPageMargin
        anchors.verticalCenter: parent.verticalCenter

        Icon {
            id: icon

            anchors.verticalCenter: parent.verticalCenter
        }

        ValueButton {
            id: valueButton

            width: parent.width - icon.width
            anchors.verticalCenter: parent.verticalCenter
            labelColor: root.highlighted ? Theme.highlightColor : Theme.primaryColor
            valueColor: Theme.highlightColor
            opacity: 1.0
            enabled: false
        }
    }
}
