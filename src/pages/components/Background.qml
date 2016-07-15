/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import Sailfish.Silica 1.0
import "." as Browser

Item {
    id: wallpaper

    readonly property alias baseColor: base.color
    readonly property alias baseOpacity: base.opacity

    Item {
        id: glassTextureItem
        visible: false
        width: glassTextureImage.width
        height: glassTextureImage.height

        Rectangle {
            id: base
            anchors.fill: parent
            color: Theme.primaryColor
            opacity: window.opaqueBackground ? 1 : 0
            Behavior on opacity {
                enabled: window.rootPage.active
                FadeAnimation { }
            }
        }

        Rectangle {
            color: Theme.overlayBackgroundColor
            opacity: 0.948
            anchors.fill: parent
        }

        Image {
            id: glassTextureImage
            opacity: 0.1
            source: "image://theme/graphic-shader-texture"
        }
    }

    ShaderEffect {
        id: wallpaperEffect
        anchors.fill: parent

        // glass texture size
        property size glassTextureSizeInv: Qt.size(1.0/(glassTextureImage.sourceSize.width),
                                                   -1.0/(glassTextureImage.sourceSize.height))

        property variant glassTexture: ShaderEffectSource {
            hideSource: true
            sourceItem: glassTextureItem
            wrapMode: ShaderEffectSource.Repeat
        }

        blending: false

        vertexShader: "
           uniform highp mat4 qt_Matrix;
           attribute highp vec4 qt_Vertex;

           void main() {
              gl_Position = qt_Matrix * qt_Vertex;
           }
        "

        fragmentShader: "
           uniform sampler2D glassTexture;
           uniform highp vec2 glassTextureSizeInv;
           uniform lowp float qt_Opacity;

           void main() {
              lowp vec4 tx = texture2D(glassTexture, gl_FragCoord.xy * glassTextureSizeInv);
              gl_FragColor = tx;
           }
        "
    }
}
