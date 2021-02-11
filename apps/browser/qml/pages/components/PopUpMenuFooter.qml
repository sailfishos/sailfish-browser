/*
 * Copyright (c) 2021 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.2
import Sailfish.Silica 1.0
import QtGraphicalEffects 1.0
import "../../shared" as Shared

Rectangle {
    id: root
    property int iconWidth
    property int toolBarPortraitHeight
    readonly property real overlayOpacity: 0.15

    height: Theme.itemSizeMedium
    implicitWidth: content.width + Theme.paddingMedium * 2
    color: Theme.colorScheme === Theme.LightOnDark ? "black" : "white"

    ColorOverlay {
        anchors.fill: parent
        source: parent
        color: Theme.primaryColor
        opacity: overlayOpacity
    }

    Row {
        id: content
        height: parent.height
        spacing: Theme.paddingLarge + Theme.paddingSmall + Theme.paddingSmall / 2
        anchors.centerIn: parent

        Shared.IconButton {
            id: forwardButton
            width: Theme.itemSizeSmall
            icon.source: "image://theme/icon-m-forward"
            icon.opacity: enabled ? 1.0 : Theme.opacityLow
            enabled: webView.canGoForward
            onTapped: {
                webView.goForward()
                overlay.animator.showChrome()
            }
        }

        Shared.IconButton {
            id: shareButton
            width: Theme.itemSizeSmall
            icon.source: "image://theme/icon-m-share"
            icon.opacity: enabled ? 1.0 : Theme.opacityLow
            enabled: webView.contentItem
            onTapped: {
                overlay.toolBar.shareActivePage()
                overlay.animator.showChrome()
            }
        }

        Shared.IconButton {
            width: Theme.itemSizeSmall
            icon.source: overlay.toolBar.bookmarked ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"
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
            id: reloadButton
            width: Theme.itemSizeSmall
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
