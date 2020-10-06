/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.private 1.0
import Sailfish.Browser 1.0

IconGridViewBase {
    id: favoriteGrid

    pageHeight: Math.ceil(browserPage.height + pageStack.panelSize)
    rows: Math.floor(pageHeight / minimumCellHeight)
    columns: Math.floor(browserPage.width / minimumCellWidth)

    signal load(string url)
    signal newTab(string url)
    signal share(string url, string title)

    function fetchAndSaveBookmark() {
        var webPage = webView && webView.contentItem
        if (webPage) {
            // Fetcher itself does async fetching. No need to create this asynchronously.
            var fetcher = iconFetcher.createObject(favoriteGrid,
                                                   {
                                                       "url": webPage.url,
                                                       "title": webPage.title,
                                                       "webPage": webPage
                                                   })
            fetcher.fetch(webPage.favicon)
        }
    }

    currentIndex: -1

    displaced: Transition { NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad; duration: 200 } }
    cacheBuffer: cellHeight * 2

    footer: Item {
        width: 1
        height: Theme.itemSizeLarge
    }

    delegate: FavoriteItem {
        id: favoriteItem

        signal addToLauncher
        signal editBookmark

        menu: favoriteContextMenu
        openMenuOnPressAndHold: false

        onAddToLauncher: {
            // url, title, favicon
            pageStack.animatorPush(addToLauncherDialog,
                                   {
                                       "url": url,
                                       "title": title,
                                       "icon": favicon
                                   })
        }

        onEditBookmark: {
            // index, url, title
            pageStack.animatorPush(editDialog,
                                   {
                                       "url": url,
                                       "title": title,
                                       "index": index,
                                   })
        }


        onClicked: favoriteGrid.load(model.url, model.title)
        onShowContextMenuChanged: {
            if (showContextMenu) {
                openMenu(
                            {
                                "view": favoriteGrid,
                                "delegate": favoriteItem,
                                "title": model.title,
                                "url": model.url,
                                "index": model.index
                            })
            }
        }

        GridView.onAdd: AddAnimation { target: favoriteItem }
    }


    FavoriteContextMenu {
        id: favoriteContextMenu
    }

    VerticalScrollDecorator {
        parent: favoriteGrid
        anchors.rightMargin: -(browserPage.width - favoriteGrid.width) / 2
        flickable: favoriteGrid
    }

    Component {
        id: desktopBookmarkWriter
        DesktopBookmarkWriter {
            onSaved: destroy()
        }
    }

    Component {
        id: iconFetcher

        IconFetcher {
            id: fetcher
            property url url
            property string title
            property var webPage
            readonly property bool sameWebPage: webPage && title === webPage.title && url === webPage.url

            function handleGrabbedThumbnail(data) {
                // If on the same web page, update thumbnail data.
                if (sameWebPage) {
                    bookmarkModel.updateFavoriteIcon(url, data, false)
                }
                webPage.onThumbnailResult.disconnect(handleGrabbedThumbnail)
                fetcher.destroy()
            }

            minimumIconSize: Theme.iconSizeSmallPlus

            onDataChanged: {
                var canDestroy = true
                if (hasAcceptedTouchIcon) {
                    bookmarkModel.updateFavoriteIcon(url, data, hasAcceptedTouchIcon)
                } else if (sameWebPage) {
                    // We are still at same web page but no accepted touch icon. Let's grab thumbnail.
                    canDestroy = false
                    webPage.onThumbnailResult.connect(handleGrabbedThumbnail)
                    webPage.grabThumbnail(Qt.size(favoriteGrid.cellHeight, favoriteGrid.cellWidth))
                }

                if (canDestroy) {
                    fetcher.destroy()
                }
            }

            Component.onCompleted: {
                // Add bookmark immediately with the defaultIcon. Update the favorite
                // asynchronously.
                bookmarkModel.add(url, title || url, defaultIcon, true)
            }
        }
    }

    Component {
        id: editDialog
        BookmarkEditDialog {
            onAccepted: bookmarkModel.edit(index, editedUrl, editedTitle)
        }
    }

    Component {
        id: addToLauncherDialog
        BookmarkEditDialog {
            property string icon
            property var bookmarkWriter

            //: Title of the "Add to App Grid" dialog.
            //% "Add to App Grid"
            title: qsTrId("sailfish_browser-he-add_bookmark_to_launcher")
            canAccept: editedUrl !== "" && editedTitle !== ""
            onAccepted: {
                bookmarkWriter = desktopBookmarkWriter.createObject(favoriteGrid)
                bookmarkWriter.save(editedUrl, editedTitle, icon)
            }
        }
    }
}
