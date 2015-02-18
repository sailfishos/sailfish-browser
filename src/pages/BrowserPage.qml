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
    property Item debug
    property Component tabPageComponent

    property alias tabs: webView.tabModel
    property alias history: historyModel
    property alias viewLoading: webView.loading
    property alias url: webView.url
    property alias title: webView.title
    property alias webView: webView

    function load(url, title) {
        webView.load(url, title)
    }

    function bringToForeground() {
        if (!window.applicationActive) {
            window.activate()
        }
    }

    function activateNewTabView() {
        pageStack.pop(browserPage, PageStackAction.Immediate);
        overlay.enterNewTabUrl(PageStackAction.Immediate)
        bringToForeground()
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

    HistoryModel {
        id: historyModel
    }

    Browser.DownloadRemorsePopup { id: downloadPopup }
    Browser.WebView {
        id: webView

        enabled: overlay.animator.allowContentUse
        fullscreenHeight: portrait ? Screen.height : Screen.width
        portrait: browserPage.isPortrait
        maxLiveTabCount: 3
        toolbarHeight: overlay.toolBar.toolsHeight
        clip: true
    }

    Rectangle {
        id: contentDimmer
        width: webView.width
        height: Math.ceil(webView.height)
        opacity: 0.9 - (overlay.y / (webView.fullscreenHeight - overlay.toolBar.toolsHeight)) * 0.9
        color: Theme.highlightDimmerColor

        MouseArea {
            anchors.fill: parent
            enabled: overlay.animator.atTop && (webView.tabModel.count > 0 || firstUseOverlay)
            onClicked: overlay.dismiss()
        }

        Browser.PrivateModeTexture {
            id: privateModeTexture
            anchors.fill: contentDimmer
            visible: webView.privateMode && !overlay.animator.allowContentUse
        }
    }

    Label {
        x: (contentDimmer.width - implicitWidth) / 2
        // Allow only half of the width
        width: parent.width / 2
        truncationMode: TruncationMode.Fade
        opacity: privateModeTexture.visible ? 1.0 : 0.0
        anchors {
            bottom: contentDimmer.bottom
            bottomMargin: (overlay.toolBar.toolsHeight - height) / 2
        }

        //: Label for private browsing above address bar
        //% "Private browsing"
        text: qsTrId("sailfish_browser-la-private_mode")
        color: Theme.highlightColor
        font.pixelSize: Theme.fontSizeLarge

        Behavior on opacity { FadeAnimation {} }
    }

    Browser.Overlay {
        id: overlay

        active: browserPage.status == PageStatus.Active
        webView: webView
        historyModel: historyModel
        browserPage: browserPage
    }

    CoverActionList {
        enabled: browserPage.status === PageStatus.Active && webView.contentItem
        iconBackground: true

        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: activateNewTabView()
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
            // url is empty when user tapped icon when browser was already open.
            if (url == "") {
                bringToForeground()
                return
            }

            // We have incoming URL so let's show it
            if (firstUseOverlay) {
                firstUseOverlay.dismiss()
            }

            if (browserPage.status !== PageStatus.Active) {
                pageStack.pop(browserPage, PageStackAction.Immediate)
            }

            webView.grabActivePage()
            if (!webView.tabModel.activateTab(url)) {
                // Not found in tabs list, load it. A new tab will be created if no tabs exist.
                webView.load(url)
                overlay.animator.showChrome(true)
            }
            bringToForeground()
        }
        onActivateNewTabViewRequested: {
            activateNewTabView()
        }
    }

    Component.onCompleted: {
        WebUtils.silicaPixelRatio = Theme.pixelRatio
        WebUtils.touchSideRadius = Theme.paddingMedium + Theme.paddingLarge
        WebUtils.touchTopRadius = Theme.paddingLarge * 2
        WebUtils.touchBottomRadius = Theme.paddingMedium + Theme.paddingSmall
        WebUtils.inputItemSize = Theme.fontSizeSmall
        WebUtils.zoomMargin = Theme.paddingMedium

        if (!WebUtils.firstUseDone) {
            var component = Qt.createComponent(Qt.resolvedUrl("components/FirstUseOverlay.qml"))
            if (component.status == Component.Ready) {
                // Parent to browserPage so that FirstUseOverlay is visible as WebView is invisible
                // when FirstUseOverlay is visible.
                firstUseOverlay = component.createObject(browserPage, {
                                                             "width": webView.width,
                                                             "height": webView.height,
                                                             "fullscreenHeight": browserPage.height,
                                                             "gestureThreshold" : webView.toolbarHeight / 2});
            } else {
                console.log("FirstUseOverlay create failed " + component.errorString())
            }
        }

        if (WebUtils.debugMode) {
            component = Qt.createComponent(Qt.resolvedUrl("components/DebugOverlay.qml"))
            if (component.status === Component.Ready) {
                debug = component.createObject(browserPage)
            } else {
                console.warn("Failed to create DebugOverlay " + component.errorString())
            }
        }
    }

    Browser.BrowserNotification {
        id: notification
    }
}
