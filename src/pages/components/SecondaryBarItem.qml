/****************************************************************************
**
** Copyright (C) 2020 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.6
import Sailfish.Silica 1.0

MouseArea {
    id: root

    property alias text: label.text
    property alias iconSource: icon.source

    width: parent.width

    Row {
        leftPadding: Theme.horizontalPageMargin
        spacing: Theme.paddingMedium

        height: parent.height

        Icon {
            id: icon

            anchors.verticalCenter: parent.verticalCenter
            source: "image://theme/icon-m-favorite"
            highlighted: root.pressed
            color: Theme.primaryColor
        }
        Label {
            id: label

            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - icon.width
            color: root.pressed ? Theme.highlightColor : Theme.primaryColor

            truncationMode: TruncationMode.Fade

            Behavior on opacity { FadeAnimation {} }
        }
    }
}
