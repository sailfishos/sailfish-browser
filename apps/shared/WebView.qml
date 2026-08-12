/****************************************************************************
**
** Copyright (c) 2014 - 2021 Jolla Ltd.
** Copyright (c) 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import QtQuick.Window 2.2 as QuickWindow
import Nemo.Configuration 1.0
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import Sailfish.WebView.Pickers 1.0 as Pickers
import Sailfish.WebView.Popups 1.0 as Popups
import Sailfish.WebView.Controls 1.0
import Sailfish.WebView 1.0 as SailfishWebView
import Sailfish.WebEngine 1.0
import Sailfish.Policy 1.0
import Sailfish.TextLinking 1.0
import "." as Browser

WebContainer {
    id: webView

    property bool activePortalMode
    readonly property bool moving: contentItem && contentItem.moving
    property bool portrait: true
    property bool contentFullscreen: contentItem && contentItem.fullscreen
    property bool needChrome: !contentItem || (contentItem.chrome && !contentItem.fullscreen)
    property real fullscreenHeight
    property bool imOpened
    property real toolbarHeight
    property string favicon: contentItem ? contentItem.favicon : ""
    readonly property color _defaultThemeColor: WebEngineSettings.colorScheme === WebEngineSettings.PrefersLightMode
            || (WebEngineSettings.colorScheme === WebEngineSettings.FollowsAmbience
                && Theme.colorScheme !== Theme.LightOnDark) ? "white" : "black"
    readonly property color themeColor: contentItem && contentItem.hasThemeColor
            ? contentItem.themeColor : _defaultThemeColor
    property bool findInPageHasResult
    property bool canShowSelectionMarkers: true
    readonly property int _topCutoutInset: Math.max(0, Screen.topCutout.y + Screen.topCutout.height)
    readonly property int _safeAreaInsetTop: 1
    readonly property int _safeAreaInsetRight: 2
    readonly property int _safeAreaInsetBottom: 4
    readonly property int _safeAreaInsetLeft: 8
    readonly property int _pageOrientation: rotationHandler ? rotationHandler.orientation : Orientation.Portrait
    readonly property int _screenOrientation: _qtScreenOrientation(_pageOrientation)
    readonly property int _contentOrientation: _validCutoutOrientation(pendingWebContentOrientation)
            ? pendingWebContentOrientation : _screenOrientation
    readonly property int _contentCutoutTop: _cutoutTop(_contentOrientation)
    readonly property int _contentCutoutRight: _cutoutRight(_contentOrientation)
    readonly property int _contentCutoutBottom: _cutoutBottom(_contentOrientation)
    readonly property int _contentCutoutLeft: _cutoutLeft(_contentOrientation)
    readonly property int _contentCutoutInsetUsage: _safeAreaInsetUsage(
            _contentCutoutTop, _contentCutoutRight, _contentCutoutBottom, _contentCutoutLeft)
    readonly property int _hostBaseCutoutTop: _cutoutTop(_screenOrientation)
    readonly property int _hostBaseCutoutRight: _cutoutRight(_screenOrientation)
    readonly property int _hostBaseCutoutBottom: _cutoutBottom(_screenOrientation)
    readonly property int _hostBaseCutoutLeft: _cutoutLeft(_screenOrientation)
    readonly property bool _hostBaseCutoutVertical: _hostBaseCutoutTop > 0 || _hostBaseCutoutBottom > 0
    readonly property bool _hostBaseCutoutHorizontal: _hostBaseCutoutLeft > 0 || _hostBaseCutoutRight > 0
    readonly property int _hostCutoutTop: width <= height && _hostBaseCutoutHorizontal
            ? _topCutoutInset : (width > height && _hostBaseCutoutVertical ? 0 : _hostBaseCutoutTop)
    readonly property int _hostCutoutRight: width <= height && _hostBaseCutoutHorizontal
            ? 0 : (width > height && _hostBaseCutoutVertical ? 0 : _hostBaseCutoutRight)
    readonly property int _hostCutoutBottom: width <= height && _hostBaseCutoutHorizontal
            ? 0 : (width > height && _hostBaseCutoutVertical ? 0 : _hostBaseCutoutBottom)
    readonly property int _hostCutoutLeft: width > height && _hostBaseCutoutVertical
            ? _topCutoutInset : (width <= height && _hostBaseCutoutHorizontal ? 0 : _hostBaseCutoutLeft)
    readonly property bool coverViewportFit: contentItem && contentItem.viewportFit === "cover"
    readonly property string _viewportFitCoverPolicy: _normalizedCutoutGuard(cutoutGuardConfig.value)
    readonly property bool _safeAreaUsedForContentCutout: contentItem
            && _contentCutoutInsetUsage !== 0
            && (contentItem.safeAreaInsetUsage & _contentCutoutInsetUsage) === _contentCutoutInsetUsage
    readonly property bool _policyAllowsCoverViewportFit: coverViewportFit
            && (_viewportFitCoverPolicy === "strict"
                || (_viewportFitCoverPolicy === "top_guard" && _safeAreaUsedForContentCutout))
    readonly property bool displayCutoutAllowed: contentFullscreen || _policyAllowsCoverViewportFit
    webContentRect: displayCutoutAllowed
                    ? Qt.rect(0, 0, width, height)
                    : Qt.rect(_hostCutoutLeft,
                              _hostCutoutTop,
                              Math.max(0, width - _hostCutoutLeft - _hostCutoutRight),
                              Math.max(0, height - _hostCutoutTop - _hostCutoutBottom))
    webContentBackgroundColor: displayCutoutAllowed ? _defaultThemeColor : themeColor

    function _qtScreenOrientation(pageOrientation) {
        switch (pageOrientation) {
        case Orientation.Landscape:
            return Qt.LandscapeOrientation
        case Orientation.PortraitInverted:
            return Qt.InvertedPortraitOrientation
        case Orientation.LandscapeInverted:
            return Qt.InvertedLandscapeOrientation
        default:
            return Qt.PortraitOrientation
        }
    }

    function _validCutoutOrientation(orientation) {
        switch (orientation) {
        case Qt.PortraitOrientation:
        case Qt.InvertedLandscapeOrientation:
        case Qt.InvertedPortraitOrientation:
        case Qt.LandscapeOrientation:
            return true
        default:
            return false
        }
    }

    function _cutoutTop(orientation) {
        return orientation === Qt.PortraitOrientation ? _topCutoutInset : 0
    }

    function _cutoutRight(orientation) {
        return orientation === Qt.InvertedLandscapeOrientation ? _topCutoutInset : 0
    }

    function _cutoutBottom(orientation) {
        return orientation === Qt.InvertedPortraitOrientation ? _topCutoutInset : 0
    }

    function _cutoutLeft(orientation) {
        return orientation === Qt.LandscapeOrientation ? _topCutoutInset : 0
    }

    function _safeAreaInsetUsage(top, right, bottom, left) {
        var usage = 0
        if (top > 0) {
            usage |= _safeAreaInsetTop
        }
        if (right > 0) {
            usage |= _safeAreaInsetRight
        }
        if (bottom > 0) {
            usage |= _safeAreaInsetBottom
        }
        if (left > 0) {
            usage |= _safeAreaInsetLeft
        }
        return usage
    }

    function _normalizedCutoutGuard(policy) {
        switch (policy) {
        case "strict":
        case "compat":
        case "top_guard":
            return policy
        default:
            return "top_guard"
        }
    }

    property var resourceController: ResourceController {
        webPage: contentItem
        background: !webView.visible
    }

    property var _webPageCreator: WebPageCreator {
        activeWebPage: contentItem
        model: tabModel
    }

    property Component textSelectionControllerComponent: Component {
        TextSelectionController {
            opacity: canShowSelectionMarkers ? 1.0 : 0.0
            contentWidth: webView.rotationHandler ? webView.rotationHandler.width : 0
            contentHeight: Math.max(0, webView.fullscreenHeight - webView.toolbarHeight)
            // Push below the overlay
            z: -1
            anchors {
                fill: parent
                bottomMargin: webView.toolbarHeight
            }

            Behavior on opacity { FadeAnimator {} }

            onStartHandleMaskChanged: browserPage.inputRegion.selectionStartHandleMask = startHandleMask
            onEndHandleMaskChanged: browserPage.inputRegion.selectionEndHandleMask = endHandleMask
        }
    }

    property var linkHandler: LinkHandler {}

    property ConfigurationValue fixedToolbarConfig: ConfigurationValue {
        key: "/apps/sailfish-browser/settings/fixed_toolbar"
        defaultValue: false
    }

    property ConfigurationValue cutoutGuardConfig: ConfigurationValue {
        key: "/apps/sailfish-browser/settings/cutout_guard"
        defaultValue: "top_guard"
    }

    function stop() {
        if (contentItem) {
            contentItem.stop()
        }
    }

    function clearSelection() {
        if (contentItem) {
            contentItem.clearSelection()
        }
    }

    function sendAsyncMessage(name, data) {
        if (!contentItem) {
            return
        }

        contentItem.sendAsyncMessage(name, data)
    }

    property Component clipboardPasteDialogComponent: Component {
        Dialog {
            id: pasteDialog

            property string origin
            property int delay
            property bool delayElapsed: delay <= 0

            canAccept: delayElapsed

            function clipboardPasteReadText(origin) {
                //% "Allow %1 to read text from the clipboard?"
                return qsTrId("sailfish_browser-la-allow_clipboard_read").arg(origin)
            }

            function clipboardPasteReadUnknownText() {
                //% "Allow this page to read text from the clipboard?"
                return qsTrId("sailfish_browser-la-allow_clipboard_read_unknown")
            }

            Popups.UserPromptInterface {
                id: clipboardPastePrompt

                anchors.fill: parent

                //% "Allow"
                acceptText: qsTrId("sailfish_browser-he-allow_clipboard_read")
                //% "Deny"
                cancelText: qsTrId("sailfish_browser-he-deny_clipboard_read")

                Popups.UserPromptUi {
                    anchors.fill: parent
                    dialog: pasteDialog
                    popupInterface: clipboardPastePrompt

                    Column {
                        width: parent.width
                        spacing: Theme.paddingMedium

                        Label {
                            x: Theme.horizontalPageMargin
                            width: parent.width - 2 * x
                            text: pasteDialog.origin.length > 0
                                  ? pasteDialog.clipboardPasteReadText(pasteDialog.origin)
                                  : pasteDialog.clipboardPasteReadUnknownText()
                            wrapMode: Text.WordWrap
                            color: Theme.highlightColor
                        }
                    }
                }
            }

            Timer {
                interval: Math.max(0, pasteDialog.delay)
                running: pasteDialog.delay > 0
                onTriggered: pasteDialog.delayElapsed = true
            }
        }
    }

    function thumbnailCaptureSize() {
        if (webView.activePortalMode) {
            console.log("Thumbnail size tried accessed in captive portal mode")
            return Qt.size(0, 0)
        }

        var pageWidth = Math.min(browserPage.width, browserPage.height)
        var pageHeight = Math.max(browserPage.width, browserPage.height)
        var thumbnailWidth = pageWidth - Theme.horizontalPageMargin * 2
        var thumbnailHeight = Math.max(pageHeight / 2.5, pageWidth / 1.66)
                - (Theme.iconSizeSmall + Theme.paddingMedium * 2)

        var ratio = Math.min(pageWidth / thumbnailWidth,
                             pageHeight / thumbnailHeight)
        var width = thumbnailWidth * ratio
        var height = thumbnailHeight * ratio

        return Qt.size(width, height)
    }

    function grabActivePage() {
        if (webView.activePortalMode) {
            console.warn("Refusing page grab in active portal mode")
            return
        }

        if (webView.contentItem && webView.activeTabRendered) {
            if (webView.privateMode) {
                webView.contentItem.grabThumbnail(thumbnailCaptureSize())
            } else {
                webView.contentItem.grabToFile(thumbnailCaptureSize())
            }
        }
    }

    function handleKeyPress(key) {
        if (key == Qt.Key_F5) {
            reload()
        }
    }

    foreground: visibility >= QuickWindow.Window.Maximized && Qt.application.state === Qt.ApplicationActive
    readyToPaint: resourceController.videoActive ? webView.visible && !resourceController.displayOff
                                                 : webView.visible && webView.contentItem
                                                   && (webView.contentItem.domContentLoaded
                                                       || webView.contentItem.painted)

    touchBlocked: contentItem && contentItem.popupOpener && contentItem.popupOpener.active
                  || !AccessPolicy.browserEnabled || false

    onKeyPressed: handleKeyPress(key)

    onBackButtonPressed: webView.goBack()

    onForwardButtonPressed: webView.goForward()

    onTouched: {
        if (webView.contentItem && webView.contentItem.textSelectionActive) {
            clearSelection()
        }
    }

    webPageComponent: Component {
        WebPage {
            id: webPage

            property bool acceptedTouchIcon
            property int frameCounter
            property bool rendered
            readonly property bool textSelectionActive: textSelectionController && textSelectionController.active
            property Item textSelectionController: null
            readonly property bool activeWebPage: container.tabId == tabId
            property bool userHasDraggedWhileLoading
            property string favicon
            property string metadataTitle
            property string feedUrl
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

                    // Possible path that leads to a new tab. Thus, capturing current
                    // view before opening context menu.
                    if (!webView.activePortalMode) {
                        webView.grabActivePage()
                    }
                    contextMenuRequested(data)
                }

                onLoginSaved: {
                    if (!webView.activePortalMode) {
                        FaviconManager.grabIcon("logins", webPage,
                                                Qt.size(Theme.iconSizeMedium,
                                                        Theme.iconSizeMedium))
                    }
                }
            }

            function effectiveTitle() {
                return metadataTitle || title || String(url)
            }

            function updateHistoryIcon(force) {
                if (loaded && !webView.activePortalMode && !webView.privateMode) {
                    if (force) {
                        FaviconManager.refreshIcon("history", webPage,
                                                   Qt.size(Theme.iconSizeMedium,
                                                           Theme.iconSizeMedium))
                    } else {
                        FaviconManager.grabIcon("history", webPage,
                                                Qt.size(Theme.iconSizeMedium,
                                                        Theme.iconSizeMedium))
                    }
                }
            }

            signal selectionCopied(var data)
            signal contextMenuRequested(var data)

            function grabItem() {
                if (rendered && activeWebPage && active) {
                    if (webView.privateMode) {
                        grabThumbnail(thumbnailCaptureSize())
                    } else {
                        grabToFile(thumbnailCaptureSize())
                    }
                }
            }

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

                var page = window.pageStack.animatorPush(clipboardPasteDialogComponent, {
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

            fixedToolbar: fixedToolbarConfig.value
            toolbarHeight: container.toolbarHeight
            safeAreaTop: webView.displayCutoutAllowed ? webView._contentCutoutTop : 0
            safeAreaRight: webView.displayCutoutAllowed ? webView._contentCutoutRight : 0
            safeAreaBottom: webView.displayCutoutAllowed ? webView._contentCutoutBottom : 0
            safeAreaLeft: webView.displayCutoutAllowed ? webView._contentCutoutLeft : 0
            throttlePainting: !foreground && !resourceController.videoActive && webView.visible || !webView.visible
            enabled: webView.enabled
            chromeGestureThreshold: toolbarHeight / 3
            chromeGestureEnabled: !forcedChrome && enabled && !webView.imOpened && !fixedToolbar

            onFileGrabWritten: tabModel.updateThumbnailPath(tabId, fileName)

            // Image data is base64 encoded which can be directly used as source in Image element
            onThumbnailResult: tabModel.updateThumbnailPath(tabId, data)

            onTitleChanged: {
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

            onUrlChanged: {
                if (url == "about:blank") {
                    rendered = false
                    frameCounter = 0
                    webView.clearSurface()
                    return
                }

                webView.findInPageHasResult = false
                var modelUrl = tabModel.url(tabId)

                rendered = false
                frameCounter = 0

                // If url has changed or url doesn't exist in the model,
                // clear the thumbnail. Preserve the thumbnails in the model
                // if it has the same url (restarting browser / resurrecting a tab).
                if (!modelUrl || modelUrl != url) {
                    tabModel.updateThumbnailPath(tabId, "")
                }
            }

            onBackgroundColorChanged: {
                // Update only webPage
                if (container.contentItem === webPage) {
                    sendAsyncMessage("Browser:SelectionColorUpdate",
                                     {
                                         "color": Theme.secondaryHighlightColor
                                     })
                }
            }

            onDraggingChanged: {
                if (dragging && loading) {
                    userHasDraggedWhileLoading = true
                }
            }

            onLoadedChanged: {
                if (loaded) {
                    if (!userHasDraggedWhileLoading && resurrectedContentRect) {
                        sendAsyncMessage("embedui:zoomToRect",
                                         {
                                             "x": resurrectedContentRect.x, "y": resurrectedContentRect.y,
                                             "width": resurrectedContentRect.width, "height": resurrectedContentRect.height
                                         })
                        resurrectedContentRect = null
                    }

                    if (!webView.activePortalMode) {
                        grabItem()

                        if (!webView.privateMode) {
                            // Update the favicon for history items.
                            updateHistoryIcon(false)
                        }
                    }
                }

                // Refresh timers (if any) keep working even for suspended views. Hence
                // suspend the view again explicitly if browser content window is in not visible (background).
                if (loaded && !webView.visible) {
                    suspendView()
                }
            }

            onLoadingChanged: {
                if (loading) {
                    userHasDraggedWhileLoading = false
                    webPage.chrome = true
                    favicon = ""
                    metadataTitle = ""
                    feedUrl = ""
                    acceptedTouchIcon = false
                }
            }

            onAfterRendering: {
                // Try to capture something else than glClear color.
                if (frameCounter < 3) {
                    ++frameCounter
                } else if (!rendered) {
                    rendered = true
                    if (!webView.activePortalMode) {
                        grabItem()
                    }
                }
            }

            onRecvAsyncMessage: {
                if (pickerOpener.message(message, data) || popupOpener.message(message, data)) {
                    return
                }

                switch (message) {
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
                    if (favicon && favicon !== previousFavicon) {
                        updateHistoryIcon(true)
                    }
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
                        if (favicon !== oldFavicon) {
                            updateHistoryIcon(true)
                        }
                    }
                    break
                }
                case "Content:SelectionRange": {
                    if (textSelectionController === null) {
                        textSelectionController = textSelectionControllerComponent.createObject(browserPage,
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
                    linkHandler.handleLink(data.uri)
                    break
                }
                case "Link:AddFeed": {
                    if (!feedUrl && data.href) {
                        feedUrl = data.href
                    }
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
                addMessageListener("Content:SelectionRange")
                addMessageListener("Content:SelectionCopied")
                addMessageListener("Content:SelectionSwap")
                addMessageListener("embed:clipboardreadpaste")

                PermissionManager.instance()
            }
        }
    }
}
