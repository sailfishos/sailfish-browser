/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0

ListItem {
    id: root

    property alias label: valueButton.label
    property alias value: valueButton.value
    property alias iconSource: icon.source
    property alias description: valueButton.description

    width: parent.width
    contentHeight: Math.max(Theme.itemSizeMedium, row.height)
    onClicked: openMenu()

    Row {
        id: row

        width: parent.width - 2*Theme.horizontalPageMargin
        x: Theme.horizontalPageMargin
        anchors.verticalCenter: parent.verticalCenter

        Icon {
            id: icon

            y: valueButton.y + valueButton.height/2 - height/2
            Behavior on y {
                NumberAnimation {
                    easing.type: Easing.InOutQuad
                    duration: valueButton._duration
                }
            }
        }

        ValueButton {
            id: valueButton

            width: parent.width - icon.width
            anchors.verticalCenter: parent.verticalCenter
            leftMargin: Theme.paddingMedium
            rightMargin: 0
            labelColor: root.highlighted ? palette.highlightColor : palette.primaryColor
            valueColor: Theme.highlightColor
            opacity: 1.0
            enabled: false
        }
    }
}
