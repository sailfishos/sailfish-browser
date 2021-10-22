/****************************************************************************
**
** Copyright (c) 2014 - 2019 Jolla Ltd.
** Copyright (c) 2019 - 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0
import org.nemomobile.configuration 1.0
import "." as Browser

SilicaGridView {
    id: tabView

    property bool portrait
    property bool privateMode
    property bool closingAllTabs
    property bool loaded

    property var remorsePopup
    readonly property bool largeScreen: Screen.sizeCategory > Screen.Medium
    readonly property real thumbnailHeight: largeScreen
                                            ? Screen.width / 3
                                            : !portrait ? parent.height / 2.5 : parent.width / 1.66
    readonly property real columns: largeScreen
                                        ? portrait ? 2 : 3
                                        : parent.width < 2 * parent.height
                                          ? parent.width <= height ? 1 : 2 : 3
    readonly property real thumbnailWidth: (parent.width - Theme.horizontalPageMargin * 2 - (Theme.paddingLarge * (columns - 1))) / columns

    signal hide
    signal enterNewTabUrl
    signal activateTab(int index)
    signal closeTab(int index)
    signal closeAll
    signal closeAllCanceled
    signal closeAllPending

    onCountChanged: if (count > 0) closingAllTabs = false
    onClosingAllTabsChanged: if (closingAllTabs) closeAllPending()

    width: parent.width - Theme.horizontalPageMargin
    height: parent.height
    x: Theme.horizontalPageMargin
    currentIndex: -1
    cellWidth: thumbnailWidth + Theme.paddingLarge
    cellHeight: thumbnailHeight + Theme.paddingLarge

    delegate: TabItem {
        id: tabItem

        enabled: !closingAllTabs
        opacity: enabled ? 1.0 : 0.0
        Behavior on opacity { FadeAnimator {}}

        width: thumbnailWidth
        height: thumbnailHeight

        GridView.onAdd: AddAnimation {
            target: tabItem
        }
        GridView.onRemove: RemoveAnimation {
            target: tabItem
        }

        Component.onCompleted:  {
            if ((index / columns * cellHeight) > tabView.height) {
                tabView.loaded = true
            }
        }
    }

    // Behind tab delegates
    children: [
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

    PullDownMenu {
        id: pullDownMenu

        flickable: tabView
        x: flickable.contentX + (flickable.width - Theme.horizontalPageMargin - width)/2
        width: Math.min(parent.width + Theme.horizontalPageMargin, screen.sizeCategory > Screen.Medium ? Screen.width*0.7 : Screen.width)
        on_InactiveHeightChanged: console.log("POS ", _inactiveHeight)

        MenuItem {
            visible: showCloseAllAction.value && model.count
            //% "Close all tabs"
            text: qsTrId("sailfish_browser-me-close_all")
            onClicked: {
                remorsePopup = Remorse.popupAction(
                            tabView,
                            //% "Closed all tabs"
                            qsTrId("sailfish_browser-closed-all-tabs"),
                            function() {
                                tabView.closeAll()
                                remorsePopup = null
                            })
                closingAllTabs = true
                remorsePopup.canceled.connect(
                            function() {
                                closingAllTabs = false
                                tabView.closeAllCanceled()
                                remorsePopup = null
                            })
            }
        }
        MenuItem {
            //% "New tab"
            text: qsTrId("sailfish_browser-me-new_tab")
            onClicked: tabView.enterNewTabUrl()
        }
    }

    Item {
        y: tabs.tabBarHeight
        height: parent.height - y
        anchors.right: parent.right
        width: Math.round(Theme.paddingSmall/2)

        VerticalScrollDecorator {
            _forcedParent: parent
            flickable: tabView
        }
    }

    ViewPlaceholder {
        x: -Theme.horizontalPageMargin
        width: parent.width + Theme.horizontalPageMargin
        enabled: !model.count || closingAllTabs
        //: Hint to create a new tab from pull down menu.
        //% "Pull down to create a new tab"
        text: qsTrId("sailfish_browser-la-pull_down_to_create_tab_hint")
    }

    onLoadedChanged: positionViewAtIndex(model.activeTabIndex, GridView.Center)
}
