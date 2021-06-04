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
import org.nemomobile.configuration 1.0

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
                icon.source: model.privateMode ? privateIcon.grabIcon : persistentIcon.grabIcon
                icon.color: palette.primaryColor
            }

            Rectangle {
                anchors.fill: parent

                z: -100
                color: Theme.colorScheme == Theme.LightOnDark ? "black" : "white"
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

        children: [
            Image {
                id: persistentIcon
                property string grabIcon

                function updateGrabImage() {
                    persistentIcon.grabToImage(function(result) {
                        grabIcon = result.url
                    });
                }

                source: "image://theme/icon-m-tabs"
                visible: false
                Label {
                    anchors.centerIn: parent
                    text: !privateMode && _closingAllTabs ? 0 : webView.persistentTabModel.count
                    font.pixelSize: Theme.fontSizeExtraSmall
                    font.bold: true
                    onTextChanged: parent.updateGrabImage()
                }
                Component.onCompleted: updateGrabImage()
            },
            Item {
                id: privateIcon
                property string grabIcon
                property bool isEmptyIcon: (privateMode && _closingAllTabs) || webView.privateTabModel.count === 0

                function updateGrabImage() {
                    privateIcon.grabToImage(function(result) {
                        grabIcon = result.url
                    });
                }
                y: -500 // NOTE: Hiding drawing out of sight
                height: _privateIcon.implicitHeight
                width: _privateIcon.implicitWidth

                Image {
                    id: _privateIcon

                    source: privateIcon.isEmptyIcon ? "image://theme/icon-m-incognito" : "image://theme/icon-m-incognito-selected"
                    visible: false
                }

                Label {
                    anchors.fill: _privateIcon
                    text: !privateIcon.isEmptyIcon ? webView.privateTabModel.count : ""
                    font.pixelSize: Theme.fontSizeExtraSmall
                    font.bold: true
                    horizontalAlignment: Qt.AlignHCenter
                    verticalAlignment: Qt.AlignVCenter

                    layer.enabled: true
                    layer.samplerName: "maskSource"
                    layer.effect: ShaderEffect {
                        property variant source: _privateIcon
                        fragmentShader: "
                                    varying highp vec2 qt_TexCoord0;
                                    uniform highp float qt_Opacity;
                                    uniform lowp sampler2D source;
                                    uniform lowp sampler2D maskSource;
                                    void main(void) {
                                        gl_FragColor = texture2D(source, qt_TexCoord0.st) * (1.0-texture2D(maskSource, qt_TexCoord0.st).a) * qt_Opacity;
                                    }
                                "
                    }
                    onTextChanged: parent.updateGrabImage()
                }
                Component.onCompleted: updateGrabImage()
            }

        ]
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
