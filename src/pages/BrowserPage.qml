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
import Sailfish.Silica.private 1.0
import Sailfish.Browser 1.0
import "components" as Browser


Page {
    id: browserPage

    property Item firstUseOverlay
    property Item debug
    property alias firstUseFullscreen: webView.firstUseFullscreen
    property Component tabPageComponent

    property alias tabs: webView.tabModel
    property alias history: historyModel
    property alias viewLoading: webView.loading
    property alias url: webView.url
    property alias title: webView.title
    property alias thumbnailPath: webView.thumbnailPath

    property alias imageLoader: imageLoader
    property alias desktopBookmarkWriter: desktopBookmarkWriter
    property alias webView: webView

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

    onStatusChanged: {
        if (status === PageStatus.Inactive) {
            MozContext.sendObserve("memory-pressure", null)
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
        toolbarHeight: overlay.toolBar.height

        bookmarkModel: BookmarkModel {}

        onCanGoBackChanged: console.log("CAN GO BACK:", canGoBack)
        clip: true
    }

    Rectangle {
        width: webView.width
        height: Math.ceil(webView.height)
        opacity: 0.9 - (overlay.y / (webView.fullscreenHeight - overlay.toolBar.height)) * 0.9
        color: Theme.highlightDimmerColor

        MouseArea {
            anchors.fill: parent
            enabled: overlay.animator.atTop
            onClicked: overlay.animator.showChrome()
        }
    }

    Browser.Overlay {
        id: overlay

        enabled: !webView.fullscreenMode && !webView.moving
        webView: webView
        historyModel: historyModel
        browserPage: browserPage
    }

    CoverActionList {
        enabled: browserPage.status === PageStatus.Active && (Config.sailfishVersion >= 2.0)
        iconBackground: true

        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: {
                overlay.openNewTabView(PageStackAction.Immediate)
                activate()
            }
        }
    }

    // TODO: remove once we move to sailfish 2.0
    CoverActionList {
        enabled: browserPage.status === PageStatus.Active && (Config.sailfishVersion < 2.0)
        iconBackground: true

        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: {
                overlay.openNewTabView(PageStackAction.Immediate)
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

    DesktopBookmarkWriter {
        id: desktopBookmarkWriter
        minimumIconSize: Theme.iconSizeMedium
    }

    Image {
        id: imageLoader
        x: Screen.height * 2
        sourceSize.width: Theme.iconSizeLauncher
    }
}
