import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0

ListItem {
    id: root

    property string search

    function remove() {
        view.model.remove(model.index)
    }

    function openLink() {
        webView.tabModel.newTab(model.url)
        pageStack.pop()
    }

    width: view.width
    contentHeight: Math.max(Theme.itemSizeMedium, column.height + 2*Theme.paddingMedium)
    menu: contextMenuComponent

    ListView.onAdd: AddAnimation { target: root }

    Row {
        id: row
        width: parent.width

        leftPadding: Theme.horizontalPageMargin
        rightPadding: Theme.horizontalPageMargin

        spacing: Theme.paddingMedium

        Column {
            id: column

            Label {
                id: hostLabel
                textFormat: Text.StyledText
                font.pixelSize: Theme.fontSizeSmall
                truncationMode: TruncationMode.Fade
                width: root.width - deleteButton.width - 2*Theme.horizontalPageMargin
                text: Theme.highlightText(model.url, search, Theme.highlightColor)
                opacity: Theme.opacityHigh
            }
            Label {
                textFormat: Text.StyledText
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.WordWrap
                text: Theme.highlightText(model.title, search, Theme.highlightColor)
                width: hostLabel.width
            }
        }

        IconButton {
            id: deleteButton
            icon.source: "image://theme/icon-m-clear"
            anchors.top: column.top
            onClicked: remove()
        }
    }

    ListView.onRemove: animateRemoval()
    onClicked: openLink()

    Component {
        id: contextMenuComponent

        ContextMenu {
            MenuItem {
                //: Share link from browser history pulley menu
                //% "Share"
                text: qsTrId("sailfish_browser-me-share-link")
                onClicked: pageStack.animatorPush("Sailfish.WebView.Popups.ShareLinkPage",
                                                  {"link" : model.url, "linkTitle": model.title})
            }
            MenuItem {
                //% "Copy to clipboard"
                text: qsTrId("sailfish_browser-me-copy-to-clipboard")
                onClicked: Clipboard.text = model.url
            }

            MenuItem {
                //: Delete history entry
                //% "Delete"
                text: qsTrId("sailfish_browser-me-delete")
                onClicked: remove()
            }
        }
    }
}
