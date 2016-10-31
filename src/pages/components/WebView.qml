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
import QtQuick.Window 2.1 as QtQuick
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import Sailfish.WebView.Pickers 1.0 as Pickers
import Sailfish.WebView.Popups 1.0 as Popups
import Qt5Mozilla 1.0
import "." as Browser

WebContainer {
    id: webView

    property color _decoratorColor: Theme.highlightDimmerColor
    readonly property bool moving: contentItem ? contentItem.moving : false
    property bool findInPageHasResult

    property var resourceController: ResourceController {
        webPage: contentItem
        background: !webView.visible
    }

    property Timer auxTimer: Timer {
        interval: 1000
    }

    property var _webPageCreator: WebPageCreator {
        activeWebPage: contentItem
        // onNewWindowRequested is always handled as synchronous operation (not through newTab).
        onNewWindowRequested: tabModel.newTab(url, "", parentId)
    }

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

    function thumbnailCaptureSize() {
        var ratio = Screen.width / browserPage.thumbnailSize.width
        var height = browserPage.thumbnailSize.height * ratio

        return Qt.size(Screen.width, height)
    }

    function grabActivePage() {
        if (webView.contentItem && webView.activeTabRendered) {
            if (webView.privateMode) {
                webView.contentItem.grabThumbnail(thumbnailCaptureSize())
            } else {
                webView.contentItem.grabToFile(thumbnailCaptureSize())
            }
        }
    }

    foreground: Qt.application.active
    readyToPaint: resourceController.videoActive ? webView.visible && !resourceController.displayOff : webView.visible && webView.contentItem && webView.contentItem.domContentLoaded
    allowHiding: !resourceController.videoActive && !resourceController.audioActive
    fullscreenMode: (contentItem && contentItem.chromeGestureEnabled && !contentItem.chrome) ||
                    (contentItem && contentItem.fullscreen)

    popupActive: contentItem && contentItem.popupOpener && contentItem.popupOpener.active || false
    favicon: contentItem ? contentItem.favicon : ""

    webPageComponent: Component {
        WebPage {
            id: webPage

            property int iconSize
            property string iconType
            property int frameCounter
            property bool rendered
            readonly property bool activeWebPage: container.tabId == tabId

            property QtObject pickerOpener: Pickers.PickerOpener {
                pageStack: window.pageStack
                contentItem: webPage
            }

            property QtObject popupOpener: Popups.PopupOpener {
                pageStack: pickerOpener.pageStack
                parentItem: browserPage
                contentItem: webPage
                // ContextMenu needs a reference to correct TabModel so that
                // private and public tabs are created to correct model. While context
                // menu is open, tab model cannot change (at least at the moment).
                tabModel: webView.tabModel

                onAboutToOpenContextMenu: {
                    // Possible path that leads to a new tab. Thus, capturing current
                    // view before opening context menu.
                    webView.grabActivePage()
                    contextMenuRequested(data)
                }
            }

            signal selectionRangeUpdated(variant data)
            signal selectionCopied(variant data)
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

            fullscreenHeight: container.fullscreenHeight
            toolbarHeight: container.toolbarHeight
            throttlePainting: !foreground && !resourceController.videoActive && webView.visible || !webView.visible
            enabled: webView.enabled

            // There needs to be enough content for enabling chrome gesture
            chromeGestureThreshold: toolbarHeight / 2
            chromeGestureEnabled: !forcedChrome && enabled && !webView.imOpened

            onGrabResult: tabModel.updateThumbnailPath(tabId, fileName)

            // Image data is base64 encoded which can be directly used as source in Image element
            onThumbnailResult: tabModel.updateThumbnailPath(tabId, data)

            onAtYBeginningChanged: {
                if (atYBeginning && activeWebPage && domContentLoaded) {
                    chrome = true
                }
            }

            onAtYEndChanged: {
                // Don't hide chrome if content lenght is short e.i. forcedChrome is enabled.
                if (atYEnd && !forcedChrome && chrome && activeWebPage && domContentLoaded) {
                    chrome = false
                }
            }

            onUrlChanged: {
                if (url == "about:blank") return

                webView.findInPageHasResult = false
                var modelUrl = tabModel.url(tabId)

                rendered = false
                frameCounter = 0

                // If url has changed or url doesn't exists in the model,
                // clear the thumbnail. Preserve the thumbnails in the model
                // if it has the same url (restarting browser / resurrecting a tab).
                if (!modelUrl || modelUrl != url) {
                    tabModel.updateThumbnailPath(tabId, "")
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
                if (loaded) {
                    if (!userHasDraggedWhileLoading && resurrectedContentRect) {
                        sendAsyncMessage("embedui:zoomToRect",
                                         {
                                             "x": resurrectedContentRect.x, "y": resurrectedContentRect.y,
                                             "width": resurrectedContentRect.width, "height": resurrectedContentRect.height
                                         })
                        resurrectedContentRect = null
                    }
                    grabItem()
                }

                // Refresh timers (if any) keep working even for suspended views. Hence
                // suspend the view again explicitly if browser content window is in not visible (background).
                if (loaded && !webView.visible) {
                    suspendView();
                }
            }

            onLoadingChanged: {
                if (loading) {
                    userHasDraggedWhileLoading = false
                    webPage.chrome = true
                    favicon = ""
                    iconType = ""
                    iconSize = 0
                }
            }

            onAfterRendering: {
                // Try to capture something else than glClear color.
                if (frameCounter < 3) {
                    ++frameCounter
                } else if (!rendered) {
                    rendered = true
                    grabItem()
                }
            }

            onRecvAsyncMessage: {
                if (pickerOpener.handlesMessage(message) || popupOpener.handlesMessage(message)) {
                    return
                }

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
                        }

                        for (var i in sizes) {
                            var faviconSize = parseInt(sizes[i])
                            // Accept largest icon but one that is still smaller than Theme.itemSizeExtraLarge.
                            if (faviconSize && faviconSize > iconSize && faviconSize <= Theme.itemSizeExtraLarge * 2) {
                                iconSize = faviconSize
                                parsedFavicon = true
                            }
                        }
                    }

                    // Always pick at least one touch icon, parsed favicons are always based on size (good enough quality)
                    if (!acceptedTouchIcon && acceptableTouchIcon || parsedFavicon) {
                        favicon = data.href
                        iconType = iconSize >= Theme.iconSizeMedium ? data.rel : ""
                    }
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
}
