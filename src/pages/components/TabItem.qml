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

MouseArea {
    id: root

    readonly property bool down: pressed && containsMouse
    readonly property bool activeTab: activeTabIndex === index
    // Expose ListView for all items
    property Item view: ListView.view
    layer.effect: PressEffect {}
    layer.enabled: down
    width: parent.width
    height: Screen.width / 2

    onClicked: view.activateTab(index)

    // Keep this as square. Image streched incorrectly regardless of fillMode.
    // ShaderEffectSource "texture" will take correct part of this.
    Loader {
        id: tabTexture

        height: width
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: index === 0 ? Theme.paddingLarge : Theme.paddingMedium
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
        }
        sourceComponent: activeTab ? liveSource : image
    }

    Component {
        id: image
        Image {
            source: thumbnailPath
            width: parent.width
            height: width
            cache: false
        }
    }

    Component {
        id: liveSource
        ShaderEffectSource {
            anchors.fill: parent
            sourceItem: activeWebPage
            sourceRect: Qt.rect(0, 0, parent.width, parent.height)
        }
    }

    ShaderEffectSource {
        id: mask
        anchors {
            fill: root
            topMargin: tabTexture.anchors.topMargin
            leftMargin: tabTexture.anchors.leftMargin
            rightMargin: tabTexture.anchors.rightMargin
            bottomMargin: Theme.paddingMedium
        }

        hideSource: true
        visible: false
        sourceItem: Rectangle {
            color: "white"
            radius: 2/3 * Theme.paddingMedium
            x: mask.x
            y: mask.y
            width: mask.width
            height: mask.height
        }
    }

    ShaderEffectSource {
        id: texture
        anchors.fill: mask
        hideSource: true
        visible: false
        sourceItem: tabTexture
        sourceRect: Qt.rect(0, 0, mask.width, mask.height)
    }

    ShaderEffect {
        id: roundingItem
        property variant source: texture
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
        }
    "
    }

    OpacityRampEffect {
        slope: 2.6
        offset: 0.6
//        slope: slope.value
//        offset: offset.value

        sourceItem: roundingItem
        direction: OpacityRamp.TopToBottom
    }

    // Debug slider for opacity ramp
//    Column {
//        anchors.top: parent.top
//        anchors.topMargin: 20
//        width: parent.width
//        spacing: 20
//        Slider {
//            id: slope
//            width: parent.width
//            value: 2.6
//            stepSize: 0.02
//            minimumValue: 0.5
//            maximumValue: 10.0
//            valueText: value
//        }

//        Slider {
//            id: offset
//            width: parent.width
//            value: 0.6
//            stepSize: 0.01
//            valueText: value
//        }
//    }

    IconButton {
        id: close

        anchors {
            left: tabTexture.left
            bottom: parent.bottom
            bottomMargin: -Theme.paddingMedium
        }
        icon.source: "image://theme/icon-m-tab-close"
        onClicked: view.closeTab(index)
    }

    Label {
        anchors {
            left: close.right
            right: tabTexture.right
            rightMargin: Theme.paddingMedium
            verticalCenter: close.verticalCenter
        }

        text: title
        truncationMode: TruncationMode.Fade
        color: down ? Theme.highlightColor : Theme.primaryColor
    }
}
