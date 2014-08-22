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
import "." as Browser

PanelBackground {
    id: overlay

    property alias webView: overlayAnimator.webView
    property Item browserPage
    property alias historyModel: historyList.model
    property alias toolBar: toolBar
    property alias progressBar: progressBar
    property alias animator: overlayAnimator

    property bool tabsViewVisible

    function loadPage(url, title)  {
        // let gecko figure out how to handle malformed URLs

        var searchString = url
        var pageTitle = title || ""
        if (!isNaN(searchString) && searchString.trim()) {
            searchString = "\"" + searchString.trim() + "\""
        }

        console.log("LOAD ON ENTRER:", searchString)
        if (toolBar.enteringNewTabUrl) {
            webView.tabModel.newTab(searchString, title)
        } else {
            webView.load(searchString, title)
        }
        webView.focus = true
        overlayAnimator.showChrome()
        console.log("LOOSING AT TOP BY CLICKING!")
    }

    function openNewTabView(action) {
        tabsViewVisible = false
        overlayAnimator.showOverlay(action === PageStackAction.Immediate)
    }

    y: webView.fullscreenHeight - toolBar.toolsHeight

    width: parent.width
    height: historyContainer.height

    gradient: Gradient {
        GradientStop { position: 0.0; color: Theme.rgba(Theme.highlightBackgroundColor, 0.3) }
        GradientStop { position: 1.0; color: Theme.rgba(Theme.highlightBackgroundColor, 0.0) }
    }

    // This is ugly
    onTabsViewVisibleChanged: {
        console.log("#PEREKEL: ", tabsViewVisible)
        if (tabsViewVisible) {
            webView.captureScreen()
            webView.opacity = 0.0
            overlayAnimator.hide()
        } else {
            webView.opacity = 1.0
            overlayAnimator.showChrome()
        }
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

    Browser.OverlayAnimator {
        id: overlayAnimator

        overlay: overlay
        portrait: browserPage.isPortrait
        active: Qt.application.active && browserPage.status === PageStatus.Active

        onAtBottomChanged: {
            if (atBottom) {
                searchField.resetUrl()
            }
        }
    }

    Connections {
        target: webView
        onUrlChanged: searchField.resetUrl()
    }

    Image {
        anchors.fill: parent
        source: "image://theme/graphic-gradient-edge"
    }

    Browser.ProgressBar {
        id: progressBar
        width: parent.width
        height: toolBar.toolsHeight
        visible: !firstUseOverlay
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
        enabled: !webView.fullscreenMode

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
                    overlayAnimator.showChrome()
                }
            } else {
                // Store previous end state
                if (!overlayAnimator.dragging) {
                    state = overlayAnimator.state
                }

                overlayAnimator.drag()
                console.log("Previous dragging state:", state)

            }
        }

        Item {
            id: historyContainer

            readonly property bool showFavorites: (!searchField.edited && searchField.text === webView.url || !searchField.text)

            width: parent.width
            height: toolBar.toolsHeight + historyList.height

            Browser.ToolBar {
                id: toolBar

                function saveBookmark(data) {
                    webView.bookmarkModel.addBookmark(webView.url, webView.title || webView.url,
                                                      data, browserPage.favoriteImageLoader.acceptedTouchIcon)
                    browserPage.desktopBookmarkWriter.iconFetched.disconnect(toolBar.saveBookmark)
                }

                function fetchIcon() {
                    // Async operation when loading of touch icon is needed.
                    browserPage.desktopBookmarkWriter.iconFetched.connect(toolBar.saveBookmark)
                    // TODO: would be safer to pass here an object containing url, title, icon, and icon type (touch icon)
                    // as this is async call in case we're fetching favicon.
                    browserPage.desktopBookmarkWriter.fetchIcon(browserPage.favoriteImageLoader.icon)
                }

                url: overlay.webView.url
                bookmarked: webView.bookmarkModel.count && webView.bookmarkModel.contains(webView.url)
                onShowChrome: overlayAnimator.showChrome()
                onShowOverlay: overlayAnimator.showOverlay()
                onShowTabs: overlay.tabsViewVisible = true
                onShowShare: {
                    if (overlayAnimator.secondaryTools) {
                        overlayAnimator.showChrome()
                    } else {
                        overlayAnimator.showSecondaryTools()
                    }
                }
                onLoad: overlay.loadPage(text)

                onBookmarkActivePage: {
                    var webPage = webView && webView.contentItem
                    if (webPage) {
                        toolBar.fetchIcon()
                    }
                }

                onRemoveActivePageFromBookmarks: webView.bookmarkModel.removeBookmark(webView.url)

                opacity: (overlay.y - webView.fullscreenHeight/2)  / (webView.fullscreenHeight/2 - toolBar.height)
                visible: opacity > 0.0

                secondaryToolsActive: overlayAnimator.secondaryTools
            }

            SearchField {
                id: searchField

                readonly property bool focusSearchField: overlayAnimator.atTop
                property bool edited

                function resetUrl() {
                    // Reset first text and then mark as unedited.
                    text = overlay.loadInNewTab ? "" : webView.url
                    edited = false
                }

                width: parent.width
                textLeftMargin: Theme.paddingLarge
                textRightMargin: Theme.paddingLarge

                //: Placeholder text for url typing and searching
                //% "Type URL or search"
                placeholderText: qsTrId("sailfish_browser-ph-type_url_or_search")
                EnterKey.onClicked: overlay.loadPage(text)

                background: null
                opacity: toolBar.opacity * -1.0
                visible: opacity > 0.0
                onFocusSearchFieldChanged: {
                    if (focusSearchField) {
                        searchField.forceActiveFocus()
                        searchField.selectAll()
                    } else {
                        searchField.focus = false
                    }
                }

                onTextChanged: {
                    if (!edited && text !== webView.url) {
                        edited = true
                    }
                }
            }

            Browser.HistoryList {
                id: historyList

                width: parent.width
                height: browserPage.height - searchField.height - dragArea.drag.minimumY
                search: searchField.text
                opacity: historyContainer.showFavorites ? 0.0 : 1.0
                visible: !overlayAnimator.atBottom && opacity > 0.0
                anchors.top: searchField.bottom

                onSearchChanged: if (search !== webView.url) historyModel.search(search)
                onLoad: overlay.loadPage(url, title)

                Behavior on opacity { FadeAnimation {} }
            }

            Browser.FavoriteGrid {
                id: favoriteGrid
                anchors {
                    top: toolBar.bottom
                    horizontalCenter: parent.horizontalCenter
                }

                height: historyList.height
                opacity: historyContainer.showFavorites ? 1.0 : 0.0
                visible: !overlayAnimator.atBottom && opacity > 0.0
                model: webView.bookmarkModel

                onLoad: overlay.loadPage(url, title)

                // Do we need this one???
                onNewTab: {
                    toolBar.reset("", true)
                    overlay.loadPage(url, title)
                }

                onRemoveBookmark: webView.bookmarkModel.removeBookmark(url)
                onEditBookmark: {
                    // index, url, title
                    pageStack.push(editDialog,
                                   {
                                       "url": url,
                                       "title": title,
                                       "index": index,
                                   })
                }

                onAddToLauncher: {
                    // url, title, favicon
                    pageStack.push(addToLauncher,
                                   {
                                       "url": url,
                                       "title": title
                                   })
                }

                onShare: pageStack.push(Qt.resolvedUrl("../ShareLinkPage.qml"), {"link" : url, "linkTitle": title})

                Behavior on opacity { FadeAnimation {} }
            }
        }
    }

    // TODO: Test if Loader would be make sense here.
    Browser.TabView {
        id: tabView
        opacity: tabsViewVisible ? 1.0 : 0.0
        // Animator updates only target values. Fading away set makes this invisible
        // only after animation has ended (opacity == 0.0) and tabsViewVisible takes
        // care of making this visible when animation starts.
        visible: tabsViewVisible || opacity > 0.0
        model: webView.tabModel
        parent: browserPage

        Behavior on opacity { Browser.FadeAnimation {} }

        onHide: tabsViewVisible = false
        // rename this signal. To showOverlay or similar
        onNewTab: openNewTabView()
        onActivateTab: {
            tabsViewVisible = false
            webView.tabModel.activateTab(index)
        }
        onCloseTab: {
            //showTabs = false
            console.log("All tabs closed what to do!!")
            webView.tabModel.remove(index)
        }

        // TODO: Remove these.
        //onAddBookmark: webView.bookmarkModel.addBookmark(url, title, favicon)
        //onRemoveBookmark: webView.bookmarkModel.removeBookmarks(url)
    }

    Component {
        id: editDialog
        Browser.BookmarkEditDialog {
            onAccepted: webView.bookmarkModel.editBookmark(index, editedUrl, editedTitle)
        }
    }

    Component {
        id: addToLauncher
        Browser.BookmarkEditDialog {
            //: Title of the "Add to launcher" dialog.
            //% "Add to launcher"
            title: qsTrId("sailfish_browser-he-add_bookmark_to_launcher")
            canAccept: editedUrl !== "" && editedTitle !== ""
            onAccepted: {
                // TODO: This should use directly the icon the the bookmark.
                var icon = browserPage.favoriteImageLoader.icon
                browserPage.desktopBookmarkWriter.link = editedUrl
                browserPage.desktopBookmarkWriter.title = editedTitle
                browserPage.desktopBookmarkWriter.icon = icon
                browserPage.desktopBookmarkWriter.save()
            }
        }
    }
}
