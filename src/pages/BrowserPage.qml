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
    property alias tabs: webView.tabModel
    property alias favorites: favoriteModel
    property alias history: historyModel
    property alias viewLoading: webView.loading
    property alias currentTab: webView.currentTab

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
                    webView.resetHeight(true)
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

    HistoryModel {
        id: historyModel

        tabId: webView.tabModel.currentTabId
    }

    Browser.DownloadRemorsePopup { id: downloadPopup }
    Browser.WebView {
        id: webView

        visible: WebUtils.firstUseDone
        active: browserPage.status === PageStatus.Active
        toolbarHeight: toolBarContainer.height
        portrait: browserPage.isPortrait

        tabModel.onCountChanged: {
            if (tabModel.count === 0 && browserPage.status === PageStatus.Active) {
                pageStack.push(Qt.resolvedUrl("TabPage.qml"), {"browserPage" : browserPage, "initialSearchFocus": true })
            }
        }
    }

    Column {
        id: controlArea

        anchors.bottom: webView.bottom
        width: parent.width
        visible: !webView.popupActive
        opacity: webView.fullscreenMode ? 0.0 : 1.0
        Behavior on opacity { FadeAnimation { duration: webView.foreground ? 300 : 0 } }

        function openTabPage(focus, newTab, operationType) {
            if (browserPage.status === PageStatus.Active) {
                webView.captureScreen()
                pageStack.push(Qt.resolvedUrl("TabPage.qml"),
                               {
                                   "browserPage" : browserPage,
                                   "initialSearchFocus": focus,
                                   "newTab": newTab
                               }, operationType)
            }
        }

        Browser.StatusBar {
            width: parent.width
            height: visible ? toolBarContainer.height * 3 : 0
            visible: isPortrait
            opacity: progressBar.opacity
            title: webView.title
            url: webView.url
            onSearchClicked: controlArea.openTabPage(true, false, PageStackAction.Animated)
            onCloseClicked: webView.tabModel.closeActiveTab()
        }

        Browser.ToolBarContainer {
            id: toolBarContainer
            width: parent.width
            enabled: !webView.fullscreenMode

            Browser.ProgressBar {
                id: progressBar
                anchors.fill: parent
                opacity: webView.loading ? 1.0 : 0.0
                progress: webView.loadProgress / 100.0
            }

            // ToolBar
            Row {
                id: toolbarRow

                anchors {
                    left: parent.left; leftMargin: Theme.paddingMedium
                    right: parent.right; rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }

                // 5 icons, 4 spaces between
                spacing: isPortrait ? (width - (backIcon.width * 5)) / 4 : Theme.paddingSmall

                Browser.IconButton {
                    visible: isLandscape
                    source: "image://theme/icon-m-close"
                    onClicked: webView.tabModel.closeActiveTab()
                }

                // Spacer
                Item {
                    visible: isLandscape
                    height: Theme.itemSizeSmall
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
                    source: "image://theme/icon-m-back"
                    enabled: webView.canGoBack
                    onClicked: webView.goBack()
                }

                Browser.IconButton {
                    enabled: webView.visible
                    property bool favorited: favorites.count > 0 && favorites.contains(webView.currentTab.url)
                    source: favorited ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"
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
                    source: "image://theme/icon-m-tabs"
                    onClicked: {
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
                        color: tabPageButton.down ? Theme.highlightDimmerColor : Theme.highlightColor
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                Browser.IconButton {
                    enabled: webView.visible
                    source: webView.loading ? "image://theme/icon-m-reset" : "image://theme/icon-m-refresh"
                    onClicked: webView.loading ? webView.stop() : webView.reload()
                }

                Browser.IconButton {
                    source: "image://theme/icon-m-forward"
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

                // url is empty when user tapped icon when browser was already open.
                if (url == "" && !launch) return
            }

            if (webView.url != "") {
                webView.captureScreen()
                if (!webView.tabModel.activateTab(url)) {
                    // Not found in tabs list, create newtab and load
                    webView.tabModel.newTab(url, "")
                }
            } else {
                // New browser instance, just load the content
                if (firstUseOverlay) {
                    firstUseOverlay.destroy()
                    webView.visible = true
                }
                // TabModel.newTab and WebView are handling empty urls and triggers
                // view creation when needed.
                webView.tabModel.newTab(url, "")
            }
            if (browserPage.status !== PageStatus.Active) {
                pageStack.pop(browserPage, PageStackAction.Immediate)
            }
        }
        onFirstUseDoneChanged: {
            if (WebUtils.firstUseDone && firstUseOverlay) {
                firstUseOverlay.destroy()
            }
        }
    }

    Component.onCompleted: {
        if (!WebUtils.firstUseDone) {
            var component = Qt.createComponent(Qt.resolvedUrl("components/FirstUseOverlay.qml"))
            if (component.status == Component.Ready) {
                firstUseOverlay = component.createObject(browserPage, {"width": browserPage.width, "height": browserPage.height - toolBarContainer.height });
            } else {
                console.log("FirstUseOverlay create failed " + component.status)
            }
        }
    }

    BookmarkModel {
        id: favoriteModel
    }

    Browser.BrowserNotification {
        id: notification
    }
}
