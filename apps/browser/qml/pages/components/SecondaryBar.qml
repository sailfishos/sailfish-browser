/****************************************************************************
**
** Copyright (c) 2014 - 2015 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import org.nemomobile.dbus 2.0
import "." as Browser
import "../../shared" as Shared

Column {
    id: root
    property bool bookmarked
    property int horizontalOffset
    property int iconWidth
    property real midIconWidth: iconWidth + (iconWidth - forwardButton.width) / 4

    width: parent.width
    height: isPortrait ? Settings.toolbarLarge : Settings.toolbarSmall
    clip: opacity < 1.0

    OverlayListItem {
        height: overlay.toolBar.rowHeight

        iconWidth: root.iconWidth
        horizontalOffset: root.horizontalOffset
        iconSource: "image://theme/icon-m-favorite-selected"
        //% "Bookmarks"
        text: qsTrId("sailfish_browser-la-bookmarks")

        onClicked: {
            showChrome()
            pageStack.push("../BookmarkPage.qml", { bookmarkModel: bookmarkModel })
        }
    }

    OverlayListItem {
        height: overlay.toolBar.rowHeight

        iconWidth: root.iconWidth
        horizontalOffset: root.horizontalOffset
        //% "History"
        text: qsTrId("sailfish_browser-la-history")
        iconSource: "image://theme/icon-m-history"

        onClicked: {
            showChrome()
            var historyPage = pageStack.push("../HistoryPage.qml", { model: historyModel })
            historyPage.loadPage.connect(loadPage)
        }
    }

    OverlayListItem {
        height: overlay.toolBar.rowHeight

        iconWidth: root.iconWidth
        horizontalOffset: root.horizontalOffset
        //% "Settings"
        text: qsTrId("sailfish_browser-la-setting")
        iconSource: "image://theme/icon-m-setting"

        onClicked: {
            showChrome()
            pageStack.push(Qt.resolvedUrl("../SettingsPage.qml"))
        }
    }

    Row {
        width: parent.width
        height: overlay.toolBar.rowHeight

        Browser.TabButton {
            id: addTabButton
            width: iconWidth + horizontalOffset
            horizontalOffset: root.horizontalOffset
            icon.source: webView.privateMode ? "image://theme/icon-m-incognito-new" : "image://theme/icon-m-tabs"
            label.text: webView.privateMode ? "" : "+"
            onTapped: enterNewTabUrl()
        }

        Shared.ExpandingButton {
            id: forwardButton
            expandedWidth: iconWidth
            icon.source: "image://theme/icon-m-forward"
            active: webView.canGoForward
            onTapped: webView.goForward()
        }

        // Spacer for pushing Search, Bookmark, Share, Downloads to the right hand side
        Item {
            height: parent.height
            width: parent.width - addTabButton.width - forwardButton.width - midIconWidth * 4 - downloadsButton.width
        }

        Shared.IconButton {
            width: midIconWidth
            icon.source: "image://theme/icon-m-search"
            active: webView.contentItem
            onTapped: {
                findInPageActive = true
                findInPage()
            }
        }

        Shared.IconButton {
            width: midIconWidth
            icon.source: bookmarked ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"
            active: webView.contentItem
            onClicked: {
                if (bookmarked) {
                    removeActivePageFromBookmarks()
                } else {
                    bookmarkActivePage()
                }
            }
        }

        Shared.IconButton {
            width: midIconWidth
            icon.source: "image://theme/icon-m-share"
            active: webView.contentItem
            onTapped: shareActivePage()
        }

        Shared.IconButton {
            width: midIconWidth
            icon.source: "image://theme/icon-m-file-download-as-pdf"
            active: webView.contentItem && webView.contentItem.active && !webView.loading
            onTapped: {
                if (DownloadManager.pdfPrinting) {
                    pdfPrintingNotice.show()
                } else {
                    savePageAsPDF()
                }
            }
        }

        Shared.IconButton {
            id: downloadsButton
            width: iconWidth + horizontalOffset
            icon.source: "image://theme/icon-m-transfer"
            icon.anchors.horizontalCenterOffset: -horizontalOffset
            onTapped: settingsApp.call("showTransfers", [])
        }
    }

    Notice {
        id: pdfPrintingNotice
        duration: 3000
        //% "Already saving pdf"
        text: qsTrId("sailfish_browser-la-already_printing_pdf")
        verticalOffset: -overlay.toolBar.rowHeight * overlay.toolBar.maxRowCount
    }

    DBusInterface {
        id: settingsApp
        service: "com.jolla.settings"
        iface: "com.jolla.settings.ui"
        path: "/com/jolla/settings/ui"
    }
}
