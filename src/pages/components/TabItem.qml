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
    readonly property bool down: pressed && containsMouse
    // Expose ListView for all items
    property Item view: ListView.view
    layer.effect: PressEffect {}
    layer.enabled: down
    width: parent.width
    height: Screen.width / 2

    onClicked: {
        if (active) {
            view.hide()
        } else {
            view.activateTab(index)
        }
    }

    Loader {
        id: tabTexture

        property real textureHeight: parent.height - Theme.paddingMedium

        anchors {
            fill: parent
            topMargin: index === 0 ? Theme.paddingLarge : Theme.paddingMedium
            bottomMargin: Theme.paddingMedium
            leftMargin: Theme.paddingLarge
            rightMargin: Theme.paddingLarge
        }

        sourceComponent: model.active ? liveSource : image
    }

    Component {
        id: image
        Image {
            anchors.fill: parent
            source: thumbnailPath
            fillMode: Image.PreserveAspectCrop
            cache: false

            Behavior on opacity { FadeAnimation {} }
        }
    }

    // TODO: this should be the active tab. Current index 0 but order should be kept.
    // Fix model order.
    Component {
        id: liveSource
        ShaderEffectSource {
            anchors.fill: parent
            sourceItem: enabled ? webView && webView.contentItem : null
            sourceRect: Qt.rect(0, 0, parent.width, parent.height)
        }
    }

    ShaderEffectSource {
        id: mask
        anchors.fill: tabTexture
        hideSource: true
        visible: false
        sourceItem: Rectangle {
            color: "blue"
            radius: 2/3 * Theme.paddingMedium
            x: tabTexture.x; y: tabTexture.y
            width: tabTexture.width
            height: tabTexture.height
            visible: false
        }
    }

    ShaderEffectSource {
        id: texture
        anchors.fill: tabTexture
        hideSource: true
        visible: false
        sourceItem: tabTexture

    }

    ShaderEffect {
        id: roundingItem
        property variant source: texture
        property variant maskSource: mask

        anchors.fill: tabTexture
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
        id: effect
        slope: 2.6
        offset: 0.6
//        slope: slope.value
//        offset: offset.value

        //sourceItem: roundingItem
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
