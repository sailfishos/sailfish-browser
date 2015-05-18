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
import Sailfish.Silica.private 1.0 as Private
import "." as Browser

Rectangle {
    color: "black"
    opacity: 0.7

    Private.Wallpaper {
        anchors.fill: parent
        source: ""
        glassOnly: true
        effect.blending: true
    }
}
