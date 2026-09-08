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

    anchors.fill: parent
    property QtObject contentItem

    property bool activePortalMode
    readonly property bool moving: contentItem && contentItem.moving
    property bool portrait: true
    property bool contentFullscreen: contentItem && contentItem.fullscreen
    property QtObject chromeContentItem: contentItem
    property bool needChrome: !chromeContentItem
                              || (chromeContentItem.chrome && !chromeContentItem.fullscreen)
    property real fullscreenHeight
    property bool imOpened
    property real toolbarHeight
    property string favicon: contentItem && contentItem.favicon ? contentItem.favicon : ""
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
    readonly property int _contentOrientation: _screenOrientation
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
        background: !webView.applicationVisible
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

    function handleKeyPress(key) {
        if (key == Qt.Key_F5) {
            reload()
        }
    }

    readonly property bool applicationVisible: chromeWindow && chromeWindow.visible
    foreground: (chromeWindow ? chromeWindow.visibility : 0)
                >= QuickWindow.Window.Maximized && Qt.application.state === Qt.ApplicationActive
    touchBlocked: contentItem && contentItem.popupOpener && contentItem.popupOpener.active
                  || !AccessPolicy.browserEnabled || false

}
