/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0

ShaderEffect {
    property variant source
    property color color: Theme.rgba(Theme.highlightBackgroundColor, 0.4)
    fragmentShader: "
        uniform sampler2D source;
        uniform highp vec4 color;
        uniform lowp float qt_Opacity;
        varying highp vec2 qt_TexCoord0;
        void main(void)
        {
            highp vec4 pixelColor = texture2D(source, qt_TexCoord0);
            gl_FragColor = vec4(mix(pixelColor.rgb/max(pixelColor.a, 0.00390625), color.rgb/max(color.a, 0.00390625), color.a) * pixelColor.a, pixelColor.a) * qt_Opacity;
        }
        "
}
