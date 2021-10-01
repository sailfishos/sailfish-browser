/****************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0

ListItem {
    id: root

    property string search
    property bool showDeleteButton

    function remove(url) {
        remorseDelete(function() {
            view.model.remove(url)
        })
    }

    width: parent.width
    contentHeight: Math.max(Theme.itemSizeMedium, column.height + 2*Theme.paddingMedium)
    menu: Component {
        ContextMenu {
            MenuItem {
                //% "Open in normal mode tab"
                text: webView.privateMode ? qsTrId("sailfish_browser-me-open-in-normal-mode-tab")
                                          : //% "Open in private tab"
                                            qsTrId("sailfish_browser-me-open-in-private-tab")
                onClicked: view.load(model.url, model.title, true)
            }
            MenuItem {
                //: Share link from browser history pulley menu
                //% "Share"
                text: qsTrId("sailfish_browser-me-share-link")
                onClicked: webShareAction.shareLink(model.url, model.title)
            }
            MenuItem {
                //% "Copy to clipboard"
                text: qsTrId("sailfish_browser-me-copy-to-clipboard")
                onClicked: {
                    Clipboard.text = model.url
                    //% "Link address copied"
                    notification.text = qsTrId("sailfish_browser-me-link_address_copied")
                    notification.show()
                }
            }
            MenuItem {
                //% "Add to bookmarks"
                text: qsTrId("sailfish_browser-me-add-to-bookmarks")
                onClicked: view.saveBookmark(model.url, model.title, model.favicon)
            }
            MenuItem {
                //: Delete history entry
                //% "Delete"
                text: qsTrId("sailfish_browser-me-delete")
                onClicked: historyDelegate.remove(model.url)
            }
        }
    }

    ListView.onAdd: AddAnimation { target: root }

    Notice {
        id: notification

        duration: Notice.Short
        verticalOffset: -Theme.itemSizeMedium
    }

    Row {
        id: row
        x: Theme.horizontalPageMargin
        width: parent.width  - Theme.horizontalPageMargin * 2
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.paddingMedium

        FavoriteIcon {
            id: websiteIcon
            icon: model.favicon
            sourceSize.width: Theme.iconSizeMedium
            sourceSize.height: Theme.iconSizeMedium
            width: height
            height: column.height
        }

        Column {
            id: column
            width: parent.width - websiteIcon.width - (deleteButton.visible ? deleteButton.width : 0)
            Label {
                text: Theme.highlightText(model.title, search, Theme.highlightColor)
                textFormat: Text.StyledText
                color: highlighted ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
                truncationMode: TruncationMode.Fade
                width: parent.width
            }

            Label {
                text: Theme.highlightText(model.url, search, Theme.highlightColor)
                textFormat: Text.StyledText
                color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                truncationMode: TruncationMode.Fade
                width: parent.width
            }
        }

        IconButton {
            id: deleteButton
            visible: showDeleteButton
            icon.source: "image://theme/icon-m-clear"
            onClicked: remove(model.url)
        }
    }

    ListView.onRemove: animateRemoval()
    onClicked: view.load(model.url, model.title, false)
}
