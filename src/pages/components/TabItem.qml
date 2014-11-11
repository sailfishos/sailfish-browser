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

    property bool activeTab: activeTabIndex === index
    // Expose ListView for all items
    property Item view: GridView.view
    property real topMargin
    property real leftMargin
    property real rightMargin
    property real bottomMargin
    property bool destroying

    // In direction so that we can break this binding when closing a tab
    implicitWidth: width
    implicitHeight: height

    enabled: !destroying

    clip: true
    layer.effect: PressEffect {}
    layer.enabled: _showPress

    // Rounded background item that is also a placeholder for a tab not having
    // thumbnail image.
    contentItem.visible: false
    contentItem.color: Theme.rgba(Theme.highlightBackgroundColor, Theme.highlightBackgroundOpacity)
    contentItem.radius: 2/3 * Theme.paddingMedium
    contentItem.x: root.leftMargin
    contentItem.y: root.topMargin
    contentItem.width: root.implicitWidth - root.leftMargin - root.rightMargin
    contentItem.height: root.implicitHeight - root.topMargin - root.bottomMargin

    onClicked: view.activateTab(index)

    // contentItem is hidden so this cannot be children of the contentItem.
    // So, making them as siblings of the contentItem.
    data: [
        Image {
            id: image

            readonly property bool active: source != ""

            source: !activeTab ? thumbnailPath : ""
            cache: false
            visible: false
            asynchronous: true
            smooth: true
        },
        ShaderEffectSource {
            id: textureSource
            width: root.implicitWidth
            height: root.implicitHeight
            live: !destroying
            visible: false
            sourceItem: activeTab ? activeWebPage : (image.active ? image : contentItem)
            sourceRect: Qt.rect(0, 0, mask.width, mask.height)
        },
        ShaderEffectSource {
            id: mask
            anchors.fill: contentItem
            live: textureSource.live
            hideSource: true
            visible: false
            sourceItem: Rectangle {
                x: contentItem.x
                y: contentItem.y
                width: contentItem.width
                height: contentItem.height
                radius: contentItem.radius
                color: "white"
            }
        },
        ShaderEffect {
            id: roundingItem
            property variant source: textureSource
            property variant maskSource: mask

            anchors.fill: mask
            smooth: true

            fragmentShader: "
                varying highp vec2 qt_TexCoord0;
                uniform highp float qt_Opacity;
                uniform lowp sampler2D source;
                uniform lowp sampler2D maskSource;
                void main(void) {
                    gl_FragColor = texture2D(source, qt_TexCoord0.st) * (texture2D(maskSource, qt_TexCoord0.st).a) * qt_Opacity;
                }"
        },
        OpacityRampEffect {
            id: ramp
            slope: 2.6
            offset: 0.6
            opacity: 1.0
            enabled: Qt.application.active

            sourceItem: roundingItem
            anchors.fill: mask
            direction: OpacityRamp.TopToBottom
        },
        IconButton {
            id: close

            visible: ramp.enabled
            anchors {
                left: mask.left
                bottom: parent.bottom
                bottomMargin: -root.bottomMargin
            }
            highlighted: down || activeTab
            icon.source: "image://theme/icon-m-tab-close"
            onClicked: {
                // Break binding, so that texture size would not change when
                // closing tab (animating height).
                root.implicitHeight = root.height
                root.implicitWidth = root.width

                destroying = true
                // Break binding to prevent texture source to change.
                if (activeTab) {
                    activeTab = true
                }
                activeTabIndex = -1
                removeTimer.running = true
            }
        },
        Timer {
            id: removeTimer
            interval: 16
            onTriggered: view.closeTab(index)
        },
        Label {
            anchors {
                left: close.right
                right: mask.right
                rightMargin: Theme.paddingMedium
                verticalCenter: close.verticalCenter
            }

            visible: ramp.enabled
            text: title
            truncationMode: TruncationMode.Fade
            color: down || activeTab ? Theme.highlightColor : Theme.primaryColor
        }
    ]
}
