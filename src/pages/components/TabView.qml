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

SilicaGridView {
    id: tabView

    property bool portrait
    property bool privateMode

    signal hide
    signal enterNewTabUrl
    signal activateTab(int index)
    signal closeTab(int index)
    signal closeAll

    cellWidth: portrait ? parent.width : parent.width / 3
    cellHeight: portrait ? Screen.width / 2 : cellWidth

    width: parent.width
    height: parent.height
    currentIndex: -1
    header: spacer
    footer: spacer
    // If approved to Silica, remove these transitions from browser.
    displaced: Browser.DisplaceTransition {}
    remove: Browser.RemoveTransition {}

    PrivateModeTexture {
        visible: privateMode
    }

    delegate: TabItem {
        id: tabItem

        width: tabView.cellWidth
        height: tabView.cellHeight

        topMargin: Theme.paddingMedium
        leftMargin: (portrait || index % 3 === 0) ? Theme.paddingLarge : Theme.paddingMedium
        rightMargin: (portrait || index % 3 === 2) ? Theme.paddingLarge : Theme.paddingMedium
        bottomMargin: Theme.paddingMedium

        Behavior on width {
            enabled: !tabItem.destroying
            NumberAnimation { easing.type: Easing.InOutQuad; duration: 150 }
        }
        Behavior on height {
            enabled: !tabItem.destroying
            NumberAnimation { easing.type: Easing.InOutQuad; duration: 150 }
        }

        GridView.onAdd: AddAnimation {}
    }

    // Behind tab delegates
    children: MouseArea {
        z: -1
        width: tabView.width
        height: tabView.height
        onClicked: hide()
    }

    PullDownMenu {
        visible: Qt.application.active
        flickable: tabView

        MenuItem {
            text: tabView.privateMode ?
                    //: Menu item switching back to normal browser
                    //% "Normal browsing"
                    qsTrId("sailfish_browser-me-normal_browsing") :
                    //: Menu item switching to private browser
                    //% "Private browsing"
                    qsTrId("sailfish_browser-me-private_browsing")
            onClicked: tabView.privateMode = !tabView.privateMode
        }
        MenuItem {
            visible: webView.tabModel.count
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

    ViewPlaceholder {
        enabled: !webView.tabModel.count
        //: Hint to create a new tab from pull down menu.
        //% "Pull down to create a new tab"
        text: qsTrId("sailfish_browser-la-pull_down_to_create_tab_hint")
    }

    Component {
        id: spacer
        Item {
            width: tabView.width
            height: Theme.paddingMedium
        }
    }
}
