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

Loader {
    id: root

    property Component header
    property bool showFavorites
    property QtObject model
    readonly property real contentY: item ? item.contentY : 0
    readonly property Item contextMenu: item ? item.contextMenu : null
    readonly property bool moving: item && item.moving
    readonly property int count: item ? item.count : 0

    signal load(string url, string title)
    signal newTab(string url, string title)
    signal share(string url, string title)

    function positionViewAtBeginning() {
        if (item) {
            item.positionViewAtBeginning()
        }
    }

    function fetchAndSaveBookmark() {
        var webPage = webView && webView.contentItem
        if (webPage) {
            // Fetcher itself does async fetching. No need to create this asynchronously.
            var fetcher = iconFetcher.createObject(root,
                                                   {
                                                       "url": webPage.url,
                                                       "title": webPage.title,
                                                       "webPage": webPage
                                                   })
            fetcher.fetch(webPage.favicon)
        }
    }

    onActiveChanged: if (active) active = true // remove binding
    active: showFavorites
    asynchronous: true

    sourceComponent: SilicaGridView {
        id: favoriteGrid

        readonly property real pageHeight: Math.ceil(browserPage.height + pageStack.panelSize)

        readonly property Item contextMenu: currentItem ? currentItem._menuItem : null
        readonly property bool contextMenuActive: contextMenu && contextMenu.active

        // Figure out which delegates need to be moved down to make room
        // for the context menu when it's open.
        readonly property int minOffsetIndex: contextMenu ? currentIndex - (currentIndex % columns) + columns : 0
        readonly property int yOffset: contextMenu ? contextMenu.height : 0

        readonly property int rows: Math.floor(pageHeight / minimumCellHeight)
        readonly property int columns: Math.floor(browserPage.width / minimumCellWidth)

        readonly property int horizontalMargin: largeScreen ? 6 * Theme.paddingLarge : Theme._homePageMargin
        readonly property int initialCellWidth: (browserPage.width - 2*horizontalMargin) / columns

        // The multipliers below for Large screens are magic. They look good on Jolla tablet.
        readonly property real minimumCellWidth: largeScreen ? Theme.itemSizeExtraLarge * 1.6 : Theme.itemSizeExtraLarge
        // phone reference row height: 960 / 6
        readonly property real minimumCellHeight: largeScreen ? Theme.itemSizeExtraLarge * 1.6 : Theme.pixelRatio * 160

        header: root.header
        model: root.model
        width: cellWidth * columns
        currentIndex: -1
        anchors.horizontalCenter: parent.horizontalCenter
        _quickScrollRightMargin: -(browserPage.width - width) / 2
        cellWidth: Math.floor(initialCellWidth + (initialCellWidth - Theme.iconSizeLauncher) / (columns - 1))
        cellHeight: Math.round(pageHeight / rows)

        displaced: Transition { NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad; duration: 200 } }
        cacheBuffer: cellHeight * 2

        onVisibleChanged: {
            if (!visible && contextMenuActive) {
                contextMenu.hide()
            }
        }

        delegate: ListItem {
            id: container

            property real offsetY: largeScreen
                                   ? - (((-favoriteGrid.originY+container.contentHeight/2)%favoriteGrid.pageHeight)/favoriteGrid.pageHeight - 0.5) * (Theme.paddingLarge*4)
                                   : 0

            signal addToLauncher
            signal editBookmark

            width: favoriteGrid.cellWidth
            _showPress: false
            contentHeight: favoriteGrid.cellHeight
            menu: favoriteContextMenu
            down: favoriteItem.down
            openMenuOnPressAndHold: false
            // Do not capture mouse events here. This ListItem only handles
            // menu creation and destruction.
            enabled: false

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

                onClicked: root.load(model.url, model.title)
                onShowContextMenuChanged: {
                    if (showContextMenu) {
                        // Set currentIndex for grid to make minOffsetIndex calculation to work.
                        favoriteGrid.currentIndex = model.index
                        container.openMenu(
                                    {
                                        "view": root,
                                        "delegate": container,
                                        "title": model.title,
                                        "url": model.url,
                                        "index": model.index
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
}
