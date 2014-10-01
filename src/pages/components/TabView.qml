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

SilicaGridView {
    id: tabView

    property bool portrait

    signal hide
    signal enterNewTabUrl
    signal activateTab(int index)
    signal closeTab(int index)
    signal closeAll

    cellWidth: portrait ? parent.width : parent.width / 3
    cellHeight: portrait ? Screen.width / 2 : cellWidth
    width: parent.width
    height: parent.height
    clip: true
    currentIndex: -1
    displaced: Transition { NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad; duration: 200 } }

    Behavior on cellWidth { NumberAnimation { easing.type: Easing.InOutQuad; duration: 200 } }
    Behavior on cellHeight { NumberAnimation { easing.type: Easing.InOutQuad; duration: 200 } }

    delegate: TabItem {
        id: tabItem

        width: tabView.cellWidth
        height: tabView.cellHeight

        topMargin: (portrait ? index === 0 : index < 3) ? Theme.paddingLarge : Theme.paddingMedium
        leftMargin: (portrait || index % 3 === 0) ? Theme.paddingLarge : Theme.paddingMedium
        rightMargin: (portrait || index % 3 === 2) ? Theme.paddingLarge : Theme.paddingMedium

        ListView.onAdd: AddAnimation {}
        ListView.onRemove: RemoveAnimation { target: tabItem }
    }

    // Behind tab delegates
    children: MouseArea {
        z: -1
        width: tabView.width
        height: tabView.height
        onClicked: hide()
    }

    PullDownMenu {
        flickable: tabView

        MenuItem {
            //% "Close all tabs"
            text: qsTrId("sailfish_browser-me-close_all")
            onClicked: tabView.closeAll()
        }
        MenuItem {
            //% "New tab"
            text: qsTrId("sailfish_browser-me-new_tab")
            onClicked: tabView.enterNewTabUrl()
        }
    }

    VerticalScrollDecorator {
        flickable: tabView
    }
}
