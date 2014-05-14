/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import "components" as Browser


Page {
    id: browserPage

    property Item firstUseOverlay
    property alias firstUseFullscreen: webView.firstUseFullscreen
    property Component tabPageComponent

    property alias tabs: webView.tabModel
    property alias favorites: favoriteModel
    property alias history: historyModel
    property alias viewLoading: webView.loading
    property alias url: webView.url
    property alias title: webView.title
    property alias thumbnailPath: webView.thumbnailPath

    function load(url, title) {
        webView.load(url, title)
    }

    // Safety clipping. There is clipping in ApplicationWindow that should react upon focus changes.
    // This clipping can handle also clipping of QmlMozView. When this page is active we do not need to clip
    // if input method is not visible.
    clip: status != PageStatus.Active || webView.inputPanelVisible

    orientationTransitions: Transition {
        to: 'Portrait,Landscape,LandscapeInverted'
        from: 'Portrait,Landscape,LandscapeInverted'
        SequentialAnimation {
            PropertyAction {
                target: browserPage
                property: 'orientationTransitionRunning'
                value: true
            }
            ParallelAnimation {
                FadeAnimation {
                    target: webView.contentItem
                    to: 0
                    duration: 150
                }
                FadeAnimation {
                    target: !webView.fullscreenMode ? overlay : null
                    to: 0
                    duration: 150
                }
            }
            PropertyAction {
                target: browserPage
                properties: 'width,height,rotation,orientation'
            }
            ScriptAction {
                script: {
                    // Restores the Bindings to width, height and rotation
                    _defaultTransition = false
                    webView.resetHeight()
                    _defaultTransition = true
                }
            }
            FadeAnimation {
                target: !webView.fullscreenMode ? overlay : null
                to: 1
                duration: 150
            }
            // End-2-end implementation for OnUpdateDisplayPort should
            // give better solution and reduce visible relayoutting.
            FadeAnimation {
                target: webView.contentItem
                to: 1
                duration: 850
            }
            PropertyAction {
                target: browserPage
                property: 'orientationTransitionRunning'
                value: false
            }
        }
    }

//    onHeightChanged: console.log("Page height:", height)

    HistoryModel {
        id: historyModel
    }

    Browser.DownloadRemorsePopup { id: downloadPopup }
    Browser.WebView {
        id: webView

        readonly property bool moving: contentItem ? contentItem.moving : false

        enabled: overlayAnimator.allowContentUse
        visible: WebUtils.firstUseDone && height > 0
        active: browserPage.status === PageStatus.Active
        fullscreenHeight: portrait ? Screen.height : Screen.width
        portrait: browserPage.isPortrait
        maxLiveTabCount: 3
        toolBarHeight: overlay.toolBar.height

        tabModel.onCountChanged: {
            if (tabModel.count === 0 && browserPage.status === PageStatus.Active) {
                pageStack.push(tabPageComponent ? tabPageComponent : Qt.resolvedUrl("TabPage.qml"), {"browserPage" : browserPage, "initialSearchFocus": true })
            }
        }

//        onHeightChanged: console.log("WebView HEIGHT:", height)

        clip: true
    }

    Rectangle {
        width: webView.width
        height: Math.ceil(webView.height)
        opacity: 0.9 - (overlay.y / (webView.fullscreenHeight - overlay.toolBar.height)) * 0.9
        color: Theme.highlightDimmerColor

//        onYChanged: console.log("DIMMER Y :", y)
//        onHeightChanged: console.log("DIMMER HEIGHT:", height)

        MouseArea {
            anchors.fill: parent
            enabled: overlayAnimator.atTop
            onClicked: overlayAnimator.hide()
        }
    }

    Browser.OverlayAnimator {
        id: overlayAnimator

        overlay: overlay
        webView: webView
        portrait: browserPage.isPortrait
    }

    Browser.Overlay {
        id: overlay

        enabled: !webView.fullscreenMode && !webView.moving
        webView: webView
        historyModel: historyModel
        browserPage: browserPage
        overlayAnimator: overlayAnimator

//        onYChanged: console.log("Overlay Y :", y)
//        onHeightChanged: console.log("Overlay HEIGHT:", height)
    }


    CoverActionList {
        enabled: browserPage.status === PageStatus.Active
        iconBackground: true

        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: {
                overlay.openTabPage(true, true, PageStackAction.Immediate)
                activate()
            }
        }

        CoverAction {
            iconSource: webView.loading ? "image://theme/icon-cover-cancel" : "image://theme/icon-cover-refresh"
            onTriggered: {
                if (webView.loading) {
                    webView.stop()
                } else {
                    webView.reload()
                }
            }
        }
    }

    Connections {
        target: WebUtils
        onOpenUrlRequested: {
            if (!window.applicationActive) {
                window.activate()
            }
            // url is empty when user tapped icon when browser was already open.
            if (url == "") return

            // We have incoming URL so let's show it
            if (firstUseOverlay) {
                firstUseOverlay.destroy()
                webView.visible = true
            }
            webView.captureScreen()
            if (!webView.tabModel.activateTab(url)) {
                // Not found in tabs list, create newtab and load
                webView.load(url)
            }
            if (browserPage.status !== PageStatus.Active) {
                pageStack.pop(browserPage, PageStackAction.Immediate)
            }
        }
    }

    Component.onCompleted: {
        if (!WebUtils.firstUseDone) {
            var component = Qt.createComponent(Qt.resolvedUrl("components/FirstUseOverlay.qml"))
            if (component.status == Component.Ready) {
                firstUseOverlay = component.createObject(browserPage, {"width":browserPage.width, "height":browserPage.heigh, "gestureThreshold" : toolBar.height});
            } else {
                console.log("FirstUseOverlay create failed " + component.errorString())
            }
        }
    }

    BookmarkModel {
        id: favoriteModel
    }

    Browser.BrowserNotification {
        id: notification
    }

    // Compile TabPage
    Loader {
        id: tabPageCompiler
        asynchronous: true
        sourceComponent: Item {
            Component.onCompleted: tabPageComponent = Qt.createComponent(Qt.resolvedUrl("TabPage.qml"))
        }
    }
}
