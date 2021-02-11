/****************************************************************************
**
** Copyright (c) 2021 Jolla Ltd.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.6
import Sailfish.Silica 1.0

SilicaItem {
    id: root

    property bool down

    signal earlyClick
    signal clicked
    signal delayedClick

    property int __silica_menuitem
    property alias text: title.text
    property alias description: description.text

    height: Theme.itemSizeSmall
    width: parent.width

    Column {
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width

        MenuItem {
            id: title
            height: implicitHeight
            down: root.down
            highlighted: root.highlighted
        }

        MenuItem {
            id: description
            height: text ? implicitHeight : 0
            font.pixelSize: Theme.fontSizeExtraSmall
            down: root.down
            highlighted: root.highlighted
            color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
        }
    }
}
