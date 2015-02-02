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
import Sailfish.Browser 1.0

SilicaGridView {
    id: favoriteGrid

    // browserPage.isPortrait ? 4 : 7
    readonly property int columns: Math.floor(parent.width / Theme.itemSizeExtraLarge)
    readonly property int initialCellWidth: (parent.width - Theme.paddingLarge * 2) / columns

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

    signal load(string url, string title)
    signal newTab(string url, string title)
    signal share(string url, string title)

    width: cellWidth * columns
    currentIndex: -1

    cellWidth: Math.round(initialCellWidth + (initialCellWidth - Theme.iconSizeLauncher) / (columns - 1))
    cellHeight: Math.round(Screen.height / 6)
    displaced: Transition { NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad; duration: 200 } }

    onVisibleChanged: {
        if (!visible && contextMenuActive) {
            contextMenu.hide()
        }
    }

    readonly property bool contextMenuActive: contextMenu && contextMenu.active
    property Item contextMenu: currentItem ? currentItem._menuItem : null
    // Figure out which delegates need to be moved down to make room
    // for the context menu when it's open.
    property int minOffsetIndex: contextMenu ? currentIndex - (currentIndex % columns) + columns : 0
    property int yOffset: contextMenu ? contextMenu.height : 0

    delegate: ListItem {
        id: container

        property Item remorse

        function remove() {
            //: Remorse timer for removing bookmark
            //% "Removing favorite"
            remorse = remorseAction(qsTrId("sailfish_browser-la-removing_favorite"),
                                    function() { bookmarkModel.removeBookmark(url) } )
            remorse.canceled.connect(function() {
                remorse = null
            })
        }

        signal addToLauncher
        signal editBookmark

        width: favoriteGrid.cellWidth
        _showPress: false
        contentHeight: favoriteGrid.cellHeight
        menu: favoriteContextMenu
        down: favoriteItem.down
        showMenuOnPressAndHold: false
        // Do not capture mouse events here. This ListItem only handles
        // menu creation and destruction.
        enabled: false

        // Update menu position upon orientation change. Delegate's x might change when
        // orientation changes as there are 4 columns in portrait and 7 in landscape.
        // This breaks binding that exists in ContextMenu.
        onXChanged: {
            if (_menuItem && _menuItem.active) {
                _menuItem.x = -x
            }
        }

        onVisibleChanged: {
            if (!visible && remorse) {
                remorse.destroy()
            }
        }

        onAddToLauncher: {
            // url, title, favicon
            pageStack.push(addToLauncherDialog,
                           {
                               "url": url,
                               "title": title,
                               "icon": favicon
                           })
        }

        onEditBookmark: {
            // index, url, title
            pageStack.push(editDialog,
                           {
                               "url": url,
                               "title": title,
                               "index": index,
                           })
        }

        FavoriteItem {
            id: favoriteItem

            width: favoriteGrid.cellWidth
            height: favoriteGrid.cellHeight
            y: index >= favoriteGrid.minOffsetIndex ? favoriteGrid.yOffset : 0.0

            onClicked: favoriteGrid.load(model.url, model.title)
            onShowContextMenuChanged: {
                if (showContextMenu) {
                    // Set currentIndex for grid to make minOffsetIndex calculation to work.
                    favoriteGrid.currentIndex = model.index
                    container.showMenu(
                                {
                                    "view": favoriteGrid,
                                    "delegate": container,
                                    "title": model.title,
                                    "url": model.url
                                })
                }
            }

            GridView.onAdd: AddAnimation { target: favoriteItem }
            GridView.onRemove: animateRemoval()
        }
    }

    FavoriteContextMenu {
        id: favoriteContextMenu
    }

    VerticalScrollDecorator {
        parent: favoriteGrid
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
            property Item webPage
            readonly property bool sameWebPage: webPage && title === webPage.title && url === webPage.url

            function handleGrabbedThumbnail(data) {
                // If web page changed, switch to default icon.
                data = sameWebPage ? data : defaultIcon
                bookmarkModel.addBookmark(url, title || url, data, false)
                webPage.onThumbnailResult.disconnect(handleGrabbedThumbnail)
                fetcher.destroy()
            }

            minimumIconSize: Theme.iconSizeMedium

            onDataChanged: {
                var canDestroy = true

                if (hasAcceptedTouchIcon) {
                    bookmarkModel.addBookmark(url, title || url, data, hasAcceptedTouchIcon)
                } else if (sameWebPage) {
                    // We are still at same web page but no accepted touch icon. Let's grab thumbnail.
                    canDestroy = false
                    webPage.onThumbnailResult.connect(handleGrabbedThumbnail)
                    webPage.grabThumbnail()
                } else {
                    // Use default icon.
                    bookmarkModel.addBookmark(url, title || url, defaultIcon, false)
                }

                if (canDestroy) {
                    fetcher.destroy()
                }
            }
        }
    }

    Component {
        id: editDialog
        BookmarkEditDialog {
            onAccepted: bookmarkModel.editBookmark(index, editedUrl, editedTitle)
        }
    }

    Component {
        id: addToLauncherDialog
        BookmarkEditDialog {
            property string icon
            property var bookmarkWriter

            //: Title of the "Add to launcher" dialog.
            //% "Add to launcher"
            title: qsTrId("sailfish_browser-he-add_bookmark_to_launcher")
            canAccept: editedUrl !== "" && editedTitle !== ""
            onAccepted: {
                bookmarkWriter = desktopBookmarkWriter.createObject(favoriteGrid)
                bookmarkWriter.save(editedUrl, editedTitle, icon)
            }
        }
    }
}
