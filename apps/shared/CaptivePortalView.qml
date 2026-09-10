/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */
import QtQuick 2.6
import Qt5Mozilla 1.0
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import Sailfish.WebView.Controls 1.0
import Sailfish.WebView.Pickers 1.0 as Pickers
import Sailfish.WebView.Popups 1.0 as Popups
import Sailfish.WebView 1.0 as SailfishWebView

BrowserContentView {
    id: webPage

    Binding {
        target: webView && webView.nativeWindow ? webPage : null
        property: "presentationWindow"
        value: webView ? webView.nativeWindow : null
    }
    property var webView
    privateMode: true
    active: browserPage.active
    orientation: webView._screenOrientation
    clip: true
    property string _qmozChromeInitialUrl: ""
    dynamicToolbarHeight: forcedChrome ? 0 : webView.toolbarHeight
    marginBottom: webView.imOpened ? virtualKeyboardHeight : (forcedChrome ? webView.toolbarHeight : 0)

    function syncState() {
        webView.updateHostedState(String(url), title, loading, loadProgress,
                                  canGoBack, canGoForward, security, true)
    }

    HostedTabSession {
        id: viewSession

        model: webView.privateTabModel
        view: webPage
        privateMode: true
    }
    Connections {
        target: webPage.tabModel
        onRevisionChanged: viewSession.applyRuntimeSnapshot(false)
    }
    Connections {
        target: webView
        onHostedLoadRequested: {
            if (webPage.selectedTabId.length) webPage.load(url, fromExternal)
            else webView.tabModel.newTab(url, fromExternal)
        }
        onHostedReloadRequested: webPage.reload()
        onHostedGoBackRequested: webPage.goBack()
        onHostedGoForwardRequested: webPage.goForward()
    }
    onSelectedTabChanged: { viewSession.applyRuntimeSnapshot(false); syncState() }
    onTabCloseResult: viewSession.runtimeTabCloseResult(tabId, closed)
    onLoadingChanged: syncState()
    onUrlChanged: syncState()
    onLoadProgressChanged: syncState()
    onCanGoBackChanged: syncState()
    onCanGoForwardChanged: syncState()
    onSecurityChanged: syncState()

    property bool acceptedTouchIcon
    readonly property bool textSelectionActive: textSelectionController && textSelectionController.active
    property Item textSelectionController: null
    readonly property bool activeWebPage: viewSession.selectedPersistentId(webPage) === String(webView.tabId)
    property string favicon
    property string metadataTitle
    property var pendingClipboardPasteData
    property QtObject _textZoomController: SailfishWebView.TextZoomController {
        webPage: webPage
    }

    property QtObject pickerOpener: Pickers.PickerOpener {
        pageStack: window.pageStack
        contentItem: webPage
    }

    property QtObject popupOpener: Popups.PopupOpener {
        pageStack: window.pageStack
        parentItem: browserPage
        contentItem: webPage
        // ContextMenu needs a reference to correct TabModel so that
        // private and public tabs are created to correct model. While context
        // menu is open, tab model cannot change (at least at the moment).
        tabModel: webView.tabModel

        onAboutToOpenContextMenu: {
            if (Qt.inputMethod.visible) {
                browserPage.focus = true
                Qt.inputMethod.hide()
            }

            contextMenuRequested(data)
        }

    }

    function effectiveTitle() {
        return metadataTitle || title || String(url)
    }

    signal selectionCopied(var data)
    signal contextMenuRequested(var data)

    function clearSelection() {
        if (textSelectionController) {
            textSelectionController.clearSelection()
            browserPage.inputRegion.selectionStartHandleMask = Qt.rect(0, 0, 0, 0)
            browserPage.inputRegion.selectionEndHandleMask = Qt.rect(0, 0, 0, 0)
        }
    }

    function sendClipboardPasteResponse(data, accepted) {
        var response = {
            "id": data.id,
            "accepted": accepted
        }
        if (data.winId) {
            response.winId = data.winId
        }
        webPage.sendAsyncMessage("embedui:clipboardreadpasteresponse", response)
    }

    function openPendingClipboardPasteDialog() {
        if (window.pageStack.busy || !pendingClipboardPasteData) {
            return
        }

        window.pageStack.busyChanged.disconnect(openPendingClipboardPasteDialog)
        var data = pendingClipboardPasteData
        pendingClipboardPasteData = null
        openClipboardPasteDialog(data)
    }

    function openClipboardPasteDialog(data) {
        if (window.pageStack.busy) {
            if (pendingClipboardPasteData) {
                sendClipboardPasteResponse(pendingClipboardPasteData, false)
            } else {
                window.pageStack.busyChanged.connect(openPendingClipboardPasteDialog)
            }
            pendingClipboardPasteData = data
            return
        }

        var page = window.pageStack.animatorPush(webView.clipboardPasteDialogComponent, {
            "origin": data.origin || "",
            "delay": Math.max(0, data.delay || 0)
        })
        page.pageCompleted.connect(function(dialog) {
            dialog.accepted.connect(function() {
                sendClipboardPasteResponse(data, true)
            })
            dialog.rejected.connect(function() {
                sendClipboardPasteResponse(data, false)
            })
        })
    }

    property bool fixedToolbar: webView.fixedToolbarConfig.value
    readonly property bool forcedChrome: fixedToolbar || webView.imOpened
    property real toolbarHeight: webView.toolbarHeight
    property int virtualKeyboardHeight
    property string viewportFit: "auto"
    property int safeAreaInsetUsage
    property bool hasThemeColor
    property color themeColor: "white"
    safeAreaInsetTop: webView.displayCutoutAllowed ? webView._contentCutoutTop : 0
    safeAreaInsetRight: webView.displayCutoutAllowed ? webView._contentCutoutRight : 0
    safeAreaInsetBottom: webView.displayCutoutAllowed ? webView._contentCutoutBottom : 0
    safeAreaInsetLeft: webView.displayCutoutAllowed ? webView._contentCutoutLeft : 0
    throttlePainting: !webView.foreground && !webView.resourceController.videoActive && webView.applicationVisible || !webView.applicationVisible
    enabled: webView.enabled
    chromeGestureThreshold: toolbarHeight / 3
    chromeGestureEnabled: !forcedChrome && enabled && !webView.imOpened && !fixedToolbar

    onTitleChanged: {
        syncState()
        if (title) {
            metadataTitle = title
        }
    }

    onAtYBeginningChanged: {
        if (atYBeginning && activeWebPage && domContentLoaded) {
            chrome = true
        }
    }

    onAtYEndChanged: {
        // Don't hide chrome if content length is short i.e. forcedChrome is enabled.
        if (!atYBeginning && atYEnd && !forcedChrome && !fixedToolbar && chrome
                && activeWebPage && domContentLoaded) {
            chrome = false
        }
    }

    onBackgroundColorChanged: {
        // Update only webPage
        if (webView.contentItem === webPage) {
            sendAsyncMessage("Browser:SelectionColorUpdate",
                             {
                                 "color": Theme.secondaryHighlightColor
                             })
        }
    }

    onRecvAsyncMessage: {
        if (pickerOpener.message(message, data) || popupOpener.message(message, data)) {
            return
        }

        switch (message) {
        case "embed:viewportfit": {
            viewportFit = data.viewportFit || data.value || "auto"
            hasThemeColor = !!data.themeColor
            if (hasThemeColor) themeColor = data.themeColor
            safeAreaInsetUsage = Number(data.safeAreaInsetUsage || 0)
            break
        }
        case "embed:fullscreenchanged": {
            break
        }
        case "embed:clipboardreadpaste": {
            openClipboardPasteDialog(data)
            break
        }
        case "Link:SetIcon": {
            if (acceptedTouchIcon)
                return

            var previousFavicon = favicon
            acceptedTouchIcon = !!data.isRichIcon
            favicon = data.url

            break
        }
        case "embed:pageMetadata": {
            if (data.url && data.url !== String(url)) {
                break
            }

            if (data.title) {
                metadataTitle = data.title
            }

            var richIcon = !!data.isRichIcon
            if (data.favicon && (richIcon || !acceptedTouchIcon)) {
                var oldFavicon = favicon
                acceptedTouchIcon = richIcon
                favicon = data.favicon

            }
            break
        }
        case "Content:SelectionRange": {
            if (textSelectionController === null) {
                textSelectionController = webView.textSelectionControllerComponent.createObject(browserPage,
                                                                                        {"contentItem": webPage})
            }
            textSelectionController.selectionRangeUpdated(data)
            break
        }
        case "Content:SelectionSwap": {
            if (textSelectionController) {
                textSelectionController.swap()
            }

            break
        }
        case "embed:find": {
            // Found, or found wrapped
            if (data.r == 0 || data.r == 2) {
                webView.findInPageHasResult = true
            } else {
                webView.findInPageHasResult = false
            }
            break
        }
        // embed:OpenLink listener is registered only in the captive portal mode
        case "embed:OpenLink": {
            webView.linkHandler.handleLink(data.uri)
            break
        }
        case "Link:AddSearch": {
            if (!webView.privateMode) {
                // This adds this search as available if not already there
                SearchEngineModel.add(data.engine.title, data.engine.href)
            }
            break
        }
        }
    }
    onRecvSyncMessage: {
        // sender expects that this handler will update `response` argument
        switch (message) {
        case "Content:SelectionCopied": {
            if (data.succeeded && textSelectionController) {
                textSelectionController.showNotification()
                response.message = {"": ""}
            }
            break
        }
        }
    }

    onContextMenuRequested: {
        if (data.types.indexOf("content-text") !== -1) {
            // we want to select some content text
            webPage.sendAsyncMessage("Browser:SelectionStart", {"xPos": data.xPos, "yPos": data.yPos})
        }
    }

    Component.onCompleted: {
        loadFrameScript(Qt.resolvedUrl("ViewportFit.js"))
        loadFrameScript(Qt.resolvedUrl("PageMetadata.js"))
        loadFrameScript("file:///usr/share/sailfish-captiveportal/pages/captiveportal.js")
        var listeners = ["embed:OpenLink", "embed:viewportfit", "embed:pageMetadata",
                         "embed:fullscreenchanged", "embed:alert", "embed:confirm",
                         "embed:prompt", "embed:auth", "embed:login", "embed:permissions",
                         "embed:webrtcrequest", "embed:select", "embed:selectasync",
                         "embed:colorpicker", "embed:filepicker", "Content:ContextMenu"]
        for (var i = 0; i < listeners.length; ++i) addMessageListener(listeners[i])
        viewSession.restoreRuntimeTabs(webPage)
        addMessageListener("Content:SelectionRange")
        addMessageListener("Content:SelectionCopied")
        addMessageListener("Content:SelectionSwap")
        addMessageListener("embed:clipboardreadpaste")

        PermissionManager.instance()
    }
}
