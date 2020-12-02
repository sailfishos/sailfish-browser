/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Mikko Harju <mikko.harju@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import Sailfish.Silica 1.0

/*
 Two-color opaque background dimmer. Consists of
    - baseColor + baseOpacity as background color
    - dimmerColor + dimmerOpacity blended on top of background color
 */

ShaderEffect {
    property color dimmerColor: Theme.highlightDimmerColor
    property real dimmerOpacity

    property color baseColor: "white"
    property real baseOpacity

    blending: false
    visible: dimmerOpacity > 0.0 || baseOpacity > 0.0

    vertexShader: "
         uniform lowp float baseOpacity;
         uniform lowp float dimmerOpacity;
         uniform lowp vec4 baseColor;
         uniform lowp vec4 dimmerColor;
         uniform highp mat4 qt_Matrix;
         attribute highp vec4 qt_Vertex;

         varying lowp vec4 color;

         void main() {
          lowp vec4 base = baseColor * baseOpacity;
          lowp vec4 dimmer = dimmerColor * dimmerOpacity;
          color = base + dimmer*(1.0 - base.a);
          gl_Position = qt_Matrix * qt_Vertex;
         }
    "

    fragmentShader: "
         varying lowp vec4 color;
         void main() {
            gl_FragColor = color;
         }
    "
}
