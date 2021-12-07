/****************************************************************************
**
** Copyright (c) 2020 - 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


import QtQuick 2.2
import QtQuick.Window 2.2 as QuickWindow
import Sailfish.Silica 1.0
import Sailfish.Silica.private 1.0 as Private
import Sailfish.Browser 1.0
import Sailfish.Policy 1.0
import "components" as Browser
import "../shared" as Shared

Page {
    id: browserPage

    readonly property rect inputMask: inputMaskForOrientation(orientation)
    readonly property bool active: status == PageStatus.Active

    property alias overlay: overlay
    property alias url: webView.url
    property alias title: webView.title
    property alias webView: webView

    function load(url, title) {
        webView.load(url, title)
    }

    function bringToForeground(window) {
        if ((webView.visibility < QuickWindow.Window.Maximized) && window) {
            window.raise()
        }
    }

    function inputMaskForOrientation(orientation) {
        // mask is in portrait window coordinates
        var portraitScreen = window.QuickWindow.Screen.primaryOrientation === Qt.PortraitOrientation
        var mask = Qt.rect(0, 0,
                           portraitScreen ? Screen.width : Screen.height,
                           portraitScreen ? Screen.height : Screen.width)
        if (webView.enabled && browserPage.active && !webView.touchBlocked) {
            var overlayVisibleHeight = browserPage.height - overlay.y

            switch (window.QuickWindow.Screen.angleBetween(orientation, window.QuickWindow.Screen.primaryOrientation)) {
            case 0:
            case 360:
                mask.y = overlay.y
                // fallthrough
            case 180:
            case -180:
                mask.height = overlayVisibleHeight
                break
            case 270:
            case -90:
                mask.x = overlay.y
                // fallthrough
            case 90:
            case -270:
                mask.width = overlayVisibleHeight
            }
        }
        return mask
    }

    property int pageOrientation: pageStack.currentPage._windowOrientation
    onPageOrientationChanged: {
        // When on other pages update immediately.
        if (!active) {
            webView.applyContentOrientation(pageOrientation)
        }
    }

    orientationTransitions: orientationFader.orientationTransition

    background: null

    Shared.OrientationFader {
        id: orientationFader

        visible: webView.contentItem
        page: browserPage
        fadeTarget: overlay
        color: webView.contentItem ? (webView.resourceController.videoActive &&
                                      webView.contentItem.fullscreen ? "black" : webView.contentItem.backgroundColor)
                                   : "white"

        onApplyContentOrientation: webView.applyContentOrientation(browserPage.orientation)
    }

    Private.VirtualKeyboardObserver {
        id: virtualKeyboardObserver

        active: webView.enabled
        transpose: window._transpose
        orientation: browserPage.orientation

        onWindowChanged: webView.chromeWindow = window

        // Update content height only after virtual keyboard fully opened.
        states: State {
            name: "boundHeightControl"
            when: virtualKeyboardObserver.opened && webView.enabled
            PropertyChanges {
                target: webView.contentItem
                virtualKeyboardHeight: virtualKeyboardObserver.imSize
            }
        }
    }

    Shared.WebView {
        id: webView

        enabled: overlay.animator.allowContentUse
        fullscreenHeight: portrait ? Screen.height : Screen.width
        portrait: browserPage.isPortrait
        maxLiveTabCount: 3
        toolbarHeight: overlay.animator.opened ? overlay.toolBar.rowHeight : 0
        rotationHandler: browserPage
        imOpened: virtualKeyboardObserver.opened
        canShowSelectionMarkers: false

        onForegroundChanged: {
            if (foreground && webView.chromeWindow) {
                webView.chromeWindow.raise()
            }
        }

        onWebContentOrientationChanged: orientationFader.waitForWebContentOrientationChanged = false

        function applyContentOrientation(orientation) {
            orientationFader.waitForWebContentOrientationChanged = (contentItem && contentItem.active)

            switch (orientation) {
            case Orientation.None:
            case Orientation.Portrait:
                updateContentOrientation(Qt.PortraitOrientation)
                break
            case Orientation.Landscape:
                updateContentOrientation(Qt.LandscapeOrientation)
                break
            case Orientation.PortraitInverted:
                updateContentOrientation(Qt.InvertedPortraitOrientation)
                break
            case Orientation.LandscapeInverted:
                updateContentOrientation(Qt.InvertedLandscapeOrientation)
                break
            }
        }
    }

    // Use Connections so that target updates when model changes.
    Connections {
        target: AccessPolicy.browserEnabled && webView && webView.tabModel || null
        ignoreUnknownSignals: true
    }

    InputRegion {
        window: webView.chromeWindow
        x: inputMask.x
        y: inputMask.y
        width: inputMask.width
        height: inputMask.height
    }

    Browser.CaptivePortalOverlay {
        id: overlay

        active: true
        webView: webView
        containerPage: browserPage

        animator.onAtBottomChanged: {
            if (!animator.atBottom) {
                webView.clearSelection()
            }
        }

        onActiveChanged: {
            if (active && webView.contentItem) {
                overlay.animator.showChrome()
            }

            if (!active) {
                if (webView.chromeWindow && webView.foreground) {
                    webView.chromeWindow.raise()
                }
            }
        }
    }

    Connections {
        target: WebUtils
        onOpenUrlRequested: {
            // Refuse if blocked by MDM
            if (!AccessPolicy.browserEnabled) {
                bringToForeground(webView.chromeWindow)
                window.activate()
                return
            }

            if (!webView.tabModel.activateTab(url)) {
                webView.clearSelection()
                webView.tabModel.newTab(url)
                overlay.dismiss(!Qt.application.active /* immadiate */)
            }
            bringToForeground(webView.chromeWindow)
            window.activate()
        }
        onShowChrome: {
            overlay.dismiss(!Qt.application.active /* immadiate */)
            bringToForeground(webView.chromeWindow)
            window.activate()
        }
    }
}
