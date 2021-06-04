/****************************************************************************
**
** Copyright (c) 2020 - 2021 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.6
import Sailfish.Silica 1.0

BackgroundItem {
    id: root

    property alias text: label.text
    property alias iconSource: icon.source
    property alias checked: toggle.checked
    property bool checkable
    property int horizontalOffset
    property int iconWidth

    width: parent.width

    Item {
        id: iconContainer

        width: iconWidth
        height: parent.height
        x: Theme.paddingMedium

        Icon {
            id: icon

            anchors {
                centerIn: parent
                horizontalCenterOffset: horizontalOffset
            }
            highlighted: root.highlighted
            color: Theme.primaryColor
            opacity: root.enabled ? 1.0 : Theme.opacityLow
        }
    }
    Label {
        id: label

        anchors {
            left: iconContainer.right
            leftMargin: Theme.paddingMedium
            right: parent.right
            rightMargin: toggle.visible ? toggle.width + Theme.paddingMedium : Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        color: root.highlighted ? Theme.highlightColor : Theme.primaryColor
        opacity: root.enabled ? 1.0 : Theme.opacityLow
        truncationMode: TruncationMode.Fade

        Behavior on opacity { FadeAnimation {} }
    }
    Switch {
        id: toggle
        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
        }

        visible: checkable
        onClicked: root.clicked(mouse)
    }
}
