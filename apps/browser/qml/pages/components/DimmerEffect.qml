/****************************************************************************
**
** Copyright (c) 2013 Jolla Ltd.
** Contact: Mikko Harju <mikko.harju@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import Sailfish.Silica 1.0

/*
  Opaque background dimmer.
    - dimmerColor + dimmerOpacity
 */

ShaderEffect {
    property color dimmerColor: Theme.highlightDimmerColor
    property real dimmerOpacity

    blending: false
    visible: dimmerOpacity > 0.0

    vertexShader: "
         uniform lowp float dimmerOpacity;
         uniform lowp vec4 dimmerColor;
         uniform highp mat4 qt_Matrix;
         attribute highp vec4 qt_Vertex;

         varying lowp vec4 color;

         void main() {
          color = dimmerColor * dimmerOpacity;
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
