/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import "." as Browser

Row {
    id: toolbarRow

    property int iconWidth: isPortrait ? width / 6 : height

    height: isPortrait ? Settings.toolbarLarge : Settings.toolbarSmall
    anchors {
        left: parent.left; leftMargin: isPortrait ? 0 : Theme.paddingMedium
        right: parent.right; rightMargin: isPortrait ? 0 : Theme.paddingMedium
    }

    // 5 icons, 4 spaces between
    spacing: isPortrait ? (width - (backIcon.width * 6)) / 5 : Theme.paddingSmall

    Browser.IconButton {
        visible: isLandscape
        width: iconWidth
        icon.source: "image://theme/icon-m-close"
        onTapped: webView.tabModel.closeActiveTab()
    }

    // Spacer
    Item {
        visible: isLandscape
        height: parent.height
        width: browserPage.width
               - toolbarRow.spacing * (toolbarRow.children.length - 1)
               - backIcon.width * (toolbarRow.children.length - 1)
               - parent.anchors.leftMargin
               - parent.anchors.rightMargin

        Browser.TitleBar {
            url: webView.url
            title: webView.title
            height: parent.height
            onClicked: overlayAnimator.state = "fullscreenOverlay"
            // Workaround for binding loop jb#15182
            clip: true
        }
    }

    Browser.IconButton {
        id:backIcon
        active: webView.canGoBack
        width: iconWidth
        icon.source: "image://theme/icon-m-back"
        onTapped: webView.goBack()
    }

    Browser.IconButton {
        property bool favorited: favorites.count > 0 && favorites.contains(webView.url)
        active: webView.visible
        width: iconWidth
        icon.source: favorited ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"
        onTapped: {
            if (favorited) {
                favorites.removeBookmark(webView.url)
            } else {
                favorites.addBookmark(webView.url, webView.title, webView.favicon)
            }
        }
    }

    Browser.IconButton {
        id: tabPageButton
        width: iconWidth
        icon.source: "image://theme/icon-m-tabs"
        onTapped: {
            if (firstUseOverlay) {
                firstUseOverlay.visible = false
                firstUseOverlay.destroy()
            }
            if (!WebUtils.firstUseDone) WebUtils.firstUseDone = true
            overlay.openTabPage(false, false, PageStackAction.Animated)
        }
        Label {
            visible: webView.tabModel.count > 0
            text: webView.tabModel.count
            x: (parent.width - contentWidth) / 2 - 5
            y: (parent.height - contentHeight) / 2 - 5
            font.pixelSize: Theme.fontSizeExtraSmall
            font.bold: true
            color: tabPageButton.down ?  Theme.primaryColor : Theme.highlightDimmerColor
            horizontalAlignment: Text.AlignHCenter
        }
    }

    Browser.IconButton {
        active: webView.visible
        width: iconWidth
        icon.source: webView.loading ? "image://theme/icon-m-reset" : "image://theme/icon-m-refresh"
        onTapped: webView.loading ? webView.stop() : webView.reload()
    }

    Browser.IconButton {
        icon.source: "image://theme/icon-m-forward"
        width: iconWidth
        active: webView.canGoForward
        onTapped: webView.goForward()
    }

    Browser.IconButton {
        icon.source: "image://theme/icon-m-document"
        width: iconWidth
        onTapped: {
            if (overlayAnimator.atTop) {
                overlayAnimator.hide()
            } else {
                overlayAnimator.show()
            }
        }
    }
}
