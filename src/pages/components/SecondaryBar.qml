/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import "." as Browser

Item {
    id: root
    property bool bookmarked
    property int horizontalOffset
    property int iconWidth
    property real midIconWidth: iconWidth + (iconWidth - forwardButton.width) / 3

    width: parent.width
    height: isPortrait ? Settings.toolbarLarge : Settings.toolbarSmall
    clip: true

    Row {
        width: parent.width
        height: parent.height

        Browser.TabButton {
            width: iconWidth + horizontalOffset
            horizontalOffset: root.horizontalOffset
            label.text: "+"
            onTapped: enterNewTabUrl()
        }

        Browser.NavigationButton {
            id: forwardButton
            buttonWidth: iconWidth
            icon.source: "image://theme/icon-m-forward"
            active: webView.canGoForward
            onTapped: webView.goForward()
        }

        Browser.IconButton {
            width: midIconWidth
            icon.source: "image://theme/icon-m-search"
            active: webView.contentItem
            onTapped: searchFromActivePage()
        }

        Browser.IconButton {
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

        Browser.IconButton {
            width: midIconWidth
            icon.source: "image://theme/icon-m-share"
            active: webView.contentItem
            onTapped: shareActivePage()
        }

        Browser.IconButton {
            width: iconWidth + horizontalOffset
            icon.source: "image://theme/icon-m-mobile-network"
            icon.anchors.horizontalCenterOffset: -horizontalOffset
            onTapped: showDownloads()
        }
    }
}
