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
import Sailfish.Silica.private 1.0 as Private
import Sailfish.Browser 1.0
import Nemo.Configuration 1.0

SilicaControl {
    id: tabView

    property bool portrait
    property bool privateMode
    property var tabModel
    property alias scaledPortraitHeight: tabsToolBar.scaledPortraitHeight
    property alias scaledLandscapeHeight: tabsToolBar.scaledLandscapeHeight

    signal hide
    signal enterNewTabUrl
    signal activateTab(int index)
    signal closeTab(int index)
    signal closeAll
    signal closeAllCanceled
    signal closeAllPending

    property var _remorsePopup
    property bool _closingAllTabs

    property bool _emptyPrivateIcon: (privateMode && _closingAllTabs) || webView.privateTabModel.count === 0

    anchors.fill: parent

    Private.TabView {
        id: tabs

        anchors {
            fill: parent
            bottomMargin: tabsToolBar.height
        }

        header: Private.TabBar {
            id: headerTabs
            model: modeModel
            delegate: Private.TabButton {
                id: tabButton

                icon.source: {
                    if (!model.privateMode) {
                        return "image://theme/icon-m-tabs"
                    } else if (tabView._emptyPrivateIcon) {
                        return "image://theme/icon-m-incognito"
                    } else {
                        "image://theme/icon-m-incognito-selected"
                    }
                }
                icon.color: palette.primaryColor

                icon.children: Label {
                    anchors.centerIn: tabButton.icon

                    font.pixelSize: Theme.fontSizeExtraSmall
                    font.bold: true
                    color: {
                        if (model.privateMode) {
                            return tabView.palette.colorScheme === Theme.LightOnDark
                                    ? Theme.darkPrimaryColor
                                    : Theme.lightPrimaryColor
                        } else if (highlighted || tabButton.isCurrentTab) {
                            return palette.highlightColor
                        } else {
                            return tabView.palette.colorScheme === Theme.LightOnDark
                                    ? Theme.lightPrimaryColor
                                    : Theme.darkPrimaryColor
                        }
                    }

                    text: model.privateMode
                            ? (!tabView._emptyPrivateIcon ? webView.privateTabModel.count : "")
                            : (!tabView.privateMode && tabView._closingAllTabs ? 0 : webView.persistentTabModel.count)
                }
            }

            Rectangle {
                anchors.fill: parent

                z: -100
                color: tabView.palette.colorScheme === Theme.LightOnDark
                        ? Theme.darkPrimaryColor
                        : Theme.lightPrimaryColor
            }
        }
        _headerBackgroundVisible: false
        model: modeModel
        interactive: false
        currentIndex: privateMode ? 1 : 0
        delegate: Private.TabItem {
            id: tabItem
            property bool privateMode: model.privateMode
            allowDeletion: false
            flickable: _tabView

            TabGridView {
                id: _tabView

                portrait: tabView.portrait
                model: tabItem.privateMode ? webView.privateTabModel : webView.persistentTabModel
                header: Item {
                    width: 1
                    height: Theme.paddingLarge
                }

                onHide: tabView.hide()
                onEnterNewTabUrl: tabView.enterNewTabUrl()
                onActivateTab: tabView.activateTab(index)
                onCloseTab: tabView.closeTab(index)
                onCloseAll: tabView.closeAll()
                onCloseAllCanceled: tabView.closeAllCanceled()
                onCloseAllPending: tabView.closeAllPending()
            }

            onIsCurrentItemChanged: {
                if (isCurrentItem) {
                    _remorsePopup = Qt.binding(function() { return _tabView.remorsePopup })
                    _closingAllTabs = Qt.binding(function() { return _tabView.closingAllTabs })
                }
            }

            Connections {
                target: popupMenu
                onCloseAllTabs: {
                    if (isCurrentItem)
                        _tabView.closeAllTabs()
                }
            }

            PrivateModeTexture {
                visible: privateMode
                z: -1
            }
        }

        onCurrentIndexChanged: {
            if (_remorsePopup) {
                _remorsePopup.trigger()
            }
            privateMode = currentIndex !== 0
        }

        ListModel {
            id: modeModel

            ListElement {
                privateMode: false
            }
            ListElement {
                privateMode: true
            }
        }
    }

    TabsToolBar {
        id: tabsToolBar
        anchors.bottom: parent.bottom
        onBack: pageStack.pop()
        onEnterNewTabUrl: tabView.enterNewTabUrl()
        onOpenMenu: popupMenu.active = true
    }

    PopUpMenu {
        id: popupMenu

        signal closeAllTabs

        anchors.fill: parent
        active: false

        menuItem: Component {
            Item {
                id: menuItem_
                readonly property int iconWidth: Theme.iconSizeMedium + Theme.paddingLarge
                readonly property int verticalPadding: 3 * Theme.paddingSmall

                height: content.height + verticalPadding * 2

                Column {
                    id: content

                    y: verticalPadding
                    width: parent.width
                    spacing: Theme.paddingLarge

                    Column {
                        width: parent.width

                        OverlayListItem {
                            height: Theme.itemSizeSmall
                            iconWidth: menuItem_.iconWidth
                            iconSource: "image://theme/icon-m-tab-new"
                            enabled: !_closingAllTabs
                            //% "New tab"
                            text: qsTrId("sailfish_browser-la-new_tab")
                            onClicked: {
                                popupMenu.visible = false
                                // override to block animation
                                tabs.currentIndex = privateMode ? 1 : 0
                                tabView.privateMode = false
                                tabView.enterNewTabUrl()
                            }
                        }

                        OverlayListItem {
                            height: Theme.itemSizeSmall
                            iconWidth: menuItem_.iconWidth
                            iconSource: "image://theme/icon-m-incognito-new"
                            enabled: !_closingAllTabs
                            //% "New private tab"
                            text: qsTrId("sailfish_browser-la-new_private_tab")
                            onClicked: {
                                popupMenu.visible = false
                                // override to block animation
                                tabs.currentIndex = privateMode ? 1 : 0
                                tabView.privateMode = true
                                tabView.enterNewTabUrl()
                            }
                        }

                        OverlayListItem {
                            height: Theme.itemSizeSmall
                            iconWidth: menuItem_.iconWidth
                            iconSource: "image://theme/icon-m-tab-close"
                            enabled: showCloseAllAction.value && webView.tabModel.count && !_closingAllTabs
                            //% "Close all tabs"
                            text: qsTrId("sailfish_browser-me-close_all")
                            onClicked: {
                                popupMenu.active = false
                                popupMenu.closeAllTabs()
                            }
                        }
                    }
                }
            }
        }
        onClosed: active = false
    }

    ConfigurationValue {
        id: showCloseAllAction
        key: "/apps/sailfish-browser/settings/show_close_all"
        defaultValue: true
    }
}
