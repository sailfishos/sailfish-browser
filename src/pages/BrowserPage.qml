/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


import QtQuick 2.1
import QtQuick.Window 2.1 as QtWindow
import Sailfish.Silica 1.0
import Sailfish.Silica.private 1.0 as Private
import Sailfish.Browser 1.0
import "components" as Browser

Page {
    id: browserPage

    readonly property rect inputMask: inputMaskForOrientation(orientation)
    readonly property bool active: status == PageStatus.Active
    readonly property bool largeScreen: Screen.sizeCategory > Screen.Medium
    readonly property size thumbnailSize: Qt.size((Screen.width - (largeScreen ? (2 * Theme.horizontalPageMargin) : 0)), (largeScreen ? Theme.itemSizeExtraLarge + (2 * Theme.paddingLarge) : Screen.height / 5))
    property Item debug
    property Component tabPageComponent

    property alias tabs: webView.tabModel
    property alias history: historyModel
    property alias viewLoading: webView.loading
    property alias url: webView.url
    property alias title: webView.title
    property alias webView: webView

    function load(url, title) {
        webView.load(url, title)
    }

    function bringToForeground() {
        if (!Qt.application.active) {
            webView.raise()
        }
    }

    function activateNewTabView() {
        pageStack.pop(browserPage, PageStackAction.Immediate);
        overlay.enterNewTabUrl(PageStackAction.Immediate)
        bringToForeground()
        // after bringToForeground, webView has focus => activate chrome
        window.activate()
    }

    function inputMaskForOrientation(orientation) {
        // mask is in portrait window coordinates
        var mask = Qt.rect(0, 0, Screen.width, Screen.height)
        if (webView.enabled && browserPage.active && !webView.popupActive) {
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

    onStatusChanged: {
        if (overlay.enteringNewTabUrl) {
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
        onClosedChanged: {
            if (closed) {
                webView.updatePageFocus(false)
            }
        }

        // Update content height only after virtual keyboard fully opened.
        states: State {
            name: "boundHeightControl"
            when: virtualKeyboardObserver.opened && webView.enabled
            PropertyChanges {
                target: webView.contentItem
                height: browserPage.height - virtualKeyboardObserver.panelSize
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
        width: window.width
        height: window.height
        rotationHandler: browserPage
        imOpened: virtualKeyboardObserver.opened

        tabModel.onCountChanged: window.opaqueBackground = tabModel.count == 0

        function applyContentOrientation(orientation) {
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
            resetHeight()
        }
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
        width: browserPage.width
        height: Math.ceil(overlay.y)

        baseColor: overlay.baseColor
        baseOpacity: overlay.baseOpacity
        dimmerOpacity: 0.9 - (overlay.y / (webView.fullscreenHeight - overlay.toolBar.toolsHeight)) * 0.9

        MouseArea {
            anchors.fill: parent
            enabled: overlay.animator.atTop && webView.tabModel.count > 0
            onClicked: overlay.dismiss()
        }

        Browser.PrivateModeTexture {
            id: privateModeTexture
            anchors.fill: contentDimmer
            visible: webView.privateMode && !overlay.animator.allowContentUse
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

        onEnteringNewTabUrlChanged: window.opaqueBackground = overlay.enteringNewTabUrl
    }

    onActiveChanged: {
        if (active && webView.contentItem && !overlay.enteringNewTabUrl && !webView.contentItem.fullscreen) {
            overlay.animator.showChrome()
        }
    }

    CoverActionList {
        enabled: (browserPage.status === PageStatus.Active || !webView.tabModel || webView.tabModel.count === 0)
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
            // Url is empty when user tapped icon when browser was already open.
            // In case first use not done show the overlay immediately.
            if (url == "") {
                bringToForeground()
                if (!WebUtils.firstUseDone) {
                    overlay.animator.showOverlay(true)
                }

                return
            }

            if (browserPage.status !== PageStatus.Active) {
                pageStack.pop(browserPage, PageStackAction.Immediate)
            }

            webView.grabActivePage()
            if (!webView.tabModel.activateTab(url)) {
                // Not found in tabs list, load it. A new tab will be created if no tabs exist.
                webView.load(url)
                overlay.animator.showChrome(true)
            }
            bringToForeground()
        }
        onActivateNewTabViewRequested: {
            activateNewTabView()
        }
    }

    Component.onCompleted: {
        WebUtils.silicaPixelRatio = Theme.pixelRatio
        WebUtils.touchSideRadius = Theme.paddingMedium + Theme.paddingLarge
        WebUtils.touchTopRadius = Theme.paddingLarge * 2
        WebUtils.touchBottomRadius = Theme.paddingMedium + Theme.paddingSmall
        WebUtils.inputItemSize = Theme.fontSizeSmall
        WebUtils.zoomMargin = Theme.paddingMedium

        if (!WebUtils.firstUseDone) {
            window.setBrowserCover(webView.tabModel)
        }

        if (WebUtils.debugMode) {
            component = Qt.createComponent(Qt.resolvedUrl("components/DebugOverlay.qml"))
            if (component.status === Component.Ready) {
                debug = component.createObject(browserPage)
            } else {
                console.warn("Failed to create DebugOverlay " + component.errorString())
            }
        }
    }

    Browser.BrowserNotification {
        id: notification
    }
}
