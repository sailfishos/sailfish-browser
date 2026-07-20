/*
 * Copyright (c) 2021 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.2
import Sailfish.Silica 1.0
import "../../shared" as Shared

Rectangle {
    id: root

    readonly property real overlayOpacity: 0.15

    height: Theme.itemSizeMedium - Theme.paddingMedium
    implicitWidth: content.width
    implicitHeight: Theme.iconSizeMedium
    color: Qt.tint(
               Theme.colorScheme === Theme.LightOnDark ? "black" : "white",
               Theme.rgba(Theme.primaryColor, root.overlayOpacity))

    Grid {
        id: content

        columns: 4 // Buttons
        rows: 1

        height: root.height
        width: (Theme.itemSizeLarge + spacing) * columns - spacing
        anchors.right: parent.right

        spacing: Theme.paddingMedium

        Shared.IconButton {
            height: parent.height
            width: Theme.itemSizeLarge
            icon.source: "image://theme/icon-m-tab-close"
            icon.opacity: enabled ? 1.0 : Theme.opacityLow
            enabled: webView.tabModel.count > 0
            onTapped: {
                webView.tabModel.closeActiveTab()
                if (webView.tabModel.count === 0) {
                    overlay.startPage(PageStackAction.Animated)
                }
            }
        }

        Shared.IconButton {
            height: parent.height
            width: Theme.itemSizeLarge
            icon.source: "image://theme/icon-m-forward"
            icon.opacity: enabled ? 1.0 : Theme.opacityLow
            enabled: webView.canGoForward
            onTapped: {
                webView.goForward()
                overlay.animator.showChrome()
            }
        }

        Shared.IconButton {
            height: parent.height
            width: Theme.itemSizeLarge
            icon.source: overlay.toolBar.bookmarked ? "image://theme/icon-m-favorite-selected"
                                                    : "image://theme/icon-m-favorite"
            icon.opacity: enabled ? 1.0 : Theme.opacityLow
            enabled: webView.contentItem
            onTapped: {
                if (overlay.toolBar.bookmarked) {
                    overlay.toolBar.removeActivePageFromBookmarks()
                } else {
                    overlay.toolBar.bookmarkActivePage()
                }
            }
        }

        Shared.IconButton {
            height: parent.height
            width: Theme.itemSizeLarge
            icon.source: webView.loading ? "image://theme/icon-m-reset" : "image://theme/icon-m-refresh"
            icon.opacity: enabled ? 1.0 : Theme.opacityLow
            enabled: webView.contentItem
            onTapped: {
                if (webView.loading) {
                    webView.stop()
                } else {
                    webView.reload()
                }
                overlay.animator.showChrome()
            }
        }
    }
}
