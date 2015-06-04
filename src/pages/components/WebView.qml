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
import "WebPopupHandler.js" as PopupHandler
import "." as Browser

WebContainer {
    id: webView

    property color _decoratorColor: Theme.highlightDimmerColor
    readonly property bool moving: contentItem ? contentItem.moving : false

    property bool findInPageHasResult

    function stop() {
        if (contentItem) {
            contentItem.stop()
        }
    }

    function sendAsyncMessage(name, data) {
        if (!contentItem) {
            return
        }

        contentItem.sendAsyncMessage(name, data)
    }

    function grabActivePage() {
        if (webView.contentItem) {
            webView.privateMode ? webView.contentItem.grabThumbnail() : webView.contentItem.grabToFile()
        }
    }

    foreground: Qt.application.active
    allowHiding: !resourceController.videoActive && !resourceController.audioActive
    fullscreenMode: (contentItem && contentItem.chromeGestureEnabled && !contentItem.chrome) ||
                    (contentItem && contentItem.fullscreen)

    favicon: contentItem ? contentItem.favicon : ""

    visible: WebUtils.firstUseDone

//    property var foobar: WebViewCreator {
//        activeWebView: contentItem
//        // onNewWindowRequested is always handled as synchronous operation (not through newTab).
//        onNewWindowRequested: tabModel.newTab(url, "", parentId)
//    }

    webPageComponent: Component {
        WebPage {
            id: webPage

            property int iconSize
            property string iconType
            readonly property bool activeWebPage: container.tabId == tabId

            fullscreenHeight: container.fullscreenHeight
            toolbarHeight: container.toolbarHeight

            enabled: webView.enabled

            // There needs to be enough content for enabling chrome gesture
            chromeGestureThreshold: toolbarHeight / 2
            chromeGestureEnabled: (contentHeight > fullscreenHeight + toolbarHeight) && !forcedChrome && enabled && !webView.imOpened

            signal selectionRangeUpdated(variant data)
            signal selectionCopied(variant data)
            signal contextMenuRequested(variant data)

            width: container.rotationHandler && container.rotationHandler.width || 0

            onClearGrabResult: tabModel.updateThumbnailPath(tabId, "")
            onGrabResult: tabModel.updateThumbnailPath(tabId, fileName)

            // Image data is base64 encoded which can be directly used as source in Image element
            onThumbnailResult: tabModel.updateThumbnailPath(tabId, data)

            onUrlChanged: {
                if (url == "about:blank") return

                webView.findInPageHasResult = false

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

            onDraggingChanged: {
                if (dragging && loading) {
                    userHasDraggedWhileLoading = true
                }
            }

            onLoadedChanged: {
                if (loaded && !userHasDraggedWhileLoading) {
                    resetHeight(false)
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
                // suspend the view again explicitly if browser content window is in not visible (background).
                if (loaded && !webView.visible) {
                    suspendView();
                }

                if (loaded) {
                    webView.privateMode ? grabThumbnail() : grabToFile()
                }
            }

            onLoadingChanged: {
                if (loading) {
                    userHasDraggedWhileLoading = false
                    webPage.chrome = true
                    favicon = ""
                    iconType = ""
                    iconSize = 0

                    resetHeight(false)
                }
            }

            onRecvAsyncMessage: {
                switch (message) {
                case "chrome:linkadded": {
                    var parsedFavicon = false
                    var acceptedTouchIcon = (iconType === "apple-touch-icon" || iconType === "apple-touch-icon-precomposed")
                    var acceptableTouchIcon = (data.rel === "apple-touch-icon" || data.rel === "apple-touch-icon-precomposed")
                    if (data.href && (data.rel === "icon" || acceptableTouchIcon)) {
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
                            // Accept largest icon but one that is still smaller than Theme.itemSizeExtraLarge.
                            if (faviconSize && faviconSize > iconSize && faviconSize <= Theme.itemSizeExtraLarge) {
                                iconSize = faviconSize
                                parsedFavicon = true
                            }
                        }
                    }

                    if (!acceptedTouchIcon && (data.rel === "shortcut icon" || acceptableTouchIcon || parsedFavicon)) {
                        favicon = data.href
                        iconType = iconSize >= Theme.iconSizeMedium ? data.rel : ""
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
                case "embed:find": {
                    // Found, or found wrapped
                    if( data.r == 0 || data.r == 2) {
                        webView.findInPageHasResult = true
                    } else {
                        webView.findInPageHasResult = false
                    }
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
        }
    }

//    Rectangle {
//        id: verticalScrollDecorator

//        width: 5
//        height: contentItem ? contentItem.verticalScrollDecorator.size : 0
//        y: contentItem ? contentItem.verticalScrollDecorator.position : 0
//        z: 1
//        anchors.right: contentItem ? contentItem.right: undefined
//        color: _decoratorColor
//        smooth: true
//        radius: 2.5
//        visible: contentItem && contentItem.contentHeight > contentItem.height && !contentItem.pinching && !popupActive
//        opacity: contentItem && contentItem.verticalScrollDecorator.moving ? 1.0 : 0.0
//        Behavior on opacity { NumberAnimation { properties: "opacity"; duration: 400 } }
//    }

//    Rectangle {
//        id: horizontalScrollDecorator

//        width: contentItem ? contentItem.horizontalScrollDecorator.size : 0
//        height: 5
//        x: contentItem ? contentItem.horizontalScrollDecorator.position : 0
//        y: webView.height - height
//        z: 1
//        color: _decoratorColor
//        smooth: true
//        radius: 2.5
//        visible: contentItem && contentItem.contentWidth > contentItem.width && !contentItem.pinching && !popupActive
//        opacity: contentItem && contentItem.horizontalScrollDecorator.moving ? 1.0 : 0.0
//        Behavior on opacity { NumberAnimation { properties: "opacity"; duration: 400 } }
//    }

    property var resourceController: ResourceController {
        webView: contentItem
        background: !webView.visible
    }

    property Timer auxTimer: Timer {
        interval: 1000
    }

    property Component pickerCreator: Component {
        PickerCreator {}
    }

    Component.onCompleted: {
        PopupHandler.auxTimer = auxTimer
        PopupHandler.pageStack = pageStack
        PopupHandler.webView = webView
        PopupHandler.browserPage = browserPage
        PopupHandler.resourceController = resourceController
        PopupHandler.WebUtils = WebUtils
        PopupHandler.tabModel = tabModel
        PopupHandler.pickerCreator = pickerCreator
    }
}
