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

    readonly property bool active: status == PageStatus.Active

    property alias overlay: overlay
    property alias url: webView.url
    property alias title: webView.title
    property alias webView: webView
    property alias inputRegion: inputRegion

    function load(url, title) {
        webView.load(url, title)
    }

    function bringToForeground(window) {
        if ((webView.chromeWindow.visibility < QuickWindow.Window.Maximized) && window) {
            window.raise()
        }
    }

    property int pageOrientation: pageStack.currentPage._windowOrientation
    onPageOrientationChanged: {
        // When on other pages update immediately.
        if (!active) {
            webView.applyContentOrientation(pageOrientation)
        }
    }
    onOrientationChanged: webView.applyContentOrientation(orientation)

    orientationTransitions: orientationFader.orientationTransition

    cutoutMode: CutoutMode.FullScreen
    background: null

    Keys.onPressed: {
        webView.handleKeyPress(event.key)
    }

    Shared.OrientationFader {
        id: orientationFader

        immediate: true
        visible: webView.contentItem
        page: browserPage
        fadeTarget: overlay
        color: webView.contentItem ? (webView.resourceController.videoActive &&
                                      webView.contentItem.fullscreen ? "black" : webView.contentItem.backgroundColor)
                                   : "white"
    }

    Private.VirtualKeyboardObserver {
        id: virtualKeyboardObserver

        active: webView.enabled
        transpose: window._transpose
        orientation: browserPage.orientation


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

        activePortalMode: true
        contentItem: portalView
        enabled: overlay.animator.allowContentUse
        fullscreenHeight: portrait ? Screen.height : Screen.width
        portrait: browserPage.isPortrait
        toolbarHeight: overlay.animator.opened ? overlay.toolBar.rowHeight : 0
        rotationHandler: browserPage
        imOpened: virtualKeyboardObserver.opened
        canShowSelectionMarkers: false

        onForegroundChanged: {
            if (foreground && webView.chromeWindow) {
                webView.chromeWindow.raise()
            }
        }




        function applyContentOrientation(orientation) {
            reportWindowOrientation(_qtScreenOrientation(orientation))
        }
    }

    Shared.CaptivePortalView {
        id: portalView

        webView: webView
        anchors {
            fill: parent
            topMargin: webView.displayCutoutAllowed ? 0 : webView._contentCutoutTop
            rightMargin: webView.displayCutoutAllowed ? 0 : webView._contentCutoutRight
            bottomMargin: webView.displayCutoutAllowed ? 0 : webView._contentCutoutBottom
            leftMargin: webView.displayCutoutAllowed ? 0 : webView._contentCutoutLeft
        }
        onTouched: if (fullscreen) fullscreenCloseVisibleTimer.restart()
    }

    IconButton {
        id: fullscreenClose

        opacity: fullscreenCloseVisibleTimer.running || pressed ? 1.0 : 0.0
        Behavior on opacity { FadeAnimation {} }
        visible: opacity > 0
        x: Theme.paddingLarge
        y: Theme.paddingLarge
        icon.source: "image://theme/icon-m-close"
        onClicked: {
            webView.sendAsyncMessage("embedui:exitFullscreen", {})
        }

        Timer {
            id: fullscreenCloseVisibleTimer

            interval: 2000
            running: webView.contentFullscreen
        }
    }

    // Use Connections so that target updates when model changes.
    Connections {
        target: AccessPolicy.browserEnabled && webView && webView.tabModel || null
        ignoreUnknownSignals: true
    }

    InputRegion {
        id: inputRegion

        window: webView.chromeWindow
        orientation: browserPage.orientation // Qt and Silica orientations match
        overlayMask: Qt.rect(0, 0, browserPage.width, browserPage.height)
        closeButtonMask: fullscreenClose.visible ? Qt.rect(fullscreenClose.x, fullscreenClose.y,
                                                           fullscreenClose.width, fullscreenClose.height)
                                                 : Qt.rect(0, 0, 0, 0)
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
                webView.tabModel.newTab(url, false)
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
