/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0

BackgroundItem {
    readonly property bool showContextMenu: down && _pressAndHold && browserPage.webView.inputPanelHeight === 0
    property bool _pressAndHold

    layer.effect: PressEffect {}
    layer.enabled: down
    _showPress: false

    onPressAndHold: {
        _pressAndHold = true
        browserPage.focus = true
    }

    onDownChanged: {
        if (!down) {
            _pressAndHold = false
        }
    }

    FavoriteIcon {
        id: favoriteIcon
        icon: favicon
        visible: hasTouchIcon
        anchors {
            centerIn: parent
            verticalCenterOffset: Math.round(-launcherText.height/2)
        }
    }

    Image {
        id: mask
        source: "icon_browser_favorites_mask00.png"
        anchors.fill: favoriteIcon
        visible: false
    }

    ShaderEffect {
        id: shaderItem
        property variant source: favoriteIcon
        property variant maskSource: mask

        enabled: !hasTouchIcon
        visible: !hasTouchIcon
        anchors.fill: favoriteIcon
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

    Text {
        id: launcherText

        anchors {
            top: favoriteIcon.bottom
            topMargin: Theme.paddingSmall
            left: parent.left
            right: parent.right
            leftMargin: Theme.paddingSmall/2
            rightMargin: Theme.paddingSmall/2
        }
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight

        color: down ? Theme.highlightColor : Theme.primaryColor
        font.pixelSize: Theme.fontSizeTiny
        text: title
    }
}
