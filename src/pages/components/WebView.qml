/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import Qt5Mozilla 1.0
import org.nemomobile.connectivity 1.0
import "WebViewTabCache.js" as TabCache
import "WebPopupHandler.js" as PopupHandler
import "WebPromptHandler.js" as PromptHandler

WebContainer {
    id: webContainer

    // This cannot be bindings in multiple mozview case. Will change in
    // later commits.
    property bool active
    // This property should cover all possible popus
    property alias popupActive: webPopups.active

    property bool loading
    property int loadProgress
    property Item contentItem
    property alias tabModel: model
    property alias currentTab: tab
    readonly property bool fullscreenMode: (contentItem && contentItem.chromeGestureEnabled && !contentItem.chrome) || webContainer.inputPanelVisible || !webContainer.foreground
    property alias canGoBack: tab.canGoBack
    property alias canGoForward: tab.canGoForward

    readonly property alias url: tab.url
    readonly property alias title: tab.title
    property string favicon

    // Groupped properties
    property alias popups: webPopups
    property alias prompts: webPrompts

    // TODO : This must be encapsulated into a newTab / loadTab function
    // Load goes so that we first use engine load to resolve url
    // and then save that resolved url to the history. This way
    // urls that resolve to download urls won't get saved to the
    // history (as those won't trigger url change). As an added bonus
    // a redirected url will be saved with the redirected url not with
    // the input url.
    //
    // "newWebView" branch has some building blocks to help here.
    property var newTabData
    // Move to C++
    readonly property bool _readyToLoad: contentItem &&
                                         contentItem.viewReady &&
                                         tabModel.loaded
    property color _decoratorColor: Theme.highlightDimmerColor

    function goBack() {
        tab.backForwardNavigation = true
        tab.goBack()
    }

    function goForward() {
        // This backForwardNavigation is internal of WebView
        tab.backForwardNavigation = true
        tab.goForward()
    }

    function stop() {
        if (contentItem) {
            contentItem.stop()
        }
    }

    function load(url, title, force) {
        if (url.substring(0, 6) !== "about:" && url.substring(0, 5) !== "file:"
            && !connectionHelper.haveNetworkConnectivity()
            && !contentItem._deferredLoad) {

            contentItem._deferredReload = false
            contentItem._deferredLoad = {
                "url": url,
                "title": title
            }
            connectionHelper.attemptToConnectNetwork()
            return
        }

        // This guarantees at that least one webview exists.
        if (tabModel.count == 0) {
            // Url is not need in webContainer.newTabData as we let engine to resolve
            // the url and use the resolved url.
            webContainer.newTabData = { "title": title }
        }

        // Bookmarks and history items pass url and title as arguments.
        if (title) {
            tab.title = title
        } else {
            tab.title = ""
        }

        // Always enable chrome when load is called.
        contentItem.chrome = true
        if ((url !== "" && contentItem.url != url) || force) {
            tab.url = url
            resourceController.firstFrameRendered = false
            contentItem.load(url)
        } else if (contentItem.url == url && webContainer.newTabData) {
            // Url will not change when the very same url is already loaded. Thus, we just add tab directly.
            // This is currently the only exception. Normally tab is added after engine has
            // resolved the url.
            tabModel.addTab(url, webContainer.newTabData.title)
        }
    }

    function reload() {
        if (!contentItem) {
            return
        }

        var url = contentItem.url.toString()
        tab.url = url

        if (url.substring(0, 6) !== "about:" && url.substring(0, 5) !== "file:"
            && !contentItem._deferredReload
            && !connectionHelper.haveNetworkConnectivity()) {

            contentItem._deferredReload = true
            contentItem._deferredLoad = null
            connectionHelper.attemptToConnectNetwork()
            return
        }

        contentItem.reload()
    }

    function sendAsyncMessage(name, data) {
        if (!contentItem) {
            return
        }

        contentItem.sendAsyncMessage(name, data)
    }

    function captureScreen() {
        if (!contentItem) {
            return
        }

        if (active && resourceController.firstFrameRendered) {
            var size = Screen.width
            if (browserPage.isLandscape && !webContainer.fullscreenMode) {
                size -= toolbarRow.height
            }

            tab.captureScreen(contentItem.url, 0, 0, size, size, browserPage.rotation)
        }
    }

    visible: WebUtils.firstUseDone
    width: parent.width
    height: browserPage.orientation === Orientation.Portrait ? Screen.height : Screen.width

    // TODO: Rename pageActive to active and remove there the beginning
    pageActive: active
    webView: contentItem

    foreground: Qt.application.active
    inputPanelHeight: window.pageStack.panelSize
    inputPanelOpenHeight: window.pageStack.imSize
    toolbarHeight: toolBarContainer.height

    on_ReadyToLoadChanged: {
        if (!visible || !_readyToLoad) {
            return
        }

        if (WebUtils.initialPage !== "") {
            webContainer.load(WebUtils.initialPage)
        } else if (tabModel.count > 0) {
            // First tab is actived when tabs are loaded to the tabs model.
            webContainer.load(tab.url, tab.title)
        } else {
            webContainer.load(WebUtils.homePage, "")
        }
    }

    Rectangle {
        id: background
        anchors.fill: parent
        color: contentItem && contentItem.bgcolor ? contentItem.bgcolor : "white"
    }

    TabModel {
        id: model

        function newTab(url, title) {
            // This might be something that we don't want to have.
            if (contentItem && contentItem.loading) {
                contentItem.stop()
            }
            captureScreen()

            // Url is not need in webView.newTabData as we let engine to resolve
            // the url and use the resolved url.
            newTabData = { "title": title }
            load(url, title)
        }

        currentTab: tab
    }

    Tab {
        id: tab

        // Used with back and forward navigation.
        property bool backForwardNavigation: false

        onUrlChanged: {
            if (tab.valid && backForwardNavigation && url != "about:blank") {
                // Both url and title are updated before url changed is emitted.
                load(url, title)
            }
        }
    }

    Component {
        id: webViewComponent
        QmlMozView {
            id: webView

            property Item container
            property Item tab
            readonly property bool loaded: loadProgress === 100
            property bool userHasDraggedWhileLoading
            property bool viewReady

            property bool _deferredReload
            property var _deferredLoad: null

            enabled: container.active
            // There needs to be enough content for enabling chrome gesture
            chromeGestureThreshold: container.toolbarHeight
            chromeGestureEnabled: contentHeight > container.height + chromeGestureThreshold

            signal selectionRangeUpdated(variant data)
            signal selectionCopied(variant data)
            signal contextMenuRequested(variant data)

            focus: true
            width: container.parent.width
            state: ""

            onLoadProgressChanged: {
                if (loadProgress > container.loadProgress) {
                    container.loadProgress = loadProgress
                }
            }

            onTitleChanged: tab.title = title
            onUrlChanged: {
                if (url == "about:blank") return

                if (!PopupHandler.isRejectedGeolocationUrl(url)) {
                    PopupHandler.rejectedGeolocationUrl = ""
                }

                if (!PopupHandler.isAcceptedGeolocationUrl(url)) {
                    PopupHandler.acceptedGeolocationUrl = ""
                }

                if (tab.backForwardNavigation) {
                    tab.updateTab(url, tab.title)
                    tab.backForwardNavigation = false
                } else if (!newTabData) {
                    // WebView.load() updates title before load starts.
                    tab.navigateTo(url)
                } else {
                    // Delay adding of the new tab until url has been resolved.
                    // Url will not change if there is download link behind.
                    tabModel.addTab(url, newTabData.title)
                }

                newTabData = null
            }

            onBgcolorChanged: {
                // Update only webView
                if (container.contentItem === webView) {
                    var bgLightness = WebUtils.getLightness(bgcolor)
                    var dimmerLightness = WebUtils.getLightness(Theme.highlightDimmerColor)
                    var highBgLightness = WebUtils.getLightness(Theme.highlightBackgroundColor)

                    if (Math.abs(bgLightness - dimmerLightness) > Math.abs(bgLightness - highBgLightness)) {
                        container._decoratorColor = Theme.highlightDimmerColor
                    } else {
                        container._decoratorColor =  Theme.highlightBackgroundColor
                    }

                    sendAsyncMessage("Browser:SelectionColorUpdate",
                                     {
                                         "color": Theme.secondaryHighlightColor
                                     })
                }
            }

            onViewInitialized: {
                addMessageListener("chrome:linkadded")
                addMessageListener("embed:alert")
                addMessageListener("embed:confirm")
                addMessageListener("embed:prompt")
                addMessageListener("embed:auth")
                addMessageListener("embed:login")
                addMessageListener("embed:permissions")
                addMessageListener("Content:ContextMenu")
                addMessageListener("Content:SelectionRange");
                addMessageListener("Content:SelectionCopied");
                addMessageListener("embed:selectasync")

                loadFrameScript("chrome://embedlite/content/SelectAsyncHelper.js")
                loadFrameScript("chrome://embedlite/content/embedhelper.js")

                viewReady = true
            }

            onDraggingChanged: {
                if (dragging && loading) {
                    userHasDraggedWhileLoading = true
                }
            }

            onLoadedChanged: {
                if (loaded && !userHasDraggedWhileLoading) {
                    container.resetHeight(false)
                }
            }

            onLoadingChanged: {
                container.loading = loading
                if (loading) {
                    userHasDraggedWhileLoading = false
                    container.favicon = ""
                    webView.chrome = true
                    container.resetHeight(false)
                }
            }

            onRecvAsyncMessage: {
                switch (message) {
                case "chrome:linkadded": {
                    if (data.rel === "shortcut icon") {
                        container.favicon = data.href
                    }
                    break
                }
                case "embed:selectasync": {
                    PopupHandler.openSelectDialog(data)
                    break;
                }
                case "embed:alert": {
                    PromptHandler.openAlert(data)
                    break
                }
                case "embed:confirm": {
                    PromptHandler.openConfirm(data)
                    break
                }
                case "embed:prompt": {
                    PromptHandler.openPrompt(data)
                    break
                }
                case "embed:auth": {
                    PopupHandler.openAuthDialog(data)
                    break
                }
                case "embed:permissions": {
                    PopupHandler.openLocationDialog(data)
                    break
                }
                case "embed:login": {
                    PopupHandler.openPasswordManagerDialog(data)
                    break
                }
                case "Content:ContextMenu": {
                    PopupHandler.openContextMenu(data)
                    break
                }
                case "Content:SelectionRange": {
                    webView.selectionRangeUpdated(data)
                    break
                }
                }
            }
            onRecvSyncMessage: {
                // sender expects that this handler will update `response` argument
                switch (message) {
                case "Content:SelectionCopied": {
                    webView.selectionCopied(data)

                    if (data.succeeded) {
                        //% "Copied to clipboard"
                        notification.show(qsTrId("sailfish_browser-la-selection_copied"))
                    }
                    break
                }
                }
            }

            // We decided to disable "text selection" until we understand how it
            // should look like in Sailfish.
            // TextSelectionController {}
            states: State {
                name: "boundHeightControl"
                when: container.inputPanelVisible || !container.foreground
                PropertyChanges {
                    target: webView
                    height: container.parent.height
                }
            }
        }
    }

    Rectangle {
        id: verticalScrollDecorator

        width: 5
        height: contentItem ? contentItem.verticalScrollDecorator.size : 0
        y: contentItem ? contentItem.verticalScrollDecorator.position : 0
        z: 1
        anchors.right: contentItem ? contentItem.right: undefined
        color: _decoratorColor
        smooth: true
        radius: 2.5
        visible: contentItem && contentItem.contentHeight > contentItem.height && !contentItem.pinching && !webPopups.active
        opacity: contentItem && contentItem.verticalScrollDecorator.moving ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { properties: "opacity"; duration: 400 } }
    }

    Rectangle {
        id: horizontalScrollDecorator

        width: contentItem ? contentItem.horizontalScrollDecorator.size : 0
        height: 5
        x: contentItem ? contentItem.horizontalScrollDecorator.position : 0
        y: webContainer.parent.height - (fullscreenMode ? 0 : toolBarContainer.height) - height
        z: 1
        color: _decoratorColor
        smooth: true
        radius: 2.5
        visible: contentItem && contentItem.contentWidth > contentItem.width && !contentItem.pinching && !webPopups.active
        opacity: contentItem && contentItem.horizontalScrollDecorator.moving ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { properties: "opacity"; duration: 400 } }
    }

    Connections {
        target: tabModel

        onCountChanged: webContainer.newTabData = null

        // arguments of the signal handler: int tabId
        onActiveTabChanged: {
            webContainer.newTabData = null

            if (!TabCache.initialized) {
                TabCache.init({"tab": tab, "container": webContainer},
                              webViewComponent, webContainer)
            }

            if (tabId > 0) {
                webContainer.contentItem = TabCache.getView(tabId)
            }

            // When all tabs are closed, we're in invalid state.
            if (tab.valid && webContainer._readyToLoad) {
                webContainer.load(tab.url, tab.title)
            }
            webContainer.currentTabChanged()
        }

        // arguments of the signal handler: int tabId
        onTabClosed: TabCache.releaseView(tabId)
    }

    ConnectionHelper {
        id: connectionHelper

        onNetworkConnectivityEstablished: {
            var url
            var title

            // TODO: this should be deferred till view created.
            if (contentItem && contentItem._deferredLoad) {
                url = contentItem._deferredLoad["url"]
                title = contentItem._deferredLoad["title"]
                contentItem._deferredLoad = null

                webContainer.load(url, title, true)
            } else if (contentItem && contentItem._deferredReload) {
                contentItem._deferredReload = false
                contentItem.reload()
            }
        }

        onNetworkConnectivityUnavailable: {
            if (contentItem) {
                contentItem._deferredLoad = null
                contentItem._deferredReload = false
            }
        }
    }

    ResourceController {
        id: resourceController
        webView: contentItem
        background: webContainer.background

        onWebViewSuspended: connectionHelper.closeNetworkSession()
        onFirstFrameRenderedChanged: {
            if (firstFrameRendered) {
                captureScreen()
            }
        }
    }

    Timer {
        id: auxTimer

        interval: 1000
    }

    QtObject {
        id: webPopups

        property bool active

        // url support is missing and these should be typed as urls.
        // We don't want to create component before it's needed.
        property string authenticationComponentUrl
        property string passwordManagerComponentUrl
        property string contextMenuComponentUrl
        property string selectComponentUrl
        property string locationComponentUrl
    }

    QtObject {
        id: webPrompts

        property string alertComponentUrl
        property string confirmComponentUrl
        property string queryComponentUrl
    }

    Component.onDestruction: connectionHelper.closeNetworkSession()
    Component.onCompleted: {
        PopupHandler.auxTimer = auxTimer
        PopupHandler.pageStack = pageStack
        PopupHandler.popups = webPopups
        PopupHandler.componentParent = browserPage
        PopupHandler.resourceController = resourceController
        PopupHandler.WebUtils = WebUtils

        PromptHandler.pageStack = pageStack
        PromptHandler.prompts = webPrompts
    }
}
