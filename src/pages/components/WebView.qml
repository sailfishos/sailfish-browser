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
import "WebPopupHandler.js" as PopupHandler
import "WebPromptHandler.js" as PromptHandler

WebContainer {
    id: webContainer

    // This cannot be bindings in multiple mozview case. Will change in
    // later commits.
    property bool active
    // This property should cover all possible popus
    property alias popupActive: webPopups.active

    property alias loading: webView.loading
    property int loadProgress
    property alias contentItem: webView
    property alias tabModel: model
    property alias currentTab: tab
    readonly property bool fullscreenMode: (webView.chromeGestureEnabled && !webView.chrome) || webContainer.inputPanelVisible || !webContainer.foreground
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
        webView.stop()
    }

    function load(url, title, force) {
        if (url.substring(0, 6) !== "about:" && url.substring(0, 5) !== "file:"
            && !connectionHelper.haveNetworkConnectivity()
            && !webView._deferredLoad) {

            webView._deferredReload = false
            webView._deferredLoad = {
                "url": url,
                "title": title
            }
            connectionHelper.attemptToConnectNetwork()
            return
        }

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
        webView.chrome = true
        if ((url !== "" && webView.url != url) || force) {
            tab.url = url
            resourceController.firstFrameRendered = false
            webView.load(url)
        } else if (webView.url == url && webContainer.newTabData) {
            // Url will not change when the very same url is already loaded. Thus, we just add tab directly.
            // This is currently the only exception. Normally tab is added after engine has
            // resolved the url.
            tabModel.addTab(url, webContainer.newTabData.title)
        }
    }

    function reload() {
        var url = webView.url.toString()
        tab.url = url

        if (url.substring(0, 6) !== "about:" && url.substring(0, 5) !== "file:"
            && !webView._deferredReload
            && !connectionHelper.haveNetworkConnectivity()) {

            webView._deferredReload = true
            webView._deferredLoad = null
            connectionHelper.attemptToConnectNetwork()
            return
        }

        webView.reload()
    }

    function sendAsyncMessage(name, data) {
        webView.sendAsyncMessage(name, data)
    }

    function captureScreen() {
        if (active && resourceController.firstFrameRendered) {
            var size = Screen.width
            if (browserPage.isLandscape && !webContainer.fullscreenMode) {
                size -= toolbarRow.height
            }

            tab.captureScreen(webView.url, 0, 0, size, size, browserPage.rotation)
        }
    }

    width: parent.width
    height: browserPage.orientation === Orientation.Portrait ? Screen.height : Screen.width

    // TODO: Rename pageActive to active and remove there the beginning
    pageActive: active
    webView: webView

    foreground: Qt.application.active
    inputPanelHeight: window.pageStack.panelSize
    inputPanelOpenHeight: window.pageStack.imSize
    toolbarHeight: toolBarContainer.height

    Rectangle {
        id: background
        anchors.fill: parent
        color: webView.bgcolor ? webView.bgcolor : "white"
    }

    TabModel {
        id: model

        function newTab(url, title) {
            // This might be something that we don't want to have.
            if (webView.loading) {
                webView.stop()
            }
            webView.captureScreen()

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
            if (tab.valid && backForwardNavigation) {
                // Both url and title are updated before url changed is emitted.
                load(url, title)
            }
        }
    }

    QmlMozView {
        id: webView

        readonly property bool loaded: loadProgress === 100
        readonly property bool readyToLoad: viewReady && tabModel.loaded
        property bool userHasDraggedWhileLoading
        property bool viewReady

        property bool _deferredReload
        property var _deferredLoad: null

        visible: WebUtils.firstUseDone
        enabled: parent.active
        // There needs to be enough content for enabling chrome gesture
        chromeGestureThreshold: toolBarContainer.height
        chromeGestureEnabled: contentHeight > webContainer.height + chromeGestureThreshold

        signal selectionRangeUpdated(variant data)
        signal selectionCopied(variant data)
        signal contextMenuRequested(variant data)

        focus: true
        width: browserPage.width
        state: ""

        onReadyToLoadChanged: {
            if (!visible) {
                return
            }

            if (WebUtils.initialPage !== "") {
                browserPage.load(WebUtils.initialPage)
            } else if (tabModel.count > 0) {
                // First tab is actived when tabs are loaded to the tabs model.
                webContainer.load(tab.url, tab.title)
            } else {
                webContainer.load(WebUtils.homePage, "")
            }
        }

        onLoadProgressChanged: {
            if (loadProgress > webContainer.loadProgress) {
                webContainer.loadProgress = loadProgress
            }
        }

        onTitleChanged: tab.title = title
        onUrlChanged: {
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
                // Use browserPage.title here to avoid wrong title to blink.
                // browserPage.load() updates browserPage's title before load starts.
                // QmlMozView's title is not correct over here.
                tab.navigateTo(url)
            } else {
                // Delay adding of the new tab until url has been resolved.
                // Url will not change if there is download link behind.
                tabModel.addTab(url, newTabData.title)
            }

            newTabData = null
        }

        onBgcolorChanged: {
            var bgLightness = WebUtils.getLightness(bgcolor)
            var dimmerLightness = WebUtils.getLightness(Theme.highlightDimmerColor)
            var highBgLightness = WebUtils.getLightness(Theme.highlightBackgroundColor)

            if (Math.abs(bgLightness - dimmerLightness) > Math.abs(bgLightness - highBgLightness)) {
                verticalScrollDecorator.color = Theme.highlightDimmerColor
                horizontalScrollDecorator.color = Theme.highlightDimmerColor
            } else {
                verticalScrollDecorator.color = Theme.highlightBackgroundColor
                horizontalScrollDecorator.color = Theme.highlightBackgroundColor
            }

            sendAsyncMessage("Browser:SelectionColorUpdate",
                             {
                                 "color": Theme.secondaryHighlightColor
                             })
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
                webContainer.resetHeight(false)
            }
        }

        onLoadingChanged: {
            if (loading) {
                userHasDraggedWhileLoading = false
                webContainer.favicon = ""
                webView.chrome = true
                webContainer.resetHeight(false)
            }
        }
        onRecvAsyncMessage: {
            switch (message) {
            case "chrome:linkadded": {
                if (data.rel === "shortcut icon") {
                    webContainer.favicon = data.href
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

        Rectangle {
            id: verticalScrollDecorator

            width: 5
            height: webView.verticalScrollDecorator.size
            y: webView.verticalScrollDecorator.position
            anchors.right: parent ? parent.right: undefined
            color: Theme.highlightDimmerColor
            smooth: true
            radius: 2.5
            visible: webView.contentHeight > webView.height && !webView.pinching && !webPopups.active
            opacity: webView.verticalScrollDecorator.moving ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { properties: "opacity"; duration: 400 } }
        }

        Rectangle {
            id: horizontalScrollDecorator
            width: webView.horizontalScrollDecorator.size
            height: 5
            x: webView.horizontalScrollDecorator.position
            y: browserPage.height - (fullscreenMode ? 0 : toolBarContainer.height) - height
            color: Theme.highlightDimmerColor
            smooth: true
            radius: 2.5
            visible: webView.contentWidth > webView.width && !webView.pinching && !webPopups.active
            opacity: webView.horizontalScrollDecorator.moving ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { properties: "opacity"; duration: 400 } }
        }

        states: State {
            name: "boundHeightControl"
            when: webContainer.inputPanelVisible || !webContainer.foreground
            PropertyChanges {
                target: webView
                height: browserPage.height
            }
        }
    }

    Connections {
        target: tabModel

        onCountChanged: webContainer.newTabData = null

        onActiveTabChanged: {
            webContainer.newTabData = null
            // Not yet actually changed but new activeWebView
            // needs to be set to PopupHandler
            PopupHandler.activeWebView = webView
            // Stop previous webView
            if (webView.loading) {
                webView.stop()
            }

            // Not yet actually changed but new activeWebView
            // needs to be set to PopupHandler
            PopupHandler.activeWebView = webView
            PromptHandler.activeWebView = webView

            // When all tabs are closed, we're in invalid state.
            if (tab.valid && webView.readyToLoad) {
                webContainer.load(tab.url, tab.title)
            }
            webContainer.currentTabChanged()
        }
    }

    ConnectionHelper {
        id: connectionHelper

        onNetworkConnectivityEstablished: {
            var url
            var title

            if (webView._deferredLoad) {
                url = webView._deferredLoad["url"]
                title = webView._deferredLoad["title"]
                webView._deferredLoad = null

                browserPage.load(url, title, true)
            } else if (webView._deferredReload) {
                webView._deferredReload = false
                webView.reload()
            }
        }

        onNetworkConnectivityUnavailable: {
            webView._deferredLoad = null
            webView._deferredReload = false
        }
    }

    ResourceController {
        id: resourceController
        webView: webView
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
        PopupHandler.activeWebView = webView
        PopupHandler.componentParent = browserPage
        PopupHandler.resourceController = resourceController
        PopupHandler.WebUtils = WebUtils

        PromptHandler.pageStack = pageStack
        PromptHandler.activeWebView = webView
        PromptHandler.prompts = webPrompts
    }
}
