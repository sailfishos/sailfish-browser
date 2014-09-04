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

SilicaListView {
    id: tabView

    readonly property real tabsHeight: model.count * Screen.width / 2
    readonly property bool fillsPage: tabsHeight >= parent.height

    signal hide
    signal enterNewTabUrl
    signal activateTab(int index)
    signal closeTab(int index)
    signal addBookmark(string url, string title, string favicon)
    signal removeBookmark(string url)

    width: parent.width
    height: parent.height
    clip: true
    currentIndex: -1
    displaced: Transition { NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad; duration: 200 } }

    delegate: TabItem {
        id: tabItem
        ListView.onAdd: AddAnimation {}
        ListView.onRemove: RemoveAnimation { target: tabItem }
    }

    footer: MouseArea {
        visible: !fillsPage
        enabled: visible
        width: tabView.width
        height: visible ? tabView.parent.height - tabsHeight : 0
        onClicked: hide()
    }

    PullDownMenu {
        flickable: tabView
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
