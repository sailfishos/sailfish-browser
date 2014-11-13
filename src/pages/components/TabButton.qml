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
import "." as Browser

Browser.IconButton {
    property alias label: label
    property int horizontalOffset
    // Don't pass touch events through if opaque
    enabled: opacity === 1.0
    icon.source: "image://theme/icon-m-tabs"
    icon.anchors.horizontalCenterOffset: horizontalOffset

    Label {
        id: label
        anchors {
            centerIn: parent
            horizontalCenterOffset: horizontalOffset
        }
        font.pixelSize: Theme.fontSizeExtraSmall
        font.bold: true
        color: down ?   Theme.highlightColor : Theme.primaryColor
        horizontalAlignment: Text.AlignHCenter
    }
}
