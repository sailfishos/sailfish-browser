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

    signal showTabs
    signal showChrome
    signal showOverlay
    signal load(string search)

    width: parent.width
    height: isPortrait ? Settings.toolbarLarge : Settings.toolbarSmall


    Row {
        anchors.centerIn: parent
        spacing: (parent.width - (7 * Theme.itemSizeSmall)) / 6
        Browser.IconButton {
            id: backIcon
            active: webView.canGoBack
            width: Theme.itemSizeSmall
            height: toolBarRow.height
            icon.source: "image://theme/icon-m-back"
            onTapped: webView.goBack()
        }

        Browser.IconButton {
            id: shareIcon
            icon.source: "image://theme/icon-m-share"
            width: Theme.itemSizeSmall
            height: toolBarRow.height
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
            width: domainBox.width

            property bool down: pressed && containsMouse

            Rectangle {
                id: domainBox
                height: label.height + 2 * Theme.paddingSmall
                width: Theme.itemSizeSmall * 3
                radius: 4.0
                anchors.verticalCenter: parent.verticalCenter
                color: "transparent"

                Label {
                    id: label
                    anchors.top: parent.top
                    anchors.topMargin: Theme.paddingSmall
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.paddingSmall
                    width: parent.width - Theme.paddingSmall * 2
                    color: touchArea.down ? Theme.highlightColor : Theme.primaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    text: parseDisplayableUrl(title)
                    horizontalAlignment: Text.AlignHCenter

                    function parseDisplayableUrl(url) {
                        var returnUrl = WebUtils.displayableUrl(url)
                        returnUrl = returnUrl.substring(returnUrl.lastIndexOf("/") + 1) // Strip protocol
                        if(returnUrl.indexOf("www.")===0) {
                            returnUrl = returnUrl.substring(4)
                        }
                        return returnUrl
                    }

                }
                border.color: label.color
                border.width: 1
            }

            onClicked: toolBarRow.showOverlay()
        }


        Browser.IconButton {
            id: tabs
            width: Theme.itemSizeSmall
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
                x: (parent.width - contentWidth) / 2 - 5
                y: (parent.height - contentHeight) / 2 - 5
                font.pixelSize: Theme.fontSizeExtraSmall
                font.bold: true
                color: tabs.down ?  Theme.primaryColor : Theme.highlightDimmerColor
                horizontalAlignment: Text.AlignHCenter
            }
        }

        Browser.IconButton {
            id: reload
            width: Theme.itemSizeSmall
            height: toolBarRow.height
            icon.source: webView.loading ? "image://theme/icon-m-reset" : "image://theme/icon-m-refresh"
            onTapped: webView.loading ? webView.stop() : webView.reload()
        }
    }
}
