import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0

ListItem {
    id: root

    property string search
    property bool showDeleteButton

    width: parent.width
    contentHeight: Math.max(Theme.itemSizeMedium, column.height + 2*Theme.paddingMedium)

    ListView.onAdd: AddAnimation { target: root }

    Row {
        id: row
        x: Theme.horizontalPageMargin
        width: parent.width  - Theme.horizontalPageMargin * 2
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.paddingMedium

        Column {
            id: column
            width: parent.width - (deleteButton.visible ? deleteButton.width : 0)
            Label {
                text: Theme.highlightText(model.title, search, Theme.highlightColor)
                textFormat: Text.StyledText
                font.pixelSize: Theme.fontSizeSmall
                truncationMode: TruncationMode.Fade
                width: parent.width
            }

            Label {
                text: Theme.highlightText(model.url, search, Theme.highlightColor)
                textFormat: Text.StyledText
                opacity: Theme.opacityHigh
                font.pixelSize: Theme.fontSizeSmall
                truncationMode: TruncationMode.Fade
                width: parent.width
            }
        }

        IconButton {
            id: deleteButton
            visible: showDeleteButton
            icon.source: "image://theme/icon-m-clear"
            onClicked: view.model.remove(model.index)
        }
    }

    ListView.onRemove: animateRemoval()
    onClicked: view.load(model.url, model.title)
}
