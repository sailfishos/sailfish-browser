/*
 * Copyright (c) 2014 - 2019 Jolla Ltd.
 * Copyright (c) 2019 Open Mobile Platform LLC.
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
import com.jolla.settings.system 1.0
import "." as Browser

Background {
    id: overlay

    property bool active
    property QtObject webView
    property Item browserPage
    property alias historyModel: historyList.model
    property alias toolBar: toolBar
    property alias progressBar: progressBar
    property alias animator: overlayAnimator
    property alias dragArea: dragArea
    property alias searchField: searchField
    readonly property alias enteringNewTabUrl: searchField.enteringNewTabUrl

    property string enteredUrl

    property real _overlayHeight: browserPage.isPortrait ? toolBar.toolsHeight : 0
    property bool _showFindInPage
    property bool _showUrlEntry: true
    property bool _showInfoOverlay
    readonly property bool _topGap: _showUrlEntry || _showFindInPage

    function loadPage(url)  {
        if (url == "about:config") {
            pageStack.animatorPush(Qt.resolvedUrl("ConfigWarning.qml"), {"browserPage": browserPage})
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
        overlayAnimator.showOverlay(action === PageStackAction.Immediate)
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

    y: webView.fullscreenHeight - toolBar.toolsHeight

    Private.VirtualKeyboardObserver {
        id: virtualKeyboardObserver
        active: overlay.active && !overlayAnimator.atBottom
        orientation: browserPage.orientation
    }

    width: parent.width
    height: historyContainer.height + virtualKeyboardObserver.panelSize
    // `visible` is controlled by Browser.OverlayAnimator
    enabled: visible

    // This is an invisible object responsible to hide/show Overlay in an animated way
    Browser.OverlayAnimator {
        id: overlayAnimator

        overlay: overlay
        portrait: browserPage.isPortrait
        webView: overlay.webView

        readonly property real _fullHeight: isPortrait ? overlay.toolBar.toolsHeight : 0
        readonly property real _infoHeight: Math.max(webView.fullscreenHeight - overlay.toolBar.certOverlayPreferedHeight - overlay.toolBar.toolsHeight, 0)

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
                _showInfoOverlay = false
            }
            dragArea.moved = false
        }

        onAtTopChanged: {
            if (atTop) {
                if (_showFindInPage || _showUrlEntry) {
                    _showInfoOverlay = false
                }
            } else {
                if (!_showInfoOverlay) {
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
        property int dragThreshold: state === "fullscreenOverlay" ? toolBar.toolsHeight * 1.5
                                                                  : state === "certOverlay"
                                                                    ? (overlayAnimator._infoHeight + toolBar.toolsHeight * 0.5)
                                                                    : (webView.fullscreenHeight - toolBar.toolsHeight * 2)

        width: parent.width
        height: historyContainer.height
        enabled: !overlayAnimator.atBottom && webView.tabModel.count > 0 && !favoriteGrid.contextMenuActive

        drag.target: overlay
        drag.filterChildren: true
        drag.axis: Drag.YAxis
        // Favorite grid first row offset is negative. So, increase minumumY drag by that.
        drag.minimumY: _overlayHeight
        drag.maximumY: webView.fullscreenHeight - toolBar.toolsHeight

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

        Browser.ProgressBar {
            id: progressBar
            width: parent.width
            height: toolBar.toolsHeight
            visible: !searchField.enteringNewTabUrl
            opacity: webView.loading ? 1.0 : 0.0
            progress: webView.loadProgress / 100.0
        }

        Item {
            id: historyContainer

            readonly property bool showFavorites: !overlayAnimator.atBottom && (!searchField.edited && searchField.text === webView.url || !searchField.text) && !toolBar.findInPageActive && _showUrlEntry

            width: parent.width
            height: toolBar.toolsHeight + historyList.height
            // Clip only when content has been moved and we're at top or animating downwards.
            clip: (overlayAnimator.atTop ||
                   overlayAnimator.direction === "downwards" ||
                   overlayAnimator.direction === "upwards" ||
                   favoriteGrid.opacity != 0.0 ||
                   historyList.opacity != 0.0) && searchField.y < 0

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
                    Browser.TextSelectionToolbar {
                        property Item controller: webView && webView.contentItem && webView.contentItem.textSelectionController
                        horizontalOffset: toolBar.horizontalOffset
                        iconWidth: toolBar.iconWidth
                        isPhoneNumber: active && controller.isPhoneNumber
                        onCall: {
                            Qt.openUrlExternally("tel:" + controller.text)
                            controller.clearSelection()
                        }

                        onShare: {
                            controller.clearSelection()
                            pageStack.animatorPush("Sailfish.WebView.Popups.ShareTextPage", {"text" : controller.text })
                        }
                        onSearch: {
                            // Open new tab with the search uri.
                            controller.clearSelection()
                            webView.tabModel.newTab(controller.searchUri)
                            overlay.animator.showChrome(true)
                        }
                        onClear: {
                            if (controller) {
                                controller.clearSelection()
                            }
                        }
                    }
                }
            }

            TextField {
                id: searchField

                readonly property bool requestingFocus: AccessPolicy.browserEnabled && overlayAnimator.atTop && browserPage.active && !dragArea.moved && (_showFindInPage || _showUrlEntry)

                // Release focus when ever history list or favorite grid is moved and overlay itself starts moving
                // from the top. After moving the overlay or the content, search field can be focused by tapping.
                readonly property bool focusOut: dragArea.moved

                readonly property bool browserEnabled: AccessPolicy.browserEnabled
                onBrowserEnabledChanged: if (!browserEnabled) focus = false

                property bool edited
                property bool enteringNewTabUrl

                function resetUrl(url) {
                    // Reset first text and then mark as unedited.
                    text = url === "about:blank" ? "" : url || ""
                    edited = false
                }

                // Follow grid / list position.
                y: -((historyContainer.showFavorites ? favoriteGrid.contentY : historyList.contentY) + height)
                // On top of HistoryList and FavoriteGrid
                z: 1
                width: parent.width
                height: Theme.itemSizeMedium
                textLeftMargin: Theme.paddingLarge
                textRightMargin: Theme.paddingLarge
                focusOutBehavior: FocusBehavior.ClearPageFocus
                font {
                    pixelSize: Theme.fontSizeLarge
                    family: Theme.fontFamilyHeading
                }

                textTopMargin: height/2 - _editor.implicitHeight/2
                labelVisible: false
                inputMethodHints: Qt.ImhUrlCharactersOnly
                background: null

                placeholderText: toolBar.findInPageActive ?
                                     //: Placeholder text for finding text from the web page
                                     //% "Find from page"
                                     qsTrId("sailfish_browser-ph-type_find_from_page") :
                                     //: Placeholder text for url typing and searching
                                     //% "Type URL or search"
                                     qsTrId("sailfish_browser-ph-type_url_or_search")

                EnterKey.onClicked: {
                    if (!text) {
                        return
                    }

                    if (toolBar.findInPageActive) {
                        webView.sendAsyncMessage("embedui:find", { text: text, backwards: false, again: false })
                        overlayAnimator.showChrome()
                    } else {
                        overlay.loadPage(text)
                    }
                }

                opacity: toolBar.crossfadeRatio * -1.0
                visible: opacity > 0.0 && y >= -searchField.height

                onYChanged: {
                    if (y < 0) {
                        dragArea.moved = true
                    }
                }

                onRequestingFocusChanged: {
                    if (requestingFocus) {
                        forceActiveFocus()
                    }
                }

                onFocusOutChanged: {
                    if (focusOut) {
                        overlay.focus = true
                    }
                }

                onFocusChanged: {
                    if (focus) {
                        cursorPosition = text.length
                        // Mark SearchField as edited if focused before url is resolved.
                        // Otherwise select all.
                        if (!text) {
                            edited = true
                        } else {
                            searchField.selectAll()
                        }
                        dragArea.moved = false
                    }
                }

                onTextChanged: {
                    if (!edited && text !== webView.url) {
                        edited = true
                    }
                }
            }

            // Below the HistoryList and FavoriteGrid to let dragging to work
            // when finding from the page active or favorite grid enabled (at top).
            // On large screens favorite grid is not covering fullscreen. Thus,
            // this needs to be enabled so that overlay can be dragged from edges.
            MouseArea {
                anchors {
                    fill: historyList
                    topMargin: _topGap ? searchField.height : 0
                    bottomMargin: _topGap ? 0 : searchField.height
                }
                enabled: toolBar.findInPageActive || favoriteGrid.enabled

                ViewPlaceholder {
                    // The parent is a sibling of the historyList. Hence,
                    // flickable binding.
                    flickable: historyList
                    x: (historyList.width - width) / 2
                    y: historyList.originY + (historyList.height - height) / 2

                    enabled: toolBar.findInPageActive && searchField.text

                    //: View placeholder text for find-in-page search.
                    //% "Press enter to search"
                    text: qsTrId("sailfish_browser-la-press_enter_to_search")
                }
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
                certOverlayActive: _showInfoOverlay
                certOverlayHeight: !_showInfoOverlay
                                   ? 0
                                   : Math.max((webView.fullscreenHeight - overlay.y - overlay.toolBar.toolsHeight)
                                              - overlay.toolBar.secondaryToolsHeight, 0)

                certOverlayAnimPos: Math.min(Math.max((webView.fullscreenHeight - overlay.y - overlay.toolBar.toolsHeight)
                                                      / (webView.fullscreenHeight - overlayAnimator._infoHeight
                                                         - overlay.toolBar.toolsHeight), 0.0), 1.0)

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
                    _showInfoOverlay = true
                    _overlayHeight = Qt.binding(function() { return overlayAnimator._infoHeight })
                    overlayAnimator.showInfoOverlay(false)
                }
                onShowChrome: overlayAnimator.showChrome()

                onCloseActiveTab: {
                    // Activates (loads) the tab next to the currect active.
                    webView.tabModel.closeActiveTab()
                    if (webView.tabModel.count == 0) {
                        overlay.enterNewTabUrl()
                    }
                }

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
            }

            Browser.HistoryList {
                id: historyList

                property int panelSize: favoriteGrid.contextMenu && favoriteGrid.contextMenu.active ? 0 : virtualKeyboardObserver.panelSize

                width: parent.width
                height: browserPage.height - _overlayHeight - panelSize

                header: Item {
                    width: parent.width
                    height: searchField.height
                }

                search: searchField.text
                opacity: historyContainer.showFavorites || toolBar.opacity > 0.9 ? 0.0 : 1.0
                enabled: overlayAnimator.atTop
                visible: !overlayAnimator.atBottom && !historyContainer.showFavorites && _showUrlEntry

                onMovingChanged: if (moving) historyList.focus = true
                onSearchChanged: if (search !== webView.url) historyModel.search(search)
                onLoad: overlay.loadPage(url)

                Behavior on opacity { FadeAnimator {} }
            }

            Browser.FavoriteGrid {
                id: favoriteGrid

                height: historyList.height
                opacity: historyContainer.showFavorites && toolBar.opacity < 0.9 ? 1.0 : 0.0
                enabled: overlayAnimator.atTop
                visible: historyContainer.showFavorites
                _quickScrollRightMargin: -(browserPage.width - width) / 2

                header: Item {
                    width: parent.width
                    height: searchField.height
                }

                model: BookmarkModel {
                    id: bookmarkModel
                    activeUrl: toolBar.url
                }

                onMovingChanged: if (moving) favoriteGrid.focus = true
                onLoad: overlay.loadPage(url)
                onNewTab: {
                    searchField.resetUrl(url)
                    // Not the best property name but functionality of opening a favorite
                    // to a new tab is exactly the same as opening new tab by typing a url.
                    searchField.enteringNewTabUrl = true
                    _showUrlEntry = true
                    overlay.loadPage(url)
                }

                onShare: pageStack.animatorPush("Sailfish.WebView.Popups.ShareLinkPage", {"link" : url, "linkTitle": title})

                Behavior on opacity { FadeAnimator {} }
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
                        overlay.enterNewTabUrl(PageStackAction.Immediate)
                    }
                }

                onCloseAllPending: overlay.enterNewTabUrl(PageStackAction.Immediate)
                onCloseAllCanceled: overlay.dismiss(true /* show chrome */, true /* immediate */)
                onCloseAll: {
                    webView.tabModel.clear()
                    overlay.enterNewTabUrl(PageStackAction.Immediate)
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
