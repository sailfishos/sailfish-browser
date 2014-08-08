/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0

Rectangle {
    id: root

    property string linkHref
    property string linkTitle
    property string imageSrc
    property string contentType
    property TabModel tabModel
    property PageStack pageStack
    property WebView webView

    property int viewId
    readonly property bool active: visible
    readonly property bool landscape: width > height

    visible: opacity > 0.0
    opacity: 0.0

    width: parent.width
    height: parent.height
    gradient: Gradient {
        GradientStop { position: 0.0; color: Theme.highlightDimmerColor }
        GradientStop { position: 1.0; color: Theme.rgba(Theme.highlightDimmerColor, .91) }
    }

    Behavior on opacity { FadeAnimation { duration: 300 } }

    Column {
        width: parent.width
        spacing: Theme.paddingMedium
        anchors.top: parent.top
        anchors.topMargin: Theme.paddingLarge*2

        Label {
            id: title
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.linkTitle.length > 0
            text: root.linkTitle
            width: root.width - Theme.paddingLarge*2
            elide: Text.ElideRight
            wrapMode: Text.Wrap
            maximumLineCount: 2
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeExtraLarge
            horizontalAlignment: Text.AlignHCenter
            opacity: .6
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            color: Theme.highlightColor
            text: root.imageSrc.length > 0 ? root.imageSrc : root.linkHref
            width: root.width - Theme.paddingLarge*2
            wrapMode: Text.Wrap
            elide: Text.ElideRight
            maximumLineCount: landscape ? 1 : 4
            font.pixelSize: title.visible ? Theme.fontSizeMedium : Theme.fontSizeExtraLarge
            horizontalAlignment: Text.AlignHCenter
            opacity: .6
        }
    }

    Column {
        id: menu

        property Item highlightedItem

        anchors.bottom: parent.bottom
        anchors.bottomMargin: landscape ? Theme.paddingLarge : Theme.itemSizeSmall
        width: parent.width

        MenuItem {
            visible: root.linkHref.length > 0 && root.imageSrc.length === 0
            //: Open link in current tab
            //% "Open link"
            text: qsTrId("sailfish_browser-me-open_link")

            onClicked: {
                root._hide()
                webView.load(root.linkHref, root.linkTitle)
            }
        }


        MenuItem {
            visible: root.linkHref.length > 0 && root.imageSrc.length === 0
            //: Open link in a new tab from browser context menu
            //% "Open link in a new tab"
            text: qsTrId("sailfish_browser-me-open_link_in_new_tab")

            onClicked: {
                root._hide()
                tabModel.newTab(root.linkHref, root.linkTitle)
            }
        }

        MenuItem {
            visible: root.linkHref.length > 0 && root.imageSrc.length === 0
            //: Share link from browser context menu
            //% "Share"
            text: qsTrId("sailfish_browser-me-share_link")

            onClicked: {
                root._hide()
                pageStack.push(Qt.resolvedUrl("../ShareLinkPage.qml"), {"link" : root.linkHref, "linkTitle": root.linkTitle})
            }
        }

        MenuItem {
            visible: root.linkHref.length > 0 && root.imageSrc.length === 0
            //: Copy link to clipboard from browser context menu
            //% "Copy to clipboard"
            text: qsTrId("sailfish_browser-me-copy_to_clipboard")

            onClicked: {
                root._hide()
                Clipboard.text = root.linkHref
            }
        }

        MenuItem {
            visible: imageSrc.length > 0
            //: Open image in a new tab from browser context menu
            //% "Open image in a new tab"
            text: qsTrId("sailfish_browser-me-open_image_in_new_tab")

            onClicked: {
                root._hide()
                tabModel.newTab(root.imageSrc, "")
            }
        }

        MenuItem {
            visible: root.imageSrc.length > 0
            //: This menu item saves image to Gallery application
            //% "Save to Gallery"
            text: qsTrId("sailfish_browser-me-save_image_to_gallery")

            onClicked: {
                root._hide()
                // drop query string from URL and split to sections
                var urlSections = imageSrc.split("?")[0].split("/")
                var leafName = urlSections[urlSections.length - 1]

                if (leafName.length === 0) {
                    leafName = "unnamed_picture"
                }

                MozContext.sendObserve("embedui:download",
                                       {
                                           "msg": "addDownload",
                                           "from": root.imageSrc,
                                           "to": "file://" + root.getUniqueFileName(leafName),
                                           "contentType": root.contentType,
                                           "viewId": root.viewId
                                       })
            }
        }

        function highlightItem(yPos) {
            var xPos = width/2
            var child = childAt(xPos, yPos)
            if (!child) {
                setHighlightedItem(null)
                return
            }
            var parentItem
            while (child) {
                if (child && child.hasOwnProperty("__silica_menuitem") && child.enabled) {
                    setHighlightedItem(child)
                    break
                }
                parentItem = child
                yPos = parentItem.mapToItem(child, xPos, yPos).y
                child = parentItem.childAt(xPos, yPos)
            }
        }

        function setHighlightedItem(item) {
            if (item === highlightedItem) {
                return
            }
            if (highlightedItem) {
                highlightedItem.down = false
            }
            highlightedItem = item
            if (highlightedItem) {
                highlightedItem.down = true
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onPressed: menu.highlightItem(mouse.y - menu.y)
        onPositionChanged: menu.highlightItem(mouse.y - menu.y)
        onCanceled:  menu.setHighlightedItem(null)
        onReleased: {
            if (menu.highlightedItem !== null) {
                menu.highlightedItem.down = false
                menu.highlightedItem.clicked()
            } else {
                onClicked: root._hide()
            }
        }
    }

    function getUniqueFileName(fileName) {
        var collisionCount = 0
        var picturesDir = WebUtils.picturesDir
        var sectmp

        while (WebUtils.fileExists(picturesDir + "/" + fileName)) {
            collisionCount++
            if (collisionCount == 1) {
                // append "(2)" before the last dot in (or at the end of) the filename
                sectmp = fileName.split(".")
                if (sectmp.length > 1) {
                    sectmp[sectmp.length-2] = sectmp[sectmp.length-2] + "(2)"
                } else {
                    sectmp[0] = sectmp[0] + "(2)"
                }
                fileName = sectmp.join(".")
            } else {
                // replace the last (n) in the filename with (n+1)
                sectmp = fileName.split("(" + collisionCount + ")")
                var tmp = sectmp.pop()
                fileName = sectmp.join("(" + collisionCount + ")") + "(" + (collisionCount+1) + ")" + tmp
            }
        }

        return picturesDir + "/" + fileName
    }

    function show() {
        opacity = 1.0
    }

    function _hide() {
        opacity = 0.0
    }
}
