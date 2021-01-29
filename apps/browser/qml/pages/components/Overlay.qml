/*
 * Copyright (c) 2014 - 2019 Jolla Ltd.
 * Copyright (c) 2019 - 2021 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.2
import QtQuick.Window 2.2 as QuickWindow
import Sailfish.Silica 1.0
import Sailfish.Silica.private 1.0 as Private
import Sailfish.Browser 1.0
import Sailfish.Policy 1.0
import Sailfish.WebView.Controls 1.0
import Sailfish.WebEngine 1.0
import com.jolla.settings.system 1.0
import "." as Browser
import "../../shared" as Shared

Shared.Background {
    id: overlay

    property bool active
    property QtObject webView
    property Item browserPage
    property alias historyModel: overlayContainer.historyModel
    property alias bookmarkModel: overlayContainer.bookmarkModel

    property alias toolBar: toolBar
    property alias progressBar: overlayContainer.progressBar
    property alias animator: overlayAnimator
    property alias dragArea: dragArea
    property alias searchField: overlayContainer.searchField
    readonly property alias enteringNewTabUrl: overlayContainer.enteringNewTabUrl

    property alias favoriteGrid: overlayContainer.favoriteItem
    property alias historyList: overlayContainer.historyItem

    property string enteredUrl

    property real _overlayHeight: browserPage.isPortrait ? toolBar.rowHeight : 0
    property bool _showFindInPage
    property bool _showUrlEntry
    readonly property bool _topGap: _showUrlEntry || _showFindInPage

    function loadPage(url) {
        if (url == "about:config") {
            pageStack.animatorPush(Qt.resolvedUrl("ConfigWarning.qml"), {"browserPage": browserPage})
        } else if (url == "about:settings") {
            pageStack.animatorPush(Qt.resolvedUrl("../SettingsPage.qml"))
        } else {
            if (webView && webView.tabModel.count === 0) {
                webView.clearSurface();
            }
            // let gecko figure out how to handle malformed URLs
            var pageUrl = url
            if (!isNaN(pageUrl) && pageUrl.trim()) {
                pageUrl = "\"" + pageUrl.trim() + "\""
            }

            if (!searchField.enteringNewTabUrl) {
                webView.releaseActiveTabOwnership()
                webView.load(pageUrl)
            } else {
                // Loading will start once overlay animator has animated chrome visible.
                enteredUrl = pageUrl
                webView.tabModel.waitingForNewTab = true
            }
        }

        overlayAnimator.showChrome()
    }

    function enterNewTabUrl(action) {
        searchField.enteringNewTabUrl = true
        _showUrlEntry = true
        _overlayHeight = Qt.binding(function () { return overlayAnimator._fullHeight })
        searchField.resetUrl("")
        overlay.animator.updateState("fullscreenWebPage")
        overlayAnimator.showOverlay(action === PageStackAction.Immediate)
    }

    function startPage() {
        searchField.enteringNewTabUrl = true
        _showUrlEntry = true
        searchField.resetUrl("")
        overlay.animator.updateState("startPage", PageStackAction.Immediate)
    }

    function dismiss(canShowChrome, immediate) {
        toolBar.resetFind()
        if (webView.contentItem && webView.contentItem.fullscreen) {
            // Web content is in fullscreen mode thus we don't show chrome
            overlay.animator.updateState("fullscreenWebPage")
        } else if (canShowChrome) {
            overlay.animator.showChrome(immediate)
        } else {
            overlay.animator.hide()
        }
        searchField.enteringNewTabUrl = false
    }

    y: webView.fullscreenHeight - toolBar.rowHeight

    Private.VirtualKeyboardObserver {
        id: virtualKeyboardObserver
        active: overlay.active && !overlayAnimator.atBottom
        orientation: browserPage.orientation
    }

    width: parent.width
    height: overlayContainer.height + virtualKeyboardObserver.panelSize
    // `visible` is controlled by Browser.OverlayAnimator
    enabled: visible

    // This is an invisible object responsible to hide/show Overlay in an animated way
    Shared.OverlayAnimator {
        id: overlayAnimator

        overlay: overlay
        portrait: browserPage.isPortrait
        webView: overlay.webView

        readonly property real _fullHeight: isPortrait ? overlay.toolBar.rowHeight : 0
        readonly property real _infoHeight: Math.max(webView.fullscreenHeight - overlay.toolBar.certOverlayPreferedHeight - overlay.toolBar.rowHeight, 0)

        onAtBottomChanged: {
            if (atBottom) {
                searchField.enteringNewTabUrl = false

                if (enteredUrl) {
                    webView.tabModel.newTab(enteredUrl)
                    enteredUrl = ""
                } else if (!toolBar.findInPageActive) {
                    searchField.resetUrl(webView.url)
                }

                favoriteGrid.positionViewAtBeginning()
                historyList.positionViewAtBeginning()

                if (!WebUtils.firstUseDone) {
                    WebUtils.firstUseDone = true
                }

                _showFindInPage = false
                _showUrlEntry = false
                toolBar.certOverlayActive = false
            }
            dragArea.moved = false
        }

        onAtTopChanged: {
            if (atTop) {
                if (_showFindInPage || _showUrlEntry) {
                    toolBar.certOverlayActive = false
                }
            } else {
                if (!toolBar.certOverlayActive) {
                    dragArea.moved = true
                }
            }
        }
    }

    Connections {
        target: webView

        onLoadingChanged: {
            if (webView.loading) {
                toolBar.resetFind()
            }
        }

        onUrlChanged: {
            if (!toolBar.findInPageActive && !searchField.enteringNewTabUrl && !searchField.edited) {
                searchField.resetUrl(webView.url)
            }
        }
    }

    MouseArea {
        id: dragArea

        property bool moved
        property int dragThreshold: state === "fullscreenOverlay" ? toolBar.rowHeight * 1.5
                                                                  : state === "certOverlay"
                                                                    ? (overlayAnimator._infoHeight + toolBar.rowHeight * 0.5)
                                                                    : (webView.fullscreenHeight - toolBar.rowHeight * 2)

        width: parent.width
        height: overlayContainer.height
        enabled: !overlayAnimator.atBottom && webView.tabModel.count > 0 && !favoriteGrid.contextMenuActive

        drag.target: overlay
        drag.filterChildren: true
        drag.axis: Drag.YAxis
        // Favorite grid first row offset is negative. So, increase minumumY drag by that.
        drag.minimumY: _overlayHeight
        drag.maximumY: webView.fullscreenHeight - toolBar.rowHeight

        drag.onActiveChanged: {
            if (!drag.active) {
                if (overlay.y < dragThreshold) {
                    if (state === "certOverlay") {
                        overlayAnimator.showInfoOverlay(false)
                    } else {
                        overlayAnimator.showOverlay(false)
                    }
                } else {
                    dismiss(true)
                }
            } else {
                // Store previous end state
                if (!overlayAnimator.dragging) {
                    state = overlayAnimator.state
                }

                overlayAnimator.drag()
            }
        }

        OverlayItem {
            id: overlayContainer
            webView: overlay.webView
            browserPage: overlay.browserPage
            dragArea: dragArea
            enteredUrl: overlay.enteredUrl
            _overlayHeight: overlay._overlayHeight
            _showFindInPage: overlay._showFindInPage
            _showUrlEntry: overlay._showUrlEntry
            tabView: tabView
            toolBar: toolBar
            overlayAnimator: overlayAnimator
        }

        PrivateModeTexture {
            opacity: toolBar.visible && webView.privateMode ? toolBar.opacity : 0.0
        }

        Loader {
            id: textSelectionToolbar

            width: parent.width
            height: isPortrait ? toolBar.scaledPortraitHeight : toolBar.scaledLandscapeHeight
            active: webView.contentItem && webView.contentItem.textSelectionActive

            opacity: active ? 1.0 : 0.0
            Behavior on opacity {
                FadeAnimator {}
            }

            onActiveChanged: {
                if (active) {
                    overlayAnimator.showChrome(false)
                    if (webView.contentItem) {
                        webView.contentItem.forceChrome(true)
                    }
                } else {
                    if (webView.contentItem) {
                        webView.contentItem.forceChrome(false)
                    }
                }
            }

            sourceComponent: Component {
                TextSelectionToolbar {
                    controller: webView && webView.contentItem && webView.contentItem.textSelectionController
                    width: textSelectionToolbar.width
                    height: textSelectionToolbar.height
                    leftPadding: toolBar.horizontalOffset
                    rightPadding: toolBar.horizontalOffset
                    onCall: {
                        Qt.openUrlExternally("tel:" + controller.text)
                    }

                    onShare: {
                        pageStack.animatorPush("Sailfish.WebView.Popups.ShareTextPage", {"text" : controller.text })
                    }
                    onSearch: {
                        // Open new tab with the search uri.
                        webView.tabModel.newTab(controller.searchUri)
                        overlay.animator.showChrome(true)
                    }
                }
            }
        }


        ViewPlaceholder {
            // The parent is a sibling of the historyList. Hence,
            // flickable binding.
            flickable: overlayContainer
            x: (overlayContainer.width - width) / 2
            y: overlayContainer.originY + (overlayContainer.height - height) / 2
            enabled: toolBar.findInPageActive && searchField.text
            //: View placeholder text for find-in-page search.
            //% "Press enter to search"
            text: qsTrId("sailfish_browser-la-press_enter_to_search")
        }

        Browser.ToolBar {
            id: toolBar

            property real crossfadeRatio: (_showFindInPage || _showUrlEntry) ? (overlay.y - webView.fullscreenHeight/2)  / (webView.fullscreenHeight/2 - toolBar.height) : 1.0

            url: webView.contentItem && webView.contentItem.url || ""
            findText: searchField.text
            bookmarked: bookmarkModel.activeUrlBookmarked

            opacity: textSelectionToolbar.active ? 0.0 : crossfadeRatio
            Behavior on opacity {
                enabled: overlayAnimator.atBottom
                FadeAnimation {}
            }

            visible: opacity > 0.0
            secondaryToolsActive: overlayAnimator.secondaryTools
            certOverlayHeight: !toolBar.certOverlayActive
                               ? 0
                               : Math.max((webView.fullscreenHeight - overlay.y - overlay.toolBar.rowHeight)
                                          - overlay.toolBar.secondaryToolsHeight, 0)

            certOverlayAnimPos: Math.min(Math.max((webView.fullscreenHeight - overlay.y - overlay.toolBar.rowHeight)
                                                  / (webView.fullscreenHeight - overlayAnimator._infoHeight
                                                     - overlay.toolBar.rowHeight), 0.0), 1.0)

            onShowOverlay: {
                _showUrlEntry = true
                _overlayHeight = Qt.binding(function() { return overlayAnimator._fullHeight })
                searchField.resetUrl(webView.url)
                overlayAnimator.showOverlay()
            }
            onShowTabs: {
                // Push the currently active tab index.
                // Changing of active tab cannot cause blinking.
                webView.grabActivePage()
                pageStack.animatorPush(tabView)
            }
            onShowSecondaryTools: overlayAnimator.showSecondaryTools()
            onShowInfoOverlay: {
                toolBar.certOverlayActive = true
                _overlayHeight = Qt.binding(function() { return overlayAnimator._infoHeight })
                overlayAnimator.showInfoOverlay(false)
            }
            onShowChrome: overlayAnimator.showChrome()

            onCloseActiveTab: {
                // Activates (loads) the tab next to the currect active.
                webView.tabModel.closeActiveTab()
                if (webView.tabModel.count === 0) {
                    overlay.startPage()
                }
            }

            onLoadPage: overlay.loadPage(url)
            onEnterNewTabUrl: overlay.enterNewTabUrl()
            onFindInPage: {
                _showFindInPage = true
                searchField.resetUrl("")
                _overlayHeight = Qt.binding(function () { return overlayAnimator._fullHeight })
                overlayAnimator.showOverlay()
            }
            onShareActivePage: {
                pageStack.animatorPush("Sailfish.WebView.Popups.ShareLinkPage", {
                                           "link": webView.url,
                                           "linkTitle": webView.title
                                       })
            }
            onBookmarkActivePage: favoriteGrid.fetchAndSaveBookmark()
            onRemoveActivePageFromBookmarks: bookmarkModel.remove(webView.url)

            onShowCertDetail: {
                if (webView.security && !webView.security.certIsNull) {
                    pageStack.animatorPush("com.jolla.settings.system.CertificateDetailsPage", {"website": webView.security.subjectDisplayName, "details": webView.security.serverCertDetails})
                }
            }
            onSavePageAsPDF: {
                var filename = ((webView.title && webView.title.length !== 0) ? webView.title : (WebUtils.pageName(webView.url) || "unnamed_file")) + ".pdf"
                var targetUrl = DownloadHelper.createUniqueFileUrl(filename, StandardPaths.download)
                WebEngine.notifyObservers("embedui:download",
                                          {
                                              "msg": "saveAsPdf",
                                              "to": targetUrl
                                          })
            }
        }
    }

    Component {
        id: tabView
        Page {
            id: tabPage

            onStatusChanged: browserPage.tabPageActive = (status == PageStatus.Active)

            Browser.TabView {
                id: tabViewItem

                model: webView.tabModel
                portrait: tabPage.isPortrait
                privateMode: webView.privateMode

                onHide: pageStack.pop()

                onPrivateModeChanged: {
                    webView.privateMode = privateMode
                    if (webView.tabModel.count === 0) {
                        overlay.enterNewTabUrl(PageStackAction.Immediate)
                    } else if (!overlayAnimator.atBottom) {
                        // Hide overlay while switching to non-empty tabmodel
                        // Dismiss overlay so that chrome gets hidden.
                        // Chrome is animated back when BrowserPage's activates.
                        dismiss(false)
                    }
                }

                onEnterNewTabUrl: {
                    overlay.enterNewTabUrl(PageStackAction.Immediate)
                    pageStack.pop()
                }
                onActivateTab: {
                    webView.tabModel.activateTab(index)
                    pageStack.pop()
                }
                onCloseTab: {
                    webView.tabModel.remove(index)
                    if (webView.tabModel.count === 0) {
                        overlay.startPage()
                    }
                }

                onCloseAllPending: overlay.enterNewTabUrl(PageStackAction.Immediate)
                onCloseAllCanceled: overlay.dismiss(true /* show chrome */, true /* immediate */)
                onCloseAll: {
                    webView.tabModel.clear()
                    overlay.startPage()
                }

                Component.onCompleted: {
                    positionViewAtIndex(webView.tabModel.activeTabIndex, ListView.Center)
                    window.setBrowserCover(webView.tabModel)
                }

                Component.onDestruction: window.setBrowserCover(webView.tabModel)
            }

            Component.onCompleted: {
                // order of completion of this an tabview is undefined. reposition on both.
                tabViewItem.positionViewAtIndex(webView.tabModel.activeTabIndex, ListView.Center)
            }
        }
    }
}
