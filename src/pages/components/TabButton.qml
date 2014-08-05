/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.private 1.0

MouseArea {
    id: mouseArea
    property alias label: label.text
    property alias icon: icon.icon
    property bool highlighted: icon.highlighted
    property bool selected

    signal tapped

    width: icon.width
    height: icon.height + label.height

    IconButton {
        id: icon
        anchors.horizontalCenter: parent.horizontalCenter
        width: Theme.iconSizeMedium
        height: width

        highlighted: selected || down || (mouseArea.pressed && mouseArea.containsMouse)

        onClicked: if (!selected) tapped()
    }

    Label {
        id: label
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: icon.bottom
        font.pixelSize: Theme.fontSizeExtraSmall
        color: highlighted ? Theme.highlightColor : Theme.primaryColor
        opacity: selected ? 1.0 : 0.0

        Behavior on opacity { FadeAnimation {} }
    }

    onClicked: {
        if (!selected) tapped()
    }
}
