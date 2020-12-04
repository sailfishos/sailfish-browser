/*
 * Copyright (c) 2014 - 2019 Jolla Ltd.
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.2
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import "." as Browser
import "../../shared" as Shared

Column {
    id: toolBarRow

    property string url
    property string findText
    property real secondaryToolsHeight
    property bool secondaryToolsActive
    property bool findInPageActive
    property real certOverlayHeight
    property bool certOverlayActive
    property real certOverlayAnimPos
    property real certOverlayPreferedHeight: 4 * toolBarRow.height
    readonly property bool showFindButtons: webView.findInPageHasResult && findInPageActive
    property alias bookmarked: secondaryBar.bookmarked
    readonly property alias rowHeight: toolsRow.height
    readonly property int maxRowCount: 5

    readonly property int horizontalOffset: largeScreen ? Theme.paddingLarge : Theme.paddingSmall
    readonly property int buttonPadding: largeScreen || orientation === Orientation.Landscape || orientation === Orientation.LandscapeInverted
                                         ? Theme.paddingMedium : Theme.paddingSmall
    readonly property int iconWidth: largeScreen ? (Theme.iconSizeLarge + 3 * buttonPadding) : (Theme.iconSizeMedium + 2 * buttonPadding)
    readonly property int smallIconWidth: largeScreen ? (Theme.iconSizeMedium + 3 * buttonPadding) : (Theme.iconSizeSmall + 2 * buttonPadding)
    // Height of toolbar should be such that viewport height is
    // even number both chrome and fullscreen modes. For instance
    // height of 110px for toolbar would result 1px rounding
    // error in chrome mode as viewport height would be 850px. This would
    // result in CSS pixels viewport height of 566.66..px -> rounded to 566px.
    // So, we make sure below that (device height - toolbar height) / pixel ratio is even number.
    // target values when Theme.pixelratio == 1 are:
    // portrait: 108px
    // landcape: 78px
    property int scaledPortraitHeight: Screen.height -
                                       Math.floor((Screen.height - Settings.toolbarLarge * Theme.pixelRatio) /
                                                  WebUtils.cssPixelRatio) * WebUtils.cssPixelRatio
    property int scaledLandscapeHeight: Screen.width -
                                        Math.floor((Screen.width - Settings.toolbarSmall * Theme.pixelRatio) /
                                                   WebUtils.cssPixelRatio) * WebUtils.cssPixelRatio


    signal showTabs
    signal showOverlay
    signal showSecondaryTools
    signal showInfoOverlay
    signal showChrome
    signal closeActiveTab
    signal showCertDetail
    signal loadPage(string url)

    // Used from SecondaryBar
    signal enterNewTabUrl
    signal findInPage
    signal shareActivePage
    signal bookmarkActivePage
    signal removeActivePageFromBookmarks
    signal savePageAsPDF

    function resetFind() {
        webView.sendAsyncMessage("embedui:find", { text: "", backwards: false, again: false })
        if (webView.contentItem) {
            webView.contentItem.forceChrome(false)
        }

        findInPageActive = false
    }

    width: parent.width

    onFindInPageActiveChanged: {
        // Down allow hiding of toolbar when finding text from the page.
        if (findInPageActive && webView.contentItem) {
            webView.contentItem.forceChrome(true)
        }
    }

    Item {
        id: certOverlay

        readonly property bool canDestroy: !visible && !certOverlayActive
        onCanDestroyChanged: {
            if (canDestroy) certOverlayLoader.active = false
        }

        visible: opacity > 0.0 || height > 0.0
        opacity: certOverlayActive ? 1.0 : 0.0
        height: certOverlayHeight
        width: parent.width

        Behavior on opacity { FadeAnimation {} }

        Loader {
            id: certOverlayLoader
            active: false
            sourceComponent: CertificateInfo {
                security: webView.security
                width: certOverlay.width
                height: certOverlayHeight
                opacity: Math.max((certOverlayAnimPos * 2.0) - 1.0, 0)

                onShowCertDetail: toolBarRow.showCertDetail()
                onCloseCertInfo: showChrome()

                onContentHeightChanged: {
                    if (contentHeight != 0) {
                        certOverlayPreferedHeight = contentHeight
                    }
                }
            }
            onLoaded: toolBarRow.showInfoOverlay()
        }
    }

    SecondaryBar {
        id: secondaryBar
        visible: opacity > 0.0 || height > 0.0
        opacity: secondaryToolsActive ? 1.0 : 0.0
        height: secondaryToolsHeight
        horizontalOffset: toolBarRow.horizontalOffset
        iconWidth: toolBarRow.iconWidth

        Behavior on opacity { FadeAnimation {} }
    }

    Row {
        id: toolsRow
        width: parent.width
        height: browserPage.isPortrait ? scaledPortraitHeight : scaledLandscapeHeight

        // Container item for cross fading tabs, close, find in page button (and keep Row's width still).
        Item {
            id: tabButton

            width: toolBarRow.iconWidth + toolBarRow.horizontalOffset
            height: parent.height

            Browser.TabButton {
                id: tabs

                icon.source: webView.privateMode ? "image://theme/icon-m-incognito" : "image://theme/icon-m-tabs"
                opacity: secondaryToolsActive || findInPageActive ? 0.0 : 1.0
                horizontalOffset: toolBarRow.horizontalOffset
                label.text: webView.tabModel.count
                onTapped: toolBarRow.showTabs()

                RotationAnimator {
                    id: rotationAnimator
                    target: tabs.icon
                    duration: 1500
                    alwaysRunToEnd: true
                }

                Connections {
                    target: webView.tabModel
                    onNewTabRequested: {
                        // New tab request triggers 360 degrees clockwise rotation
                        // for the tab icon.
                        rotationAnimator.from = 0
                        rotationAnimator.to = 360
                        rotationAnimator.restart()
                    }

                    onTabClosed: {
                        // Counter closewise when closing.
                        rotationAnimator.from = 0
                        rotationAnimator.to = -360
                        rotationAnimator.restart()
                    }
                }
            }

            Shared.IconButton {
                opacity: secondaryToolsActive && !findInPageActive ? 1.0 : 0.0
                icon.source: "image://theme/icon-m-tab-close"
                icon.anchors.horizontalCenterOffset: toolBarRow.horizontalOffset
                active: webView.tabModel.count > 0
                onTapped: closeActiveTab()
            }

            Shared.IconButton {
                opacity: !secondaryToolsActive && findInPageActive ? 1.0 : 0.0
                icon.source: "image://theme/icon-m-search"
                icon.anchors.horizontalCenterOffset: toolBarRow.horizontalOffset
                onTapped: {
                    findInPageActive = true
                    findInPage()
                }
            }
        }

        Shared.ExpandingButton {
            id: backIcon
            expandedWidth: toolBarRow.iconWidth
            icon.source: "image://theme/icon-m-back"
            active: webView.canGoBack && !toolBarRow.secondaryToolsActive && !findInPageActive
            onTapped: webView.goBack()
        }

        Shared.ExpandingButton {
            id: padlockIcon
            property bool danger: webView.security && webView.security.validState && !webView.security.allGood
            property real glow
            expandedWidth: toolBarRow.smallIconWidth
            icon.source: danger ? "image://theme/icon-s-filled-warning" : "image://theme/icon-s-outline-secure"
            active: webView.security && webView.security.validState && (!toolBarRow.secondaryToolsActive && !findInPageActive)
            icon.color: danger ? Qt.tint(Theme.primaryColor,
                                         Qt.rgba(Theme.errorColor.r, Theme.errorColor.g,
                                                 Theme.errorColor.b, glow))
                               : Theme.primaryColor
            enabled: webView.security
            onTapped: {
                if (certOverlayActive) {
                    showChrome()
                } else {
                    certOverlayLoader.active = true
                }
            }

            SequentialAnimation {
                id: securityAnimation
                PauseAnimation { duration: 2000 }
                NumberAnimation {
                    target: padlockIcon; property: "glow";
                    to: 0.0; duration: 1000; easing.type: Easing.OutCubic
                }
            }

            function warn() {
                glow = 1.0
                securityAnimation.start()
            }

            onDangerChanged: {
                warn()
            }

            Connections {
                target: webView
                onLoadingChanged: {
                    if (!webView.loading && padlockIcon.danger) {
                        padlockIcon.warn()
                    }
                }
            }
        }

        MouseArea {
            id: touchArea

            readonly property bool down: pressed && containsMouse

            height: parent.height
            width: toolBarRow.width - (tabButton.width + reloadButton.width + padlockIcon.width + backIcon.width + menuButton.width)
            enabled: !showFindButtons

            onClicked: {
                if (findInPageActive) {
                    findInPage()
                } else {
                    toolBarRow.showOverlay()
                }
            }

            Label {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width + Theme.paddingMedium
                color: touchArea.down ? Theme.highlightColor : Theme.primaryColor

                text: {
                    if (findInPageActive) {
                        //: No text search results were found from the page.
                        //% "No results"
                        return qsTrId("sailfish_browser-la-no_results")
                    } else if (url == "about:blank" || (webView.completed && webView.tabModel.count === 0 && !webView.tabModel.waitingForNewTab)) {
                        //: Placeholder text for url typing and searching
                        //% "Type URL or search"
                        return qsTrId("sailfish_browser-ph-type_url_or_search")
                    } else if (url) {
                        return WebUtils.displayableUrl(url)
                    } else {
                        //: Loading text that is visible when url is not yet resolved.
                        //% "Loading"
                        return qsTrId("sailfish_browser-la-loading")
                    }
                }

                truncationMode: TruncationMode.Fade

                opacity: showFindButtons ? 0.0 : 1.0
                Behavior on opacity { FadeAnimation {} }
            }

            Shared.ExpandingButton {
                id: previousFindResult
                active: showFindButtons
                expandedWidth: (toolBarRow.width - menuButton.width - tabButton.width) / 2
                icon {
                    source: "image://theme/icon-m-left"
                    anchors.horizontalCenterOffset: Theme.paddingLarge
                }

                onTapped: {
                    webView.sendAsyncMessage("embedui:find", { text: findText, backwards: true, again: true })
                }
            }

            Shared.ExpandingButton {
                active: showFindButtons
                expandedWidth: previousFindResult.width
                anchors.left: previousFindResult.right
                icon {
                    source: "image://theme/icon-m-right"
                    anchors.horizontalCenterOffset: -Theme.paddingLarge
                }

                onTapped: {
                    webView.sendAsyncMessage("embedui:find", { text: findText, backwards: false, again: true })
                }
            }
        }

        Shared.ExpandingButton {
            id: reloadButton
            expandedWidth: toolBarRow.iconWidth
            icon.source: webView.loading ? "image://theme/icon-m-reset" : "image://theme/icon-m-refresh"
            active: webView.contentItem && !toolBarRow.secondaryToolsActive && !findInPageActive
            onTapped: {
                if (webView.loading) {
                    webView.stop()
                } else {
                    webView.reload()
                }
                toolBarRow.showChrome()
            }
        }

        Item {
            id: menuButton

            width: toolBarRow.iconWidth + toolBarRow.horizontalOffset
            height: parent.height

            Shared.IconButton {
                icon.source: "image://theme/icon-m-menu"
                icon.anchors.horizontalCenterOffset: - toolBarRow.horizontalOffset
                width: parent.width
                opacity: secondaryToolsActive || findInPageActive ? 0.0 : 1.0
                onTapped: showSecondaryTools()
            }

            Shared.IconButton {
                icon.source: "image://theme/icon-m-menu"
                icon.anchors.horizontalCenterOffset: toolBarRow.horizontalOffset
                width: parent.width
                rotation: 180
                opacity: secondaryToolsActive && !findInPageActive ? 1.0 : 0.0
                onTapped: showChrome()
            }

            Shared.IconButton {
                icon.source: "image://theme/icon-m-reset"
                icon.anchors.horizontalCenterOffset: - toolBarRow.horizontalOffset
                width: parent.width
                opacity: !secondaryToolsActive && findInPageActive ? 1.0 : 0.0
                onTapped: {
                    resetFind()
                    showChrome()
                }
            }
        }
    }
}
