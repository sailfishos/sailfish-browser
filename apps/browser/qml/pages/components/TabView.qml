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

        anchors.fill: parent

        footer: Private.TabBar {
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

                onHide: tabView.hide()
                onActivateTab: tabView.activateTab(index)
                onCloseTab: tabView.closeTab(index)
                onCloseAll: tabView.closeAll()
                onCloseAllCanceled: tabView.closeAllCanceled()
                onCloseAllPending: tabView.closeAllPending()

                PullDownMenu {
                    MenuItem {
                        visible: tabView.showCloseAllAction.value && model.count
                        //% "Close all tabs"
                        text: qsTrId("sailfish_browser-me-close_all")
                        onClicked: _tabView.closeAllTabs()
                    }
                    MenuItem {
                        text: tabItem.privateMode
                            //% "New tab"
                            ? qsTrId("sailfish_browser-la-new_tab")
                            //% "New private tab"
                            : qsTrId("sailfish_browser-la-new_private_tab")
                        onClicked: {
                            // remove binding to block animation
                            tabs.currentIndex = tabs.currentIndex
                            tabView.privateMode = !tabItem.privateMode
                            tabView.enterNewTabUrl()
                        }
                    }
                    MenuItem {
                        text: tabItem.privateMode
                            //% "New private tab"
                            ? qsTrId("sailfish_browser-la-new_private_tab")
                            //% "New tab"
                            : qsTrId("sailfish_browser-la-new_tab")
                        onClicked: tabView.enterNewTabUrl()
                    }
                }
            }

            onIsCurrentItemChanged: {
                if (isCurrentItem) {
                    _remorsePopup = Qt.binding(function() { return _tabView.remorsePopup })
                    _closingAllTabs = Qt.binding(function() { return _tabView.closingAllTabs })
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

    ConfigurationValue {
        id: showCloseAllAction
        key: "/apps/sailfish-browser/settings/show_close_all"
        defaultValue: true
    }
}
