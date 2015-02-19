/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Siteshwar Vashisht <siteshwar@gmail.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1

Image {
    anchors.fill: parent
    fillMode: Image.Tile
    source: "graphic-diagonal-line-texture.png"
    visible: opacity > 0.0
}
