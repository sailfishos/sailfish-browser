/****************************************************************************
**
** Copyright (c) 2014 - 2021 Jolla Ltd.
** Copyright (c) 2021 Open Mobile Platform LLC.
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

    property real menuHeight
    property int footerHeight: Theme.itemSizeLarge

    signal load(string url)
    signal newTab(string url)
    signal share(string url, string title)

    pageHeight: Math.ceil(browserPage.height + pageStack.panelSize)
    rows: Math.floor(pageHeight / minimumCellHeight)
    columns: Math.floor(browserPage.width / minimumCellWidth)

    function fetchAndSaveBookmark() { fetchAndSaveHostedBookmark() }

    function fetchAndSaveHostedBookmark() {
        var hostView = browserPage.chromeHostView
        if (!hostView || !hostView.selectedTabId.length) {
            return
        }
        var tab = browserPage.hostedRuntimeTabByRuntimeId(
                    hostView, hostView.selectedTabId)
        if (!tab || !String(tab.persistentId).length) {
            return
        }

        var url = String(tab.location)
        var title = browserPage.title || url
        var fetcher = hostedIconFetcher.createObject(favoriteGrid, {
                                                         "hostView": hostView,
                                                         "tabId": String(tab.tabId),
                                                         "persistentId": String(tab.persistentId),
                                                         "location": url,
                                                         "locationRevision": String(tab.locationRevision),
                                                         "title": title
                                                     })
        if (fetcher) {
            fetcher.fetch(browserPage._hostedFavicon)
        }
    }

    currentIndex: -1

    displaced: Transition { NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad; duration: 200 } }
    cacheBuffer: cellHeight * 2

    footer: Item {
        width: 1
        height: favoriteGrid.footerHeight
    }

    delegate: FavoriteItem {
        id: favoriteItem

        signal addToLauncher
        signal editBookmark

        menu: favoriteContextMenu
        openMenuOnPressAndHold: false

        onMenuOpenChanged: {
            favoriteGrid.menuHeight = (menuOpen && historyContainer.showHistoryList)
                                      ? cellHeight : 0
        }

        onAddToLauncher: {
            // url, title, favicon
            pageStack.animatorPush("AddToAppGridDialog.qml",
                                   {
                                       "url": url,
                                       "title": title,
                                       "icon": favicon,
                                       "desktopBookmarkWriter": desktopBookmarkWriter,
                                       "bookmarkWriterParent": pageStack
                                   })
        }

        onEditBookmark: {
            // index, url, title
            pageStack.animatorPush(editDialog,
                                   {
                                       // Defined in BookmarkItem.qml
                                       // "Edit bookmark"
                                       "description": qsTrId("sailfish_browser-he-edit-bookmark"),
                                       "url": url,
                                       "title": title,
                                       "index": index,
                                   })
        }

        onClicked: favoriteGrid.load(model.url, model.title)
        onShowContextMenuChanged: {
            if (showContextMenu) {
                openMenu({
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
        id: hostedIconFetcher

        DataFetcher {
            id: hostedFetcher

            property var hostView
            property string tabId
            property string persistentId
            property string location
            property string locationRevision
            property string title
            property bool fetchingThumbnail
            property bool waitingForThumbnail

            function currentTab() {
                var tab = browserPage.hostedRuntimeTabByRuntimeId(hostView, tabId)
                return tab && String(tab.persistentId) === persistentId
                        && String(tab.location) === location
                        && String(tab.locationRevision) === locationRevision
            }

            function stopWaitingForThumbnail() {
                if (waitingForThumbnail) {
                    browserPage.hostedThumbnailUpdated.disconnect(
                                handleHostedThumbnail)
                    waitingForThumbnail = false
                }
                thumbnailWaitTimer.stop()
            }

            function finish(iconData, touchIcon) {
                stopWaitingForThumbnail()
                if (currentTab()) {
                    bookmarkModel.updateFavoriteIcon(location, iconData,
                                                     touchIcon)
                }
                destroy()
            }

            function handleHostedThumbnail(capturedPersistentId,
                                           capturedLocation,
                                           capturedLocationRevision,
                                           fileName) {
                if (capturedPersistentId !== persistentId
                        || capturedLocation !== location
                        || capturedLocationRevision !== locationRevision) {
                    return
                }
                stopWaitingForThumbnail()
                fetchingThumbnail = true
                fetch("file://" + fileName)
            }

            minimumIconSize: Theme.iconSizeSmallPlus

            onDataChanged: {
                if (fetchingThumbnail) {
                    finish(data, false)
                } else if (hasAcceptedTouchIcon) {
                    finish(data, true)
                } else if (!waitingForThumbnail && currentTab()) {
                    waitingForThumbnail = true
                    browserPage.hostedThumbnailUpdated.connect(
                                handleHostedThumbnail)
                    thumbnailWaitTimer.restart()
                    browserPage.captureHostedThumbnail()
                } else if (!currentTab()) {
                    finish(data, false)
                }
            }

            Component.onCompleted: {
                // Add immediately, then replace the
                // placeholder with a durable fetched data URI asynchronously.
                bookmarkModel.add(location, title || location, defaultIcon, true)
            }

            Component.onDestruction: stopWaitingForThumbnail()

            property Timer thumbnailWaitTimer: Timer {
                interval: 2000
                onTriggered: hostedFetcher.finish(hostedFetcher.data, false)
            }
        }
    }

    Component {
        id: editDialog
        BookmarkEditDialog {
            onAccepted: bookmarkModel.edit(index, editedUrl, editedTitle)
        }
    }
}
