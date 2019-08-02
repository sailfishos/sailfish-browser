/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0

BackgroundItem {
    id: root

    // Expose ListView for all items
    property Item view: ListView.view
    property bool destroying
    property color highlightColor: Theme.colorScheme == Theme.LightOnDark
                                   ? Theme.highlightColor
                                   : Theme.highlightFromColor(Theme.highlightColor, Theme.LightOnDark)
    // In direction so that we can break this binding when closing a tab
    implicitWidth: width
    implicitHeight: height

    enabled: !destroying

    layer.effect: PressEffect {}
    layer.enabled: _showPress

    // Background item that is also a placeholder for a tab not having
    // thumbnail image.
    contentItem.width: root.implicitWidth
    contentItem.height: root.implicitHeight

    onClicked: view.activateTab(index)

    // contentItem is hidden so this cannot be children of the contentItem.
    // So, making them as siblings of the contentItem.
    data: [
        Rectangle {
            color: "#202020"
            width: root.implicitWidth
            height: root.implicitHeight
        },
        Image {
            id: image

            source: thumbnailPath
            width: root.implicitWidth
            height: root.implicitHeight

            cache: false
            asynchronous: true
            opacity: status !== Image.Ready && source !== "" ? 0.0 : 1.0
            Behavior on opacity { FadeAnimation {} }
        },
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: Math.max(close.height, titleLabel.height) + Theme.paddingLarge * 2

            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.45; color: Qt.rgba(0, 0, 0, Theme.opacityLow)}
                GradientStop { position: 0.9; color: Qt.rgba(0, 0, 0, Theme.opacityOverlay)}
            }
        },
        IconButton {
            id: close

            anchors {
                left: parent.left
                bottom: parent.bottom
            }

            icon.color: Theme.lightPrimaryColor
            icon.highlightColor: root.highlightColor
            icon.highlighted: down || activeTab
            icon.source: "image://theme/icon-m-tab-close"
            onClicked: {
                // Break binding, so that texture size would not change when
                // closing tab (animating height).
                root.implicitHeight = root.height
                root.implicitWidth = root.width

                destroying = true
                removeTimer.running = true
            }
        },
        Timer {
            id: removeTimer
            interval: 16
            onTriggered: view.closeTab(index)
        },
        Label {
            id: titleLabel

            anchors {
                left: close.right
                right: parent.right
                rightMargin: Theme.paddingMedium
                verticalCenter: close.verticalCenter
            }

            text: title || WebUtils.displayableUrl(url)
            truncationMode: TruncationMode.Fade
            color: down || activeTab ? root.highlightColor : Theme.lightPrimaryColor
        }
    ]
}
