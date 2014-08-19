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

SilicaGridView {
    id: favoriteGrid
    property int columns: 4
    property int initialCellWidth: (parent.width - Theme.paddingLarge * 2) / columns

    signal load(string url, string title)
    // Do we need to at all
    signal newTab(string url, string title)
    signal removeBookmark(string url)
    signal editBookmark(int index, string url, string title)
    signal share(string url, string title)
    signal addToLauncher(string url, string title, string favicon)

    clip: true
    width: cellWidth * columns
    currentIndex: -1
    pressDelay: 50

    cellWidth: Math.round(initialCellWidth + (initialCellWidth - Theme.iconSizeLauncher) / (columns - 1))
    cellHeight: Math.round(Screen.height / 6)
    displaced: Transition { NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad; duration: 200 } }

    property Item contextMenu: currentItem ? currentItem._menuItem : null
    // Figure out which delegates need to be moved down to make room
    // for the context menu when it's open.
    property int minOffsetIndex: contextMenu ? currentIndex - (currentIndex % columns) + columns : 0
    property int yOffset: contextMenu ? contextMenu.height : 0

    delegate: ListItem {
        id: container

        width: favoriteGrid.cellWidth
        _showPress: false
        contentHeight: favoriteGrid.cellHeight
        menu: favoriteContextMenu
        down: favoriteItem.down
        showMenuOnPressAndHold: false

        function remove() {
            //: Remorse timer for removing bookmark
            //% "Removing favorite"
            remorse.execute(container, qsTrId("sailfish_browser-la-removing_favorite"),
                            function() { favoriteGrid.removeBookmark(url) } )
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
                                    "index": model.index,
                                    "title": model.title,
                                    "url": model.url,
                                    "favicon": model.favicon
                                })
                }
            }

            GridView.onAdd: AddAnimation { target: favoriteItem }
            GridView.onRemove: animateRemoval()

            RemorseItem { id: remorse }
        }
    }

    FavoriteContextMenu {
        id: favoriteContextMenu
    }

    VerticalScrollDecorator {
        parent: favoriteGrid
        flickable: favoriteGrid
    }
}
