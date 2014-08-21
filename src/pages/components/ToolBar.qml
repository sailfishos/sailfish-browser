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

import QtQuick 2.2
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import "." as Browser

Column {
    id: toolBarRow

    property string title
    property bool busy
    property real secondaryToolsHeight
    property bool secondaryToolsActive
    property alias bookmarked: secondaryBar.bookmarked
    readonly property alias toolsHeight: toolsRow.height

    signal showTabs
    signal showChrome
    signal showOverlay
    signal showShare

    signal bookmarkActivePage
    signal removeActivePageFromBookmarks

    signal load(string search)

    width: parent.width

    SecondaryBar {
        id: secondaryBar
        visible: opacity > 0.0 || height > 0.0
        opacity: secondaryToolsActive ? 1.0 : 0.0
        height: secondaryToolsHeight
        clip: true

        onBookmarkActivePage: toolBarRow.bookmarkActivePage()
        onRemoveActivePageFromBookmarks: toolBarRow.removeActivePageFromBookmarks()

        Behavior on opacity { FadeAnimation {} }
    }

    Row {
        id: toolsRow
        anchors.horizontalCenter: parent.horizontalCenter
        height: isPortrait ? Settings.toolbarLarge : Settings.toolbarSmall

        Browser.IconButton {
            id: reload

            width: Theme.iconSizeMedium + 2 * Theme.paddingMedium
            height: parent.height
            icon.source: webView.loading ? "image://theme/icon-m-reset" : "image://theme/icon-m-refresh"
            onTapped: webView.loading ? webView.stop() : webView.reload()
        }

        Browser.IconButton {
            id: backIcon
            active: webView.canGoBack
            width: Theme.iconSizeMedium + 2 * Theme.paddingMedium
            height: parent.height
            icon.source: "image://theme/icon-m-back"
            onTapped: webView.goBack()
        }

        /*
        Browser.IconButton {
            id: overlayIcon
            icon.source: "image://theme/icon-m-keyboard"
            width: Theme.itemSizeSmall
            height: toolBarRow.height
            onTapped: toolBarRow.showOverlay()
        }*/

        MouseArea {
            id: touchArea
            property bool down: pressed && containsMouse

            height: parent.height
            width: label.width - 2 * Theme.paddingSmall

            Label {
                id: label
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: touchArea.left
                width: toolBarRow.width - 4 * Theme.iconSizeMedium - 6 * Theme.paddingMedium - 4 * Theme.paddingSmall
                color: touchArea.down || toolBarRow.busy ? Theme.highlightColor : Theme.primaryColor
                text: title ? parseDisplayableUrl(title) : "Loading.."
                horizontalAlignment: Text.AlignHCenter
                truncationMode: TruncationMode.Fade

                function parseDisplayableUrl(url) {
                    var returnUrl = WebUtils.displayableUrl(url)
                    returnUrl = returnUrl.substring(returnUrl.lastIndexOf("/") + 1) // Strip protocol
                    if(returnUrl.indexOf("www.")===0) {
                        returnUrl = returnUrl.substring(4)
                    }
                    return returnUrl
                }
            }
            onClicked: toolBarRow.showOverlay()
        }


        Browser.IconButton {
            id: tabs

            width: Theme.iconSizeMedium + 2 * Theme.paddingMedium
            height: parent.height
            icon.source: "image://theme/icon-m-tabs"
            onTapped: {
                if (firstUseOverlay) {
                    firstUseOverlay.visible = false
                    firstUseOverlay.destroy()
                }
                if (!WebUtils.firstUseDone) WebUtils.firstUseDone = true
                toolBarRow.showTabs()
            }
            Label {
                visible: webView.tabModel.count > 0
                text: webView.tabModel.count
                x: (parent.width - contentWidth) / 2
                y: (parent.height - contentHeight) / 2
                font.pixelSize: Theme.fontSizeExtraSmall
                font.bold: true
                color: tabs.down ?   Theme.highlightColor : Theme.primaryColor
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Browser.IconButton {
            id: shareIcon

            icon.source: "image://theme/icon-lock-more"
            width: Theme.iconSizeMedium + 2 * Theme.paddingMedium
            height: parent.height
            onTapped: toolBarRow.showShare()
        }
    }
}
