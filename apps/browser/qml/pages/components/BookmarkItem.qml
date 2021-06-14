/****************************************************************************
**
** Copyright (c) 2020 - 2021 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import Sailfish.WebView.Popups 1.0 as Popups

ListItem {
    id: root

    function openLink() {
        webView.tabModel.newTab(model.url)
        pageStack.pop()
    }

    function remove(url) {
        remorseDelete(function() {
            bookmarkModel.remove(url)
        })
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
            color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
            font.pixelSize: Theme.fontSizeSmall
            truncationMode: TruncationMode.Fade
            width: parent.width
        }
    }

    ListView.onRemove: animateRemoval()
    onClicked: openLink()

    Component {
        id: contextMenuComponent
        ContextMenu {
            MenuItem {
                //% "Share"
                text: qsTrId("sailfish_browser-me-share-link")
                onClicked: webShareAction.shareLink(model.url, model.title)
            }
            MenuItem {
                //% "Copy to clipboard"
                text: qsTrId("sailfish_browser-me-copy-to-clipboard")
                onClicked: Clipboard.text = model.url
            }
            MenuItem {
                text: qsTrId("sailfish_browser-me-add_to_launcher")
                onClicked: pageStack.animatorPush("AddToAppGridDialog.qml",
                                                  {
                                                      "url": url,
                                                      "title": title,
                                                      "icon": favicon,
                                                      "desktopBookmarkWriter": desktopBookmarkWriter,
                                                      "bookmarkWriterParent": pageStack
                                                  })
            }
            MenuItem {
                // Defined in FavoriteContextMenu.qml
                // "Edit"
                text: qsTrId("sailfish_browser-me-edit")
                onClicked: {
                    var page = pageStack.animatorPush(editDialog,
                                           {
                                               //% "Edit bookmark"
                                               "description": qsTrId("sailfish_browser-he-edit-bookmark"),
                                               "url": url,
                                               "title": title,
                                               "index": bookmarkFilterModel.getIndex(model.index)
                                           })
                }
            }
            MenuItem {
                //% "Delete"
                text: qsTrId("sailfish_browser-me-delete")
                onClicked: root.remove(model.url)
            }
        }
    }

    Popups.WebShareAction {
        id: webShareAction
    }

    Component {
        id: desktopBookmarkWriter
        DesktopBookmarkWriter {
            onSaved: destroy()
        }
    }

    Component {
        id: editDialog
        BookmarkEditDialog {
            onAccepted: bookmarkModel.edit(index, editedUrl, editedTitle)
        }
    }
}
