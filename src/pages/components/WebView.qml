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
import "." as Browser

WebContainer {
    id: webView

    property color _decoratorColor: Theme.highlightDimmerColor

    function stop() {
        if (contentItem) {
            contentItem.stop()
        }
    }

    // force property only used by WebView itself for deferred loading when
    // network connectivity is established or when loading initial web page.
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

        // Modify url and title to string
        title = title ? "" + title : ""
        url = url ? "" + url : ""

        // This guarantees at that least one webview exists.
        if (tabModel.count == 0 && !tabModel.hasNewTabData) {
            tabModel.newTabData(url, title, null)
        }

        // Bookmarks and history items pass url and title as arguments.
        // Only set url and title if title exists.
        if (title) {
            currentTab.title = title
            currentTab.url = url
        }

        if (!tabModel.hasNewTabData || force || !tabModel.activatePage(tabModel.nextTabId)) {
            // First contentItem will be created once tab activated.
            if (contentItem) contentItem.loadTab(url, force)
        }
    }

    function reload() {
        if (!contentItem) {
            return
        }

        var url = contentItem.url.toString()
        currentTab.url = url
        currentTab.title = contentItem.title

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

    width: parent.width
    height: portrait ? Screen.height : Screen.width
    foreground: Qt.application.active
    inputPanelHeight: window.pageStack.panelSize
    inputPanelOpenHeight: window.pageStack.imSize
    fullscreenMode: (contentItem && contentItem.chromeGestureEnabled && !contentItem.chrome) || webView.inputPanelVisible || !webView.foreground || (contentItem && contentItem.fullscreen)
    _firstFrameRendered: resourceController.firstFrameRendered
    _readyToLoad: contentItem && contentItem.viewReady && tabModel.loaded

    loading: contentItem ? contentItem.loading : false
    favicon: contentItem ? contentItem.favicon : ""

    tabModel: TabModel {
        webPageComponent: webPageComponent
        webView: webView

        // Enable browsing after new tab actually created or it was not even requested
        browsing: webView.active && !hasNewTabData && contentItem && contentItem.loaded
        onBrowsingChanged: if (browsing) captureScreen()

        onTriggerLoad: webView.load(url, title)
    }

    // Triggered when tabs of tab model are available and QmlMozView is ready to load.
    // Load test
    // 1) tabModel.hasNewTabData -> loadTab (already activated view)
    // 2) model has tabs, load active tab -> load (activate view when needed)
    // 3) load home page -> load (activate view when needed)
    on_ReadyToLoadChanged: {
        // visible could be possible delay property for _readyToLoad if so wanted.
        if (!_readyToLoad) {
            return
        }

        if (tabModel.hasNewTabData) {
            contentItem.loadTab(tabModel.newTabUrl, false)
        } else if (tabModel.count > 0) {
            // First tab is actived when tabs are loaded to the tabs tabModel.
            tabModel.resetNewTabData()
            webView.load(currentTab.url, currentTab.title)
        } else {
            // This can happen only during startup.
            webView.load(WebUtils.homePage, "")
        }
    }

    WebViewCreator {
        activeWebView: contentItem
        onNewWindowRequested: tabModel.newTab(url, "", parentId)
    }

    Rectangle {
        id: background
        anchors.fill: parent
        color: contentItem && contentItem.bgcolor ? contentItem.bgcolor : "white"
    }

    Component {
        id: webPageComponent
        WebPage {
            id: webPage

            loaded: loadProgress === 100 && !loading

            function loadTab(newUrl, force) {
                // Always enable chrome when load is called.
                chrome = true
                if ((newUrl !== "" && url != newUrl) || force) {
                    resourceController.firstFrameRendered = false
                    webPage.load(newUrl)
                }

                // This looks like a not needed condition for now. However, if we add a max number of real tabs
                // limit then this could make sense again.
                else if (url == newUrl && tabModel.hasNewTabData) {
                    // Url will not change when the very same url is already loaded. Thus, we just add tab directly.
                    // This is currently the only exception. Normally tab is added after engine has
                    // resolved the url.
                    tabModel.addTab(newUrl, tabModel.newTabTitle)
                    tabModel.resetNewTabData()
                }
            }

            enabled: container.active
            // There needs to be enough content for enabling chrome gesture
            chromeGestureThreshold: container.toolbarHeight
            chromeGestureEnabled: contentHeight > container.height + chromeGestureThreshold

            signal selectionRangeUpdated(variant data)
            signal selectionCopied(variant data)
            signal contextMenuRequested(variant data)

            focus: true
            width: container.width
            state: ""

            onLoadProgressChanged: {
                // Ignore first load progress if it is directly 50%
                if (container.loadProgress === 0 && loadProgress === 50) {
                    return
                }

                if (loadProgress > container.loadProgress) {
                    container.loadProgress = loadProgress
                }
            }

            onTitleChanged: tabModel.updateTitle(tabId, title)
            onUrlChanged: {
                if (url == "about:blank") return

                if (!PopupHandler.isRejectedGeolocationUrl(url)) {
                    PopupHandler.rejectedGeolocationUrl = ""
                }

                if (!PopupHandler.isAcceptedGeolocationUrl(url)) {
                    PopupHandler.acceptedGeolocationUrl = ""
                }

                tabModel.updateUrl(tabId, url)
            }

            onBgcolorChanged: {
                // Update only webPage
                if (container.contentItem === webPage) {
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
                addMessageListener("embed:filepicker")

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
                    if (resurrectedContentRect) {
                        sendAsyncMessage("embedui:zoomToRect",
                                         {
                                             "x": resurrectedContentRect.x, "y": resurrectedContentRect.y,
                                             "width": resurrectedContentRect.width, "height": resurrectedContentRect.height
                                         })
                        resurrectedContentRect = null
                    }
                }
            }

            onLoadingChanged: {
                if (loading) {
                    userHasDraggedWhileLoading = false
                    container.loadProgress = 0
                    webPage.chrome = true
                    favicon = ""
                    container.resetHeight(false)
                }
            }

            onRecvAsyncMessage: {
                switch (message) {
                case "chrome:linkadded": {
                    if (data.rel === "shortcut icon") {
                        favicon = data.href
                    }
                    break
                }
                case "embed:filepicker": {
                    PopupHandler.openFilePicker(data)
                    break
                }
                case "embed:selectasync": {
                    PopupHandler.openSelectDialog(data)
                    break;
                }
                case "embed:alert": {
                    PopupHandler.openAlert(data)
                    break
                }
                case "embed:confirm": {
                    PopupHandler.openConfirm(data)
                    break
                }
                case "embed:prompt": {
                    PopupHandler.openPrompt(data)
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
                    webPage.selectionRangeUpdated(data)
                    break
                }
                }
            }
            onRecvSyncMessage: {
                // sender expects that this handler will update `response` argument
                switch (message) {
                case "Content:SelectionCopied": {
                    webPage.selectionCopied(data)

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
                when: container.inputPanelVisible || !container.foreground || !visible
                PropertyChanges {
                    target: webPage
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
        visible: contentItem && contentItem.contentHeight > contentItem.height && !contentItem.pinching && !popupActive
        opacity: contentItem && contentItem.verticalScrollDecorator.moving ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { properties: "opacity"; duration: 400 } }
    }

    Rectangle {
        id: horizontalScrollDecorator

        width: contentItem ? contentItem.horizontalScrollDecorator.size : 0
        height: 5
        x: contentItem ? contentItem.horizontalScrollDecorator.position : 0
        y: webView.parent.height - (fullscreenMode ? 0 : toolbarHeight) - height
        z: 1
        color: _decoratorColor
        smooth: true
        radius: 2.5
        visible: contentItem && contentItem.contentWidth > contentItem.width && !contentItem.pinching && !popupActive
        opacity: contentItem && contentItem.horizontalScrollDecorator.moving ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { properties: "opacity"; duration: 400 } }
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
                webView.load(url, title, true)
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
        background: webView.background

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

    Component.onDestruction: connectionHelper.closeNetworkSession()
    Component.onCompleted: {
        PopupHandler.auxTimer = auxTimer
        PopupHandler.pageStack = pageStack
        PopupHandler.webView = webView
        PopupHandler.resourceController = resourceController
        PopupHandler.WebUtils = WebUtils
        PopupHandler.tabModel = tabModel
    }
}
