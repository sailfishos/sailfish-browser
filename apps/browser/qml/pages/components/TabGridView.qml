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

SilicaGridView {
    id: tabGridView

    property bool portrait
    property bool closingAllTabs

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
    readonly property real _tabScale: housekeeping ? 0.9 : 1.0
    property bool housekeeping

    signal hide
    signal enterNewTabUrl
    signal activateTab(int index)
    signal closeTab(int index)
    signal closeAll
    signal closeAllCanceled
    signal closeAllPending

    function closeAllTabs() {
        remorsePopup = Remorse.popupAction(
                    tabGridView,
                    //% "Closed all tabs"
                    qsTrId("sailfish_browser-closed-all-tabs"),
                    function() {
                        tabGridView.closeAll()
                        remorsePopup = null
                        closingAllTabs = false
                    })
        closingAllTabs = true
        remorsePopup.canceled.connect(
                    function() {
                        closingAllTabs = false
                        tabGridView.closeAllCanceled()
                        remorsePopup = null
                    })
    }

    onCountChanged: if (count > 0) closingAllTabs = false
    onClosingAllTabsChanged: if (closingAllTabs) closeAllPending()

    width: parent.width - Theme.horizontalPageMargin
    height: parent.height
    x: Theme.horizontalPageMargin
    currentIndex: model.activeTabIndex
    cellWidth: thumbnailWidth + Theme.paddingLarge
    cellHeight: thumbnailHeight + Theme.paddingLarge

    delegate: Item {
        id: item

        opacity: !closingAllTabs ? 1.0 : 0.0
        Behavior on opacity { FadeAnimator {}}

        width: thumbnailWidth
        height: thumbnailHeight

        scale: tabGridView._tabScale
        Behavior on scale { SmoothedAnimation {duration: 200; velocity: -1} }

        GridView.onAdd: AddAnimation {
            target: item
        }
        GridView.onRemove: RemoveAnimation {
            target: item
        }

        TabItem {
            id: tabItem
            anchors.fill: parent
            enabled: !closingAllTabs
            showClose: !tabGridView.housekeeping

            onClicked: {
                if (tabGridView.housekeeping) {
                    tabGridView.housekeeping = false
                } else {
                    activateTab(index)
                }
            }
            onPressAndHold: tabGridView.housekeeping = true
            onCloseClicked: closeTab(index)
        }
        Rectangle {
            color: Theme.colorScheme === Theme.LightOnDark
                ? closeButton.down ? Theme.highlightColor : Theme.primaryColor
                : closeButton.down ? Theme.highlightDimmerColor : Theme.highlightBackgroundColor
            width: Theme.iconSizeMedium
            height: width
            radius: width / 2.
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.bottom
            opacity: tabGridView.housekeeping ? 1.0 : 0.0
            visible: opacity > 0.
            Behavior on opacity { FadeAnimation { } }

            IconButton {
                id: closeButton
                anchors.centerIn: parent
                icon.color: Theme.colorScheme === Theme.LightOnDark
                    ? Theme.highlightBackgroundColor : Theme.lightPrimaryColor
                icon.highlightColor: Theme.colorScheme === Theme.LightOnDark
                    ? Theme.highlightDimmerColor : Theme.darkSecondaryColor
                icon.highlighted: down
                icon.source: "image://theme/icon-close-app"
                enabled: !closingAllTabs && tabGridView.housekeeping
                onClicked: {
                    tabItem.markClosed()
                    closeTab(index)
                }
            }
        }
    }

    // Behind tab delegates
    children: [
        MouseArea {
            z: -1
            width: tabGridView.width
            height: tabGridView.height
            onClicked: tabGridView.housekeeping = false
        }
    ]


    Item {
        height: parent.height
        anchors.right: parent.right
        width: Math.round(Theme.paddingSmall/2)

        VerticalScrollDecorator {
            _forcedParent: parent
            flickable: tabGridView
        }
    }

    ViewPlaceholder {
        x: -Theme.horizontalPageMargin
        width: parent.width + Theme.horizontalPageMargin
        enabled: !model.count || closingAllTabs
        //: Hint to create a new tab via button new tab.
        //% "Push button plus to create a new tab"
        text: qsTrId("sailfish_browser-la-push_button_plus_to_create_tab_hint")
    }

    Timer {
        interval: 100
        running: true
        onTriggered: positionViewAtIndex(model.activeTabIndex, GridView.Center)
    }

    Connections {
        target: webView

        onApplicationClosing: {
            if (tabGridView.closingAllTabs) tabGridView.closeAll()
        }
    }
}
