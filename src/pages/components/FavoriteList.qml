/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/
import QtQuick 2.1
import Sailfish.Silica 1.0

SilicaListView {
    id: view

    property bool hasContextMenu

    signal load(string url, string title, bool newTab)
    signal removeBookmark(string url)

    anchors.fill: parent

    delegate: ListItem {
        id: favoriteDelegate

        function remove() {
            //: Remorse timer for removing bookmark
            //% "Removing favorite"
            remorse.execute(favoriteDelegate, qsTrId("sailfish_browser-la-removing_favorite"),
                            function() { view.removeBookmark(url) } )
        }

        width: view.width
        menu: hasContextMenu ? favoriteContextMenuComponent : null
        showMenuOnPressAndHold: menu !== null
        ListView.onRemove: animateRemoval()
        onClicked: view.load(model.url, model.title, false)

        Row {
            height: parent.height
            spacing: Theme.paddingMedium
            anchors {
                left: parent.left
                right: parent.right
                leftMargin: Theme.paddingLarge
                rightMargin: Theme.paddingLarge
            }

            FaviconImage {
                id: faviconImage
                anchors.verticalCenter: titleLabel.verticalCenter
                favicon: model.favicon
                link: url
            }

            Label {
                id: titleLabel
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - faviconImage.width
                text: title
                color: highlighted ? Theme.highlightColor : Theme.primaryColor
                truncationMode: TruncationMode.Fade
            }
        }

        RemorseItem { id: remorse }

        Component {
            id: favoriteContextMenuComponent
            ContextMenu {
                MenuItem {
                    //% "Open in new tab"
                    text: qsTrId("sailfish_browser-me-open_new_tab")
                    onClicked: view.load(url, title, true)
                }

                MenuItem {
                    //: "Remove favorited / bookmarked web page"
                    //% "Remove favorite"
                    text: qsTrId("sailfish_browser-me-remove_favorite")
                    onClicked: favoriteDelegate.remove()
                }
            }
        }
    }

    VerticalScrollDecorator {}
}
