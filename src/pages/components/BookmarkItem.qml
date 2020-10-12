/****************************************************************************
**
** Copyright (C) 2020 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0

ListItem {
    id: root

    function openLink() {
        webView.tabModel.newTab(model.url)
        pageStack.pop()
    }

    contentHeight: Math.max(Theme.itemSizeMedium, column.height + 2*Theme.paddingMedium)

    menu: contextMenuComponent

    ListView.onAdd: AddAnimation { target: root }

    FavoriteIcon {
        id: favoriteIcon

        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
        }

        icon: model.favicon

        sourceSize.width: Theme.iconSizeMedium
        sourceSize.height: Theme.iconSizeMedium
        width: Theme.iconSizeMedium
        height: Theme.iconSizeMedium
    }

    Column {
        id: column
        anchors {
            verticalCenter: parent.verticalCenter
            left: favoriteIcon.right
            leftMargin: Theme.paddingMedium
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
        }

        Label {
            id: titleText
            text: Theme.highlightText(title, searchText, Theme.highlightColor)
            textFormat: Text.StyledText
            color: highlighted ? Theme.highlightColor : Theme.primaryColor
            font.pixelSize: Theme.fontSizeSmall
            truncationMode: TruncationMode.Fade
            width: parent.width
        }

        Label {
            text: Theme.highlightText(url, searchText, Theme.highlightColor)
            textFormat: Text.StyledText
            opacity: Theme.opacityHigh
            color: highlighted ? Theme.highlightColor : Theme.primaryColor
            font.pixelSize: Theme.fontSizeSmall
            truncationMode: TruncationMode.Fade
            width: parent.width
        }
    }

    ListView.onRemove: animateRemoval()
    onClicked: openLink()

    Component {
        id: editDialog
        BookmarkEditDialog {
            onAccepted: bookmarkModel.edit(index, editedUrl, editedTitle)
        }
    }

    Component {
        id: contextMenuComponent
        ContextMenu {
            MenuItem {
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
                //% "Edit"
                text: qsTrId("sailfish_browser-me-edit")
                onClicked: {
                    var page = pageStack.animatorPush(editDialog,
                                           {
                                               "url": url,
                                               "title": title,
                                               "index": bookmarkFilterModel.getIndex(model.index)
                                           })
                }
            }
            MenuItem {
                //% "Delete"
                text: qsTrId("sailfish_browser-me-delete")
                onClicked: bookmarkModel.remove(model.url)
            }
        }
    }
}
