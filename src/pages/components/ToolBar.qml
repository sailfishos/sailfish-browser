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

Item {
    id: toolBarRow

    property string title
    property bool busy

    signal showTabs
    signal showChrome
    signal showOverlay
    signal showShare

    signal load(string search)

    width: parent.width
    height: isPortrait ? Settings.toolbarLarge : Settings.toolbarSmall


    Row {
        anchors.centerIn: parent
        height: parent.height

        Browser.IconButton {
            id: reload
            width: Theme.iconSizeMedium + 2 * Theme.paddingMedium
            height: toolBarRow.height
            icon.source: webView.loading ? "image://theme/icon-m-reset" : "image://theme/icon-m-refresh"
            onTapped: webView.loading ? webView.stop() : webView.reload()
        }

        Browser.IconButton {
            id: backIcon
            active: webView.canGoBack
            width: Theme.iconSizeMedium + 2 * Theme.paddingMedium
            height: toolBarRow.height
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
            height: parent.height
            width: domainBox.width - 2 * Theme.paddingSmall

            property bool down: pressed && containsMouse

            Rectangle {
                id: domainBox
                height: label.height + Theme.paddingSmall
                x: -Theme.paddingSmall
                width: toolBarRow.width - 4 * Theme.iconSizeMedium - 6 * Theme.paddingMedium - 4 * Theme.paddingSmall
                radius: 4.0
                anchors.verticalCenter: parent.verticalCenter
                color: "transparent"


                border.color: label.color
                border.width: 2
                opacity: 0.5
            }

            Label {
                id: label
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: touchArea.left
                width: parent.width
                color: touchArea.down || toolBarRow.busy ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                font.bold: true
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
            height: toolBarRow.height
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
            icon.source: "image://theme/icon-m-share"
            width: Theme.iconSizeMedium + 2 * Theme.paddingMedium
            height: toolBarRow.height
            onTapped: toolBarRow.showShare()
        }
    }
}
