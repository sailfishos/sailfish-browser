/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import "." as Browser

PanelBackground {
    id: overlay

    property bool active
    property Item webView
    property Item browserPage
    property alias historyModel: historyList.model
    property alias toolBar: toolBar
    property alias progressBar: progressBar
    property alias animator: overlayAnimator

    property var enteredPage

    function loadPage(url, title)  {
        if (firstUseOverlay) {
            firstUseOverlay.done()
        }

        if (url == "about:config") {
            pageStack.push(Qt.resolvedUrl("ConfigWarning.qml"), {"browserPage": browserPage});
        } else {
            // let gecko figure out how to handle malformed URLs
            var pageUrl = url
            var pageTitle = title || ""
            if (!isNaN(pageUrl) && pageUrl.trim()) {
                pageUrl = "\"" + pageUrl.trim() + "\""
            }

            if (!searchField.enteringNewTabUrl) {
                webView.load(pageUrl, pageTitle)
            } else {
                // Loading will start once overlay animator has animated chrome visible.
                enteredPage = {
                    "url": pageUrl,
                    "title": pageTitle
                }
                webView.tabModel.waitingForNewTab = true
            }
            webView.focus = true
        }

        overlayAnimator.showChrome()
    }

    function enterNewTabUrl(action) {
        if (webView.contentItem) {
            webView.contentItem.opacity = 0.0
        }

        searchField.enteringNewTabUrl = true
        searchField.resetUrl("")
        overlayAnimator.showOverlay(action === PageStackAction.Immediate)
    }

    function dismiss() {
        if (webView.contentItem) {
            webView.contentItem.opacity = 1.0
        }
        toolBar.resetFind()
        if (webView.contentItem && webView.contentItem.fullscreen) {
            // Web content is in fullscreen mode thus we don't show chrome
            overlay.animator.updateState("fullscreenWebPage")
        } else {
            overlay.animator.showChrome()
        }
    }

    y: webView.fullscreenHeight - toolBar.toolsHeight

    width: parent.width
    height: historyContainer.height + privateModeLabel.height
    // `visible` is controlled by Browser.OverlayAnimator
    enabled: visible

    onActiveChanged: {
        if (active && !webView.contentItem && !searchField.enteringNewTabUrl && webView.tabId > 0) {
            // Don't force reloading tab change if already loaded.
            webView.reload(false)
        }
    }

    gradient: Gradient {
        GradientStop { position: 0.0; color: Theme.rgba(Theme.highlightBackgroundColor, 0.3) }
        GradientStop { position: 1.0; color: Theme.rgba(Theme.highlightBackgroundColor, 0.0) }
    }

    // Immediately active WebView height binding when dragging
    // starts. If this binding is removed, state change to
    // "draggingOverlay" at OverlayAnimator causes a visual glitch
    // right after transition to "draggingOverlay" has finnished.
    Binding {
        target: webView
        property: "height"
        value: overlay.y
        when: dragArea.drag.active
    }

    // This is an invisible object responsible to hide/show Overlay in an animated way
    Browser.OverlayAnimator {
        id: overlayAnimator

        overlay: overlay
        portrait: browserPage.isPortrait
        active: Qt.application.active
        webView: firstUseOverlay ? firstUseOverlay : overlay.webView

        onAtBottomChanged: {
            if (atBottom) {
                searchField.enteringNewTabUrl = false

                if (enteredPage) {
                    webView.tabModel.newTab(enteredPage.url, enteredPage.title)
                    enteredPage = null
                } else if (!toolBar.findInPageActive) {
                    searchField.resetUrl(webView.url)
                }

                favoriteGrid.positionViewAtBeginning()
                historyList.positionViewAtBeginning()
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

    Image {
        anchors.fill: parent
        source: "image://theme/graphic-gradient-edge"
    }

    Label {
        id: privateModeLabel

        PrivateModeTexture {
            anchors.fill: parent
        }

        //: Label for private browsing above address bar
        //% "Private browsing"
        text: qsTrId("sailfish_browser-la-private_mode")
        visible: searchField.visible && webView.privateMode
        height: toolBar.height
        width: parent.width

        anchors.bottom: dragArea.top
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        color: Theme.highlightColor
        font.pixelSize: Theme.fontSizeLarge
    }


    Browser.ProgressBar {
        id: progressBar
        width: parent.width
        height: toolBar.toolsHeight
        visible: !firstUseOverlay && !searchField.enteringNewTabUrl
        opacity: webView.loading ? 1.0 : 0.0
        progress: webView.loadProgress / 100.0
    }

    MouseArea {
        id: dragArea

        property int dragThreshold: state === "fullscreenOverlay" ? toolBar.toolsHeight * 1.5 :
                                                                    state === "doubleToolBar" ?
                                                                        (webView.fullscreenHeight - toolBar.toolsHeight * 4) :
                                                                        (webView.fullscreenHeight - toolBar.toolsHeight * 2)

        width: parent.width
        height: historyContainer.height
        enabled: !overlayAnimator.atBottom && (webView.tabModel.count > 0 || firstUseOverlay) && !favoriteGrid.contextMenuActive

        drag.target: overlay
        drag.filterChildren: true
        drag.axis: Drag.YAxis
        drag.minimumY: browserPage.isPortrait ? toolBar.toolsHeight : 0
        drag.maximumY: browserPage.isPortrait ? webView.fullscreenHeight - toolBar.toolsHeight : webView.fullscreenHeight

        drag.onActiveChanged: {
            if (!drag.active) {
                if (overlay.y < dragThreshold) {
                    overlayAnimator.showOverlay(false)
                } else {
                    dismiss()
                }
            } else {
                // Store previous end state
                if (!overlayAnimator.dragging) {
                    state = overlayAnimator.state
                }

                overlayAnimator.drag()
            }
        }

        Item {
            id: historyContainer

            readonly property bool showFavorites: (!searchField.edited && searchField.text === webView.url || !searchField.text)

            width: parent.width
            height: toolBar.toolsHeight + historyList.height
            clip: true

            PrivateModeTexture {
                visible: toolBar.visible && webView.privateMode
            }

            Browser.ToolBar {
                id: toolBar

                url: webView.contentItem && webView.contentItem.url || ""
                findText: searchField.text
                bookmarked: bookmarkModel.count && bookmarkModel.contains(webView.url)
                opacity: (overlay.y - webView.fullscreenHeight/2)  / (webView.fullscreenHeight/2 - toolBar.height)
                visible: opacity > 0.0
                secondaryToolsActive: overlayAnimator.secondaryTools

                onShowOverlay: {
                    searchField.resetUrl(webView.url)
                    overlayAnimator.showOverlay()
                }
                onShowTabs: {
                    overlayAnimator.showChrome()
                    // Push the tab index and active page that were current at this moment.
                    // Changing of active tab cannot cause blinking.
                    pageStack.push(tabView, {
                                       "activeTabIndex": webView.tabModel.activeTabIndex,
                                       "activeWebPage": webView.contentItem
                                   })
                }
                onShowSecondaryTools: overlayAnimator.showSecondaryTools()
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
                    searchField.resetUrl("")
                    overlayAnimator.showOverlay()
                }
                onShareActivePage: {
                    pageStack.push(Qt.resolvedUrl("../ShareLinkPage.qml"), {
                                       "link" : webView.url,
                                       "linkTitle": webView.title
                                   })
                }
                onBookmarkActivePage: favoriteGrid.fetchAndSaveBookmark()
                onRemoveActivePageFromBookmarks: bookmarkModel.removeBookmark(webView.url)
            }

            SearchField {
                id: searchField

                readonly property bool overlayAtTop: overlayAnimator.atTop
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
                textLeftMargin: Theme.paddingLarge
                textRightMargin: Theme.paddingLarge
                inputMethodHints: Qt.ImhUrlCharactersOnly

                placeholderText: toolBar.findInPageActive ?
                                     //: Placeholder text for finding text from the web page
                                     //% "Find from page"
                                     qsTrId("sailfish_browser-ph-type_find_from_page") :
                                     //: Placeholder text for url typing and searching
                                     //% "Type URL or search"
                                     qsTrId("sailfish_browser-ph-type_url_or_search")
                EnterKey.onClicked: {
                    if (toolBar.findInPageActive) {
                        if (text) {
                            webView.sendAsyncMessage("embedui:find", { text: text, backwards: false, again: false })
                            overlayAnimator.showChrome()
                        }
                    } else {
                        overlay.loadPage(text)
                    }
                }

                background: null
                opacity: toolBar.opacity * -1.0
                visible: opacity > 0.0
                onOverlayAtTopChanged: {
                    if (overlayAtTop) {
                        forceActiveFocus()
                    } else {
                        focus = false
                    }
                }

                onFocusChanged: {
                    if (focus) {
                        searchField.selectAll()
                    }
                }

                onTextChanged: {
                    if (!edited && text !== webView.url) {
                        edited = true
                    }
                }
            }

            // Below the HistoryList and FavoriteGrid to let dragging to work
            // when finding from the page
            MouseArea {
                anchors {
                    fill: historyList
                    topMargin: searchField.height
                }
                enabled: toolBar.findInPageActive

                ViewPlaceholder {
                    // The parent is a sibling of the historyList. Hence,
                    // flickable binding.
                    flickable: historyList
                    x: (historyList.width - width) / 2
                    y: historyList.originY + (historyList.height - height) / 2

                    enabled: parent.enabled && searchField.text

                    //: View placeholder text for find-in-page search.
                    //% "Press enter to search"
                    text: qsTrId("sailfish_browser-la-press_enter_to_search")
                }
            }

            Browser.HistoryList {
                id: historyList

                width: parent.width
                height: browserPage.height - dragArea.drag.minimumY

                header: Item {
                    width: parent.width
                    height: searchField.height
                }

                search: searchField.text
                opacity: historyContainer.showFavorites ? 0.0 : 1.0
                enabled: overlayAnimator.atTop
                visible: !overlayAnimator.atBottom && !toolBar.findInPageActive && opacity > 0.0

                onMovingChanged: if (moving) historyList.focus = true
                onSearchChanged: if (search !== webView.url) historyModel.search(search)
                onLoad: overlay.loadPage(url, title)

                Behavior on opacity { FadeAnimation {} }
            }

            Browser.FavoriteGrid {
                id: favoriteGrid

                height: historyList.height
                anchors.horizontalCenter: parent.horizontalCenter
                opacity: historyContainer.showFavorites ? 1.0 : 0.0
                enabled: overlayAnimator.atTop
                visible: !overlayAnimator.atBottom && !toolBar.findInPageActive && opacity > 0.0

                header: Item {
                    width: parent.width
                    height: searchField.height
                }

                model: BookmarkModel {
                    id: bookmarkModel
                }

                onMovingChanged: if (moving) favoriteGrid.focus = true
                onLoad: overlay.loadPage(url, title)
                onNewTab: {
                    searchField.resetUrl(url)
                    // Not the best property name but functionality of opening a favorite
                    // to a new tab is exactly the same as opening new tab by typing a url.
                    searchField.enteringNewTabUrl = true
                    overlay.loadPage(url, title)
                }

                onShare: pageStack.push(Qt.resolvedUrl("../ShareLinkPage.qml"), {"link" : url, "linkTitle": title})

                Behavior on opacity { FadeAnimation {} }
            }
        }
    }

    Component {
        id: tabView
        Page {
            id: tabPage
            property int activeTabIndex
            property Item activeWebPage

            onStatusChanged: {
                if (activeWebPage && status == PageStatus.Active) {
                    webView.privateMode ? activeWebPage.grabThumbnail() : activeWebPage.grabToFile()
                }
            }

            Browser.TabView {
                model: webView.tabModel
                portrait: tabPage.isPortrait
                privateMode: webView.privateMode

                onHide: pageStack.pop()

                onPrivateModeChanged: {
                    webView.privateMode = privateMode
                    tabPage.activeTabIndex =  webView.tabModel.activeTabIndex
                    tabPage.activeWebPage = webView.contentItem
                }

                onEnterNewTabUrl: {
                    overlay.enterNewTabUrl(PageStackAction.Immediate)
                    pageStack.pop()
                }
                onActivateTab: {
                    // In case tab activated from tab view and first use exists.
                    // Let's mark first time usage guideline as done.
                    if (firstUseOverlay) {
                        firstUseOverlay.done()
                    }

                    webView.tabModel.activateTab(index)
                    pageStack.pop()
                }
                onCloseTab: {
                    webView.tabModel.remove(index)
                    if (webView.tabModel.count === 0) {
                        enterNewTabUrl()
                    }
                }

                onCloseAll: {
                    webView.tabModel.clear()
                    enterNewTabUrl()
                }

                Component.onCompleted: {
                    positionViewAtIndex(tabPage.activeTabIndex, ListView.Center)
                    window.setBrowserCover(webView.tabModel)
                }

                Component.onDestruction: window.setBrowserCover(webView.tabModel)
            }
        }
    }
}
