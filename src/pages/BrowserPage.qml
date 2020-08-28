/****************************************************************************
**
** Copyright (C) 2013 - 2019 Jolla Ltd.
** Copyright (c) 2019 Open Mobile Platform LLC.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
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

Page {
    id: browserPage

    readonly property rect inputMask: inputMaskForOrientation(orientation)
    readonly property bool active: status == PageStatus.Active
    property bool tabPageActive
    readonly property size thumbnailSize: Qt.size((Screen.width - (largeScreen ? (2 * Theme.horizontalPageMargin) : 0)), (largeScreen ? Theme.itemSizeExtraLarge + (2 * Theme.paddingLarge) : Screen.height / 5))
    property Item debug
    property Component tabPageComponent

    property alias overlay: overlay
    property alias tabs: webView.tabModel
    property alias history: historyModel
    property alias viewLoading: webView.loading
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

    function activateNewTabView() {
        // Only open new tab if not blocked MDM, otherwise just bring to foreground
        if (AccessPolicy.browserEnabled) {
            pageStack.pop(browserPage, PageStackAction.Immediate)
            overlay.enterNewTabUrl(PageStackAction.Immediate)
        }
        bringToForeground(webView.chromeWindow)
        // after bringToForeground, webView has focus => activate chrome
        window.activate()
    }

    function inputMaskForOrientation(orientation) {
        // mask is in portrait window coordinates
        var mask = Qt.rect(0, 0, Screen.width, Screen.height)
        if (!window.opaqueBackground && webView.enabled && browserPage.active && !webView.touchBlocked && !downloadPopup.visible) {
            var overlayVisibleHeight = browserPage.height - overlay.y

            switch (orientation) {
            case Orientation.None:
            case Orientation.Portrait:
                mask.y = overlay.y
                // fallthrough
            case Orientation.PortraitInverted:
                mask.height = overlayVisibleHeight
                break

            case Orientation.LandscapeInverted:
                mask.x = overlay.y
                // fallthrough
            case Orientation.Landscape:
                mask.width = overlayVisibleHeight
            }
        }
        return mask
    }

    background: null

    onStatusChanged: {
        if (overlay.enteringNewTabUrl || webView.tabModel.count === 0) {
            return
        }

        if (status == PageStatus.Inactive && overlay.visible) {
            overlay.animator.hide()
        }
    }

    property int pageOrientation: pageStack.currentPage._windowOrientation
    onPageOrientationChanged: {
        // When on other pages update immediately.
        if (!active) {
            webView.applyContentOrientation(pageOrientation)
        }
    }

    orientationTransitions: orientationFader.orientationTransition

    Browser.OrientationFader {
        id: orientationFader

        visible: webView.contentItem
        page: browserPage
        fadeTarget: overlay.animator.allowContentUse ? overlay : overlay.dragArea
        color: webView.contentItem ? (webView.resourceController.videoActive &&
                                      webView.contentItem.fullscreen ? "black" : webView.contentItem.bgcolor)
                                   : "white"

        onApplyContentOrientation: webView.applyContentOrientation(browserPage.orientation)
    }

    HistoryModel {
        id: historyModel
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
                virtualKeyboardMargin: virtualKeyboardObserver.panelSize
            }
        }
    }

    Browser.DownloadRemorsePopup { id: downloadPopup }
    Browser.WebView {
        id: webView

        enabled: overlay.animator.allowContentUse
        fullscreenHeight: portrait ? Screen.height : Screen.width
        portrait: browserPage.isPortrait
        maxLiveTabCount: 3
        toolbarHeight: overlay.toolBar.toolsHeight
        rotationHandler: browserPage
        imOpened: virtualKeyboardObserver.opened
        canShowSelectionMarkers: !orientationFader.waitForWebContentOrientationChanged

        // Show overlay immediately at top if needed.
        onTabModelChanged: handleModelChanges(true)

        onChromeExposed: {
            if (overlay.animator.atTop && overlay.searchField.focus && !WebUtils.firstUseDone) {
                webView.chromeWindow.raise()
            }
        }

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

        // Both model change and model count change are connected to this.
        function handleModelChanges(openOverlayImmediately) {
            if (webView.completed && (!webView.tabModel || webView.tabModel.count === 0)) {
                overlay.animator.showOverlay(openOverlayImmediately)
            }

            window.setBrowserCover(webView.tabModel)
        }
    }

    // Use Connections so that target updates when model changes.
    Connections {
        target: AccessPolicy.browserEnabled && webView && webView.tabModel || null
        ignoreUnknownSignals: true
        // Animate overlay to top if needed.
        onCountChanged: webView.handleModelChanges(false)
        onWaitingForNewTabChanged: window.opaqueBackground = webView.tabModel.waitingForNewTab
    }

    InputRegion {
        window: webView.chromeWindow
        x: inputMask.x
        y: inputMask.y
        width: inputMask.width
        height: inputMask.height
    }

    Browser.DimmerEffect {
        id: contentDimmer

        readonly property bool canOpenContentDimmer: webView.activeTabRendered && overlay.animator.atBottom

        width: browserPage.width
        height: Math.ceil(overlay.y)

        baseColor: overlay.baseColor
        baseOpacity: overlay.baseOpacity
        dimmerOpacity: overlay.animator.atBottom
                       ? 0.0
                       : 0.9 - (overlay.y / (webView.fullscreenHeight - overlay.toolBar.toolsHeight)) * 0.9

        MouseArea {
            anchors.fill: parent
            enabled: overlay.animator.atTop && webView.tabModel.count > 0
            onClicked: overlay.dismiss(true)
        }

        Browser.PrivateModeTexture {
            id: privateModeTexture
            anchors.fill: contentDimmer
            visible: webView.privateMode && !overlay.animator.allowContentUse
        }

        onCanOpenContentDimmerChanged: {
            if (canOpenContentDimmer) {
                webView.tabModel.waitingForNewTab = false
                window.opaqueBackground = false
            }
        }
    }

    Label {
        x: (contentDimmer.width - implicitWidth) / 2
        // Allow only half of the width
        width: parent.width / 2
        truncationMode: TruncationMode.Fade
        opacity: privateModeTexture.visible ? 1.0 : 0.0
        anchors {
            bottom: contentDimmer.bottom
            bottomMargin: (overlay.toolBar.toolsHeight - height) / 2
        }

        //: Label for private browsing above address bar
        //% "Private browsing"
        text: qsTrId("sailfish_browser-la-private_mode")
        color: Theme.highlightColor
        font.pixelSize: Theme.fontSizeLarge

        Behavior on opacity { FadeAnimation {} }
    }

    Browser.Overlay {
        id: overlay

        active: browserPage.status == PageStatus.Active
        webView: webView
        historyModel: historyModel
        browserPage: browserPage

        onEnteringNewTabUrlChanged: window.opaqueBackground = webView.tabModel.waitingForNewTab || enteringNewTabUrl

        animator.onAtBottomChanged: {
            if (!animator.atBottom) {
                webView.clearSelection()
            }
        }

        onActiveChanged: {
            if (active && webView.contentItem && !overlay.enteringNewTabUrl && !webView.contentItem.fullscreen) {
                overlay.animator.showChrome()
            }

            if (!active) {
                webView.clearSelection()
                if (webView.chromeWindow && webView.foreground) {
                    webView.chromeWindow.raise()
                }
            }
        }
    }

    CoverActionList {
        enabled: (browserPage.status === PageStatus.Active || browserPage.tabPageActive || !webView.tabModel || webView.tabModel.count === 0)
        iconBackground: true
        window: webView

        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: activateNewTabView()
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

            // Url is empty when user tapped icon when browser was already open.
            // In case first use not done show the overlay immediately.
            if (url == "") {
                bringToForeground(webView.chromeWindow)
                if (!WebUtils.firstUseDone) {
                    overlay.enterNewTabUrl(PageStackAction.Immediate)
                }

                return
            }

            if (browserPage.status !== PageStatus.Active) {
                pageStack.pop(browserPage, PageStackAction.Immediate)
            }

            webView.grabActivePage()
            if (!webView.tabModel.activateTab(url)) {
                webView.clearSelection()
                webView.tabModel.newTab(url)
                overlay.dismiss(true, !Qt.application.active /* immadiate */)
            }
            bringToForeground(webView.chromeWindow)
            window.activate()
        }
        onActivateNewTabViewRequested: activateNewTabView()
        onShowChrome: {
            pageStack.pop(browserPage, PageStackAction.Immediate)
            overlay.dismiss(true, !Qt.application.active /* immadiate */)
            bringToForeground(webView.chromeWindow)
            window.activate()
        }
    }

    Component.onCompleted: {
        if (!WebUtils.firstUseDone) {
            window.setBrowserCover(webView.tabModel)
        }

        if (Qt.application.arguments.indexOf("-debugMode") > 0) {
            var component = Qt.createComponent(Qt.resolvedUrl("components/DebugOverlay.qml"))
            if (component.status === Component.Ready) {
                debug = component.createObject(browserPage)
            } else {
                console.warn("Failed to create DebugOverlay " + component.errorString())
            }
        }
    }
}
