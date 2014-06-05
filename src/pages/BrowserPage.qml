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
    property alias firstUseFullscreen: webView.firstUseFullscreen
    property Component tabPageComponent

    property alias tabs: webView.tabModel
    property alias favorites: favoriteModel
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
                    target: !webView.fullscreenMode ? controlArea : null
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
                target: !webView.fullscreenMode ? controlArea : null
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

        visible: WebUtils.firstUseDone
        active: browserPage.status === PageStatus.Active
        toolbarHeight: toolBarContainer.height
        fullscreenHeight: portrait ? Screen.height : Screen.width
        portrait: browserPage.isPortrait
        maxLiveTabCount: 3

        tabModel.onCountChanged: {
            if (tabModel.count === 0 && browserPage.status === PageStatus.Active) {
                pageStack.push(tabPageComponent ? tabPageComponent : Qt.resolvedUrl("TabPage.qml"), {"browserPage" : browserPage, "initialSearchFocus": true })
            }
        }

        clip: true
        // TODO: once we get rid of bad rendering loop, check if we could use here parent.height
        // instead of fullscreenHeight. Currently with parent.height binding we skip
        // frames when returning back from tab page so that virtual keyboard was open.
        height: fullscreenHeight - (fullscreenMode ? 0 : toolBarContainer.height)

        Behavior on height {
            enabled: !browserPage.orientationTransitionRunning
            NumberAnimation { duration: 200; easing.type: Easing.InOutQuad }
        }
    }

    // TODO: This will change once toolbar can be pulled up.
    Column {
        id: controlArea

        z: 1
        y: webView.height
        width: parent.width
        visible: !webView.popupActive

        function openTabPage(focus, newTab, operationType) {
            if (browserPage.status === PageStatus.Active) {
                webView.captureScreen()
                pageStack.push(tabPageComponent ? tabPageComponent : Qt.resolvedUrl("TabPage.qml"),
                                                  {
                                                      "browserPage" : browserPage,
                                                      "initialSearchFocus": focus,
                                                      "newTab": newTab
                                                  }, operationType)
            }
        }

        Browser.ToolBarContainer {
            id: toolBarContainer
            width: parent.width
            enabled: !webView.fullscreenMode

            Browser.ProgressBar {
                id: progressBar
                anchors.fill: parent
                visible: !firstUseOverlay
                opacity: webView.loading ? 1.0 : 0.0
                progress: webView.loadProgress / 100.0
            }

            // ToolBar
            Row {
                id: toolbarRow

                anchors {
                    left: parent.left; leftMargin: isPortrait ? 0 : Theme.paddingMedium
                    right: parent.right; rightMargin: isPortrait ? 0 : Theme.paddingMedium
                }
                height: parent.height

                // 5 icons, 4 spaces between
                spacing: isPortrait ? (width - (backIcon.width * 5)) / 4 : Theme.paddingSmall

                Browser.IconButton {
                    visible: isLandscape
                    icon.source: "image://theme/icon-m-close"
                    onClicked: webView.tabModel.closeActiveTab()
                }

                // Spacer
                Item {
                    visible: isLandscape
                    height: parent.height
                    width: browserPage.width
                           - toolbarRow.spacing * (toolbarRow.children.length - 1)
                           - backIcon.width * (toolbarRow.children.length - 1)
                           - parent.anchors.leftMargin
                           - parent.anchors.rightMargin

                    Browser.TitleBar {
                        url: webView.url
                        title: webView.title
                        height: parent.height
                        onClicked: controlArea.openTabPage(true, false, PageStackAction.Animated)
                        // Workaround for binding loop jb#15182
                        clip: true
                    }
                }

                Browser.IconButton {
                    id:backIcon
                    icon.source: "image://theme/icon-m-back"
                    enabled: webView.canGoBack
                    onClicked: webView.goBack()
                }

                Browser.IconButton {
                    property bool favorited: favorites.count > 0 && favorites.contains(webView.url)
                    enabled: webView.visible
                    icon.source: favorited ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"
                    onClicked: {
                        if (favorited) {
                            favorites.removeBookmark(webView.url)
                        } else {
                            favorites.addBookmark(webView.url, webView.title, webView.favicon)
                        }
                    }
                }

                Browser.IconButton {
                    id: tabPageButton
                    icon.source: "image://theme/icon-m-tabs"
                    onClicked: {
                        if (firstUseOverlay) {
                            firstUseOverlay.visible = false
                            firstUseOverlay.destroy()
                        }
                        if (!WebUtils.firstUseDone) WebUtils.firstUseDone = true
                        controlArea.openTabPage(false, false, PageStackAction.Animated)
                    }
                    Label {
                        visible: webView.tabModel.count > 0
                        text: webView.tabModel.count
                        x: (parent.width - contentWidth) / 2 - 5
                        y: (parent.height - contentHeight) / 2 - 5
                        font.pixelSize: Theme.fontSizeExtraSmall
                        font.bold: true
                        color: tabPageButton.down ?  Theme.primaryColor : Theme.highlightDimmerColor
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                Browser.IconButton {
                    enabled: webView.visible
                    icon.source: webView.loading ? "image://theme/icon-m-reset" : "image://theme/icon-m-refresh"
                    onClicked: webView.loading ? webView.stop() : webView.reload()
                }

                Browser.IconButton {
                    icon.source: "image://theme/icon-m-forward"
                    enabled: webView.canGoForward
                    onClicked: webView.goForward()
                }
            }
        }
    }

    CoverActionList {
        enabled: browserPage.status === PageStatus.Active
        iconBackground: true

        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: {
                controlArea.openTabPage(true, true, PageStackAction.Immediate)
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
                firstUseOverlay = component.createObject(browserPage, {"width":browserPage.width, "height":browserPage.heigh, "gestureThreshold" : toolBarContainer.height});
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
