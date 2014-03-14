/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0

Item {
    property alias iconSource: icon.source
    property alias text: label.text
    property bool header: false

    width: parent.width
    height: iconSource == "" ? label.height : Math.max(label.height, Theme.itemSizeSmall)

    Image {
        id: icon
    }

    Label {
        id: label
        color: Theme.highlightColor
        font.pixelSize: header ? Theme.fontSizeHuge : Theme.fontSizeMedium
        width: iconSource == "" ? parent.width : parent.width - icon.width - Theme.paddingMedium
        anchors.right: parent.right
        wrapMode: Text.WordWrap
    }
}
