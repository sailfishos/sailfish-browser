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
import "." as Browser
import "../../shared" as Shared

Item {
    id: root

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

    // TODO: Delete height calculation for Pulley Menu after adding popup menu
    readonly property int _pulleyMenuHeight: 7 * Theme.pixelRatio

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
                anchors {
                    fill: parent
                    topMargin: _pulleyMenuHeight
                }

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
                privateMode: false
                portrait: root.portrait
                model: tabItem.privateMode ? webView.privateTabModel : webView.persistentTabModel
                header: Item {
                    width: 1
                    height: tabs.tabBarHeight + Theme.paddingLarge
                }

                onHide: root.hide()
                onEnterNewTabUrl: root.enterNewTabUrl()
                onActivateTab: root.activateTab(index)
                onCloseTab: root.closeTab(index)
                onCloseAll: root.closeAll()
                onCloseAllCanceled: root.closeAllCanceled()
                onCloseAllPending: root.closeAllPending()
            }
            onIsCurrentItemChanged: {
                if (isCurrentItem) {
                    _remorsePopup = Qt.binding(function() { return _tabView.remorsePopup })
                }
            }

            Rectangle {
                color: Theme.colorScheme == Theme.LightOnDark ? "black" : "white"
                width: parent.width
                height: Math.max(0, _pulleyMenuHeight - y)
                y: Math.max(0, _pulleyMenuHeight - _tabView.contentY + _tabView.originY)
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
                    text: webView.persistentTabModel.count
                    font.pixelSize: Theme.fontSizeExtraSmall
                    font.bold: true
                    onTextChanged: parent.updateGrabImage()
                }
                Component.onCompleted: updateGrabImage()
            },
            Item {
                id: privateIcon
                property string grabIcon

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

                    source: webView.privateTabModel.count > 0 ? "image://theme/icon-m-incognito-selected" : "image://theme/icon-m-incognito"
                    visible: false
                }

                Label {
                    anchors.fill: _privateIcon
                    text: webView.privateTabModel.count > 0 ? webView.privateTabModel.count : ""
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
        onEnterNewTabUrl: root.enterNewTabUrl()
    }
}
