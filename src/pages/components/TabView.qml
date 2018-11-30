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
import org.nemomobile.configuration 1.0
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

    spacing: Theme.paddingMedium

    delegate: TabItem {
        id: tabItem

        anchors.horizontalCenter: parent.horizontalCenter
        width: browserPage.thumbnailSize.width
        height: browserPage.thumbnailSize.height

        ListView.onAdd: AddAnimation {
            target: tabItem
        }
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

    ConfigurationValue {
        id: showCloseAllAction
        key: "/apps/sailfish-browser/settings/show_close_all"
        defaultValue: true
    }

    // Remorse popup for closing all tabs.
    RemorsePopup {
        id: closeAllTabsRemorse
    }

    PullDownMenu {
        id: pullDownMenu

        flickable: tabView

        MenuItem {
            text: tabView.privateMode ?
                    //: Menu item switching back to normal browser
                    //% "Normal browsing"
                    qsTrId("sailfish_browser-me-normal_browsing") :
                    //: Menu item switching to private browser
                    //% "Private browsing"
                    qsTrId("sailfish_browser-me-private_browsing")
            onDelayedClick: tabView.privateMode = !tabView.privateMode
        }
        MenuItem {
            visible: showCloseAllAction.value && webView.tabModel.count
            //% "Close all tabs"
            text: qsTrId("sailfish_browser-me-close_all")
            //% "Closing all tabs"
            onDelayedClick: closeAllTabsRemorse.execute(qsTrId("sailfish_browser-closing-all-tabs"), tabView.closeAll)
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
