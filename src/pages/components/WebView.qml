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
    property bool firstUseFullscreen

    function stop() {
        if (contentItem) {
            contentItem.stop()
        }
    }

    // force property only used by WebView itself for deferred loading when
    // network connectivity is established or when loading initial web page.
    function load(url, title, force) {
//        if (url.substring(0, 6) !== "about:" && url.substring(0, 5) !== "file:"
//            && !connectionHelper.haveNetworkConnectivity()
//            && !deferredLoad) {

//            deferredReload = false
//            deferredLoad = {
//                "url": url,
//                "title": title
//            }
//            connectionHelper.attemptToConnectNetwork()
//            return
//        }

        // Modify url and title to string
        title = title ? "" + title : ""
        url = url ? "" + url : ""

        // This guarantees at that least one webview exists.
        if (tabModel.count == 0 && !tabModel.hasNewTabData) {
            tabModel.newTabData(url, title, null)
        }

        if (!tabModel.hasNewTabData || force || !webView.activatePage(tabModel.nextTabId)) {
            // First contentItem will be created once tab activated.
            if (contentItem) contentItem.loadTab(url, force)
        }
    }

    function reload() {
        if (!contentItem) {
            return
        }

        var url = contentItem.url.toString()
//        if (url.substring(0, 6) !== "about:" && url.substring(0, 5) !== "file:"
//            && !deferredReload
//            && !connectionHelper.haveNetworkConnectivity()) {

//            deferredReload = true
//            deferredLoad = null
//            connectionHelper.attemptToConnectNetwork()
//            return
//        }

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
    fullscreenMode: (contentItem && contentItem.chromeGestureEnabled && !contentItem.chrome) || webView.inputPanelVisible || !webView.foreground || (contentItem && contentItem.fullscreen) || firstUseFullscreen
    _readyToLoad: contentItem && contentItem.viewReady && tabModel.loaded

    loading: contentItem ? contentItem.loading : false
    favicon: contentItem ? contentItem.favicon : ""

    webPageComponent: webPageComponent

    tabModel: TabModel {
        // Enable browsing after new tab actually created or it was not even requested
        browsing: webView.active && !hasNewTabData && contentItem && contentItem.loaded
    }

    onTriggerLoad: webView.load(url, title)

    WebViewCreator {
        activeWebView: contentItem
        // onNewWindowRequested is always handled as synchronous operation (not through newTab).
        onNewWindowRequested: webView.loadNewTab(url, "", parentId)
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

            property int iconSize
            property string iconType

            loaded: loadProgress === 100 && !loading
            enabled: container.active
            // There needs to be enough content for enabling chrome gesture
            chromeGestureThreshold: container.toolbarHeight
            chromeGestureEnabled: contentHeight > container.height + chromeGestureThreshold

            signal selectionRangeUpdated(variant data)
            signal selectionCopied(variant data)
            signal contextMenuRequested(variant data)

            focus: true
            width: container.width
            height: container.height
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

            onUrlChanged: {
                if (url == "about:blank") return

                if (!PopupHandler.isRejectedGeolocationUrl(url)) {
                    PopupHandler.rejectedGeolocationUrl = ""
                }

                if (!PopupHandler.isAcceptedGeolocationUrl(url)) {
                    PopupHandler.acceptedGeolocationUrl = ""
                }
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

                // Refresh timers (if any) keep working even for suspended views. Hence
                // suspend the view again explicitly if app window is in background.
                if (loaded && webView.background) {
                    suspendView();
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
                    var parsedFavicon = false
                    if (data.href && data.rel === "icon"
                            && iconType !== "apple-touch-icon"
                            && iconType !== "apple-touch-icon-precomposed") {
                        var sizes = []
                        if (data.sizes) {
                            var digits = data.sizes.split("x")
                            var size = digits && digits.length > 0 && digits[0]
                            if (size) {
                                sizes.push(size)
                            }
                        } else {
                            sizes = data.href.match(/\d+/)
                        }
                        for (var i in sizes) {
                            var faviconSize = parseInt(sizes[i])
                            // Accept largest icon but one that is still smaller than icon size large.
                            if (faviconSize && faviconSize > iconSize && faviconSize <= Theme.iconSizeLarge) {
                                iconSize = faviconSize
                                parsedFavicon = true
                            }
                        }
                    }

                    if (data.rel === "shortcut icon"
                            || data.rel === "apple-touch-icon"
                            || data.rel === "apple-touch-icon-precomposed"
                            || parsedFavicon) {
                        favicon = data.href
                        iconType = data.rel
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
                when: container.inputPanelVisible || !container.foreground
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

//    ConnectionHelper {
//        id: connectionHelper

//        onNetworkConnectivityEstablished: {
//            var url
//            var title

//            if (deferredLoad) {
//                url = deferredLoad["url"]
//                title = deferredLoad["title"]
//                deferredLoad = null
//                webView.load(url, title, true)
//            } else if (deferredReload) {
//                deferredReload = false
//                contentItem.reload()
//            }
//        }

//        onNetworkConnectivityUnavailable: {
//            if (contentItem) {
//                deferredLoad = null
//                deferredReload = false
//            }
//        }
//    }

    ResourceController {
        id: resourceController
        webView: contentItem
        background: webView.background

        //onWebViewSuspended: connectionHelper.closeNetworkSession()
    }

    Timer {
        id: auxTimer

        interval: 1000
    }

    //Component.onDestruction: connectionHelper.closeNetworkSession()
    Component.onCompleted: {
        PopupHandler.auxTimer = auxTimer
        PopupHandler.pageStack = pageStack
        PopupHandler.webView = webView
        PopupHandler.resourceController = resourceController
        PopupHandler.WebUtils = WebUtils
        PopupHandler.tabModel = tabModel
    }
}
