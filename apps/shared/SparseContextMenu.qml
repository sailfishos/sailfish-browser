/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Silica.private 1.0
import Sailfish.WebView.Popups 1.0

ContextMenuInterface {
    id: menu

    readonly property bool active: visible
    width: parent.width
    height: parent.height
    visible: false

    function show() { visible = true }
    function hide() { visible = false }

    MouseArea {
        anchors.fill: parent
        onClicked: menu.hide()
    }
    Rectangle {
        anchors.fill: actions
        color: Theme.highlightDimmerColor
    }
    Column {
        id: actions

        width: parent.width
        anchors.bottom: parent.bottom
        ContextMenuItem {
            visible: menu.linkHref.length > 0
            //% "Copy link"
            text: qsTrId("sailfish_captiveportal-me-copy_link")
            onClicked: { Clipboard.text = menu.linkHref; menu.hide() }
        }
        ContextMenuItem {
            visible: menu.linkTitle.length > 0
            //% "Copy text"
            text: qsTrId("sailfish_captiveportal-me-copy_text")
            onClicked: { Clipboard.text = menu.linkTitle; menu.hide() }
        }
        DownloadMenuItem {
            visible: menu.downloadsEnabled && (menu.linkHref.length > 0 || menu.imageSrc.length > 0)
            //% "Save"
            text: qsTrId("sailfish_captiveportal-me-save")
            targetDirectory: StandardPaths.download
            linkUrl: menu.imageSrc || menu.linkHref
            contentType: menu.contentType
            viewId: menu.viewId
            onClicked: menu.hide()
        }
    }
}
