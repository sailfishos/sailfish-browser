/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.6
import Sailfish.Silica 1.0

Row {
    id: root

    property alias text: label.text

    width: parent.width

    spacing: Theme.paddingMedium
    leftPadding: Theme.horizontalPageMargin

    Icon {
        id: icon

        source: "image://theme/icon-s-checkmark"
        color: Theme.highlightColor
    }
    Label {
        id: label

        anchors {
            verticalCenter: icon.verticalCenter
            // center on the first line if there are multiple lines
            verticalCenterOffset: lineCount > 1 ? (lineCount-1)*height/lineCount/2 : 0
        }
        width: parent.width - icon.width - root.spacing - 2 * Theme.horizontalPageMargin

        wrapMode: Text.Wrap
        color: Theme.highlightColor
    }
}
