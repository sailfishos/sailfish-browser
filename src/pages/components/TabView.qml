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

SilicaListView {
    id: tabView

    property bool portrait
    property bool privateMode

    signal hide
    signal enterNewTabUrl
    signal activateTab(int index)
    signal closeTab(int index)
    signal closeAll

    width: parent.width
    height: parent.height
    currentIndex: -1
    header: PageHeader {
        //: Tabs
        //% "Tabs"
        title: qsTrId("sailfish_browser-he-tabs")
    }
    footer: spacer

    delegate: TabItem {
        id: tabItem

        anchors.horizontalCenter: parent.horizontalCenter
        width: browserPage.thumbnailSize.width
        height: browserPage.thumbnailSize.height

        ListView.onAdd: AddAnimation {}
        ListView.onRemove: RemoveAnimation {
            target: tabItem
        }
    }

    // Behind tab delegates
    children: [
        PrivateModeTexture {
            z: -1
            visible: opacity > 0.0
            opacity: privateMode ? 1.0 : 0.0

            Behavior on opacity { FadeAnimation {} }
        },

        MouseArea {
            z: -1
            width: tabView.width
            height: tabView.height
            onClicked: hide()
        }
    ]

    PullDownMenu {
        id: pullDownMenu
        property var callbackFunction

        visible: Qt.application.active
        flickable: tabView

        // Delay private mode execution until PullDownMenu is closed.
        onActiveChanged: {
            if (!active && callbackFunction) {
                callbackFunction()
                callbackFunction = null
            }
        }

        MenuItem {
            function switchMode() {
                tabView.privateMode = !tabView.privateMode
            }

            text: tabView.privateMode ?
                    //: Menu item switching back to normal browser
                    //% "Normal browsing"
                    qsTrId("sailfish_browser-me-normal_browsing") :
                    //: Menu item switching to private browser
                    //% "Private browsing"
                    qsTrId("sailfish_browser-me-private_browsing")
            onClicked: pullDownMenu.callbackFunction = switchMode
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
