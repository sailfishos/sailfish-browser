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

    property bool tabsVisible
    property bool loadInNewTab

    function loadPage(url, title)  {
        // let gecko figure out how to handle malformed URLs

        var searchString = url
        var pageTitle = title || ""
        if (!isNaN(searchString) && searchString.trim()) {
            searchString = "\"" + searchString.trim() + "\""
        }

        console.log("LOAD ON ENTRER:", searchString)
        if (loadInNewTab) {
            webView.tabModel.newTab(searchString, title)
            loadInNewTab = false
        } else {
            webView.load(searchString, title)
        }
        webView.focus = true
        overlayAnimator.showChrome()
        console.log("LOOSING AT TOP BY CLICKING!")
    }

    function openNewTabView(action) {
        tabsVisible = false
        overlayTabs.currentIndex = 1 // Url entry
        loadInNewTab = true
        overlayAnimator.showOverlay(action === PageStackAction.Immediate)
    }

    y: webView.fullscreenHeight - toolBar.height
    width: parent.width
    height: historyContainer.height

    gradient: Gradient {
        GradientStop { position: 0.0; color: Theme.rgba(Theme.highlightBackgroundColor, 0.3) }
        GradientStop { position: 1.0; color: Theme.rgba(Theme.highlightBackgroundColor, 0.0) }
    }

    // This is ugly
    onTabsVisibleChanged: {
        console.log("#PEREKEL: ", tabsVisible)
        if (tabsVisible) {
            webView.captureScreen()
            webView.opacity = 0.0
            overlayAnimator.hide()
        } else {
            webView.opacity = 1.0
            overlayAnimator.showChrome()
        }
    }

    Browser.OverlayAnimator {
        id: overlayAnimator

        overlay: overlay
        portrait: browserPage.isPortrait
        active: Qt.application.active && browserPage.status === PageStatus.Active
    }

    Image {
        anchors.fill: parent
        source: "image://theme/graphic-gradient-edge"
    }

    Browser.ProgressBar {
        id: progressBar
        width: parent.width
        height: toolBar.height
        visible: !firstUseOverlay
        opacity: webView.loading ? 1.0 : 0.0
        progress: webView.loadProgress / 100.0
    }

    MouseArea {
        id: dragArea

        property int dragThreshold: state === "fullscreenOverlay" ? toolBar.height * 1.5 : (webView.fullscreenHeight - toolBar.height * 2)

        width: parent.width
        height: historyContainer.height

        opacity: !overlay.tabsVisible ? 1.0 : 0.0
        visible: opacity > 0.0
        enabled: !webView.fullscreenMode
        drag.target: overlay
        drag.filterChildren: true
        drag.axis: Drag.YAxis
        drag.minimumY: browserPage.isPortrait ? toolBar.height : 0
        drag.maximumY: browserPage.isPortrait ? webView.fullscreenHeight - toolBar.height : webView.fullscreenHeight

        drag.onActiveChanged: {
            console.log("Drag active changed")
            if (!drag.active) {
                searchField.visible = true
                if (overlay.y < dragThreshold) {
                    overlayAnimator.state = "fullscreenOverlay"
                } else {
                    overlayAnimator.state = "chromeVisible"
                }
            } else {
                // Hack to make sure VKB does not come up when swiping down from history/download tabs
                if (overlayTabs.currentIndex !== 1) {
                    searchField.visible = false
                }
                // Store previous end state
                if (overlayAnimator.state !== "draggingOverlay") {
                    state = overlayAnimator.state
                }

                //                if (overlayAnimator.atTop && webView.inputPanelVisible) {
                //                    Qt.inputMethod.hide()
                //                    webView.focus = true
                //                }

                overlayAnimator.state = "draggingOverlay"
                console.log("Previous dragging state:", state)

            }
        }

        Behavior on opacity { Browser.FadeAnimation {} }

        Browser.ToolBar {
            id: toolBar
            visible: !overlayTabs.visible && !searchBar.visible
            opacity: (overlay.y - webView.fullscreenHeight/2)  / (webView.fullscreenHeight/2 - toolBar.height)

            title: webView.url
            onShowChrome: overlayAnimator.showChrome()
            onShowOverlay: {
                overlayAnimator.showOverlay()
                overlayTabs.currentIndex = 1 // On tap switch automatically to url input
            }
            onShowShare: pageStack.push(Qt.resolvedUrl("../ShareLinkPage.qml"), {"link" : webView.url, "linkTitle": webView.title})
            onShowTabs: overlay.tabsVisible = true
            busy: webView.loading
        }

        Browser.SearchBar {
            id: searchBar
            opacity: toolBar.opacity
            visible: false
        }

        SlideshowView {
            id: overlayTabs

            readonly property bool focusSearchField: currentIndex == 1 && overlayAnimator.atTop
            readonly property bool focusSearchInPage: currentIndex == 2 && overlayAnimator.atTop

            width: parent.width
            height: toolBar.height + historyList.height
            visible: overlay.y < webView.fullscreenHeight/2
            opacity: (webView.fullscreenHeight/2 - overlay.y ) / (webView.fullscreenHeight/2 - toolBar.height)

            highlightRangeMode: ListView.StrictlyEnforceRange
            snapMode: ListView.SnapOneItem
           // orientation: ListView.Horizontal

            currentIndex: 1

            onCurrentIndexChanged: {
                if (currentIndex == 0) {
                    historyModel.search("")
                } else if (currentIndex == 1 && searchField.text !== webView.url) {
                    historyModel.search(searchField.text)
                }
            }

            onFocusSearchFieldChanged: {
                if (focusSearchField) {
                    searchField.forceActiveFocus()
                    searchField.selectAll()
                } else {
                    searchField.focus = false
                }
            }

            onFocusSearchInPageChanged: {
                if (focusSearchInPage) {
                    findInPage.findInPageField.forceActiveFocus()
                } else {
                    findInPage.findInPageField.focus = false
                }
            }

            model: VisualItemModel {
                Browser.HistoryList {
                    width: parent.width
                    height: browserPage.height - toolBar.height - Theme.paddingMedium
                    y: Theme.paddingMedium
                    model: historyModel

                    onLoad: overlay.loadPage(url, title)
                }

                Item {
                    id: historyContainer
                    width: parent.width
                    height: toolBar.height + historyList.height

                    Label {
                        id: titleLabel
                        anchors.top: parent.top
                        anchors.topMargin: Theme.paddingSmall
                        x: Theme.paddingLarge

                        // Reuse new tab label (la-new_tab)
                        text: (browserPage.tabs.count == 0) ? qsTrId("sailfish_browser-la-new_tab") : (webView.url == searchField.text ? webView.title : "")
                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeSmall
                        width: parent.width - x - Theme.paddingMedium
                        truncationMode: TruncationMode.Fade
                        opacity: 1.0

                        Behavior on opacity { FadeAnimation {} }
                    }

                    TextField {
                        id: searchField
                        width: parent.width
                        text: overlay.loadInNewTab ? "" : webView.url

                        label: "Search or type URL"

                        EnterKey.onClicked: overlay.loadPage(text)
                        anchors.top: titleLabel.bottom
                    }

                    Browser.HistoryList {
                        id: historyList

                        width: parent.width
                        height: browserPage.height - toolBar.height - dragArea.drag.minimumY
                        search: searchField.text
                        opacity: searchField.text !== webView.url && searchField.text ? 1.0 : 0.0
                        visible: !overlayAnimator.atBottom && opacity > 0.0
                        anchors.top: searchField.bottom

                        onSearchChanged: if (search !== webView.url) historyModel.search(search)
                        onLoad: overlay.loadPage(url, title)

                        Behavior on opacity { Browser.FadeAnimation {} }
                    }

                    Browser.FavoriteGrid {
                        id: favoriteGrid
                        anchors {
                            top: searchField.bottom
                            horizontalCenter: parent.horizontalCenter
                        }

                        height: historyList.height
                        opacity: searchField.text == webView.url || searchField.text == "" ? 1.0 : 0.0
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
                            browserPage.imageLoader.source = favicon
                        }

                        onShare: pageStack.push(Qt.resolvedUrl("../ShareLinkPage.qml"), {"link" : url, "linkTitle": title})

                        Behavior on opacity { Browser.FadeAnimation {} }
                    }
                }

                FindInPageView {
                    id: findInPage
                    width: overlayTabs.width
                    height: overlayTabs.height

                }

                DownloadsView {
                    width: overlayTabs.width
                    height: overlayTabs.height
                }
            }
        }
    }

    Browser.OverlayTabBar {
        visible: !tabsVisible && !overlayAnimator.atBottom
        opacity: overlayTabs.opacity
        anchors.bottom: overlay.top
        anchors.bottomMargin: Theme.paddingSmall
        currentIndex: overlayTabs.currentIndex
        onSelectTab: overlayTabs.currentIndex = index
    }

    // TODO: Test if Loader would be make sense here.
    Browser.TabView {
        id: tabView
        opacity: tabsVisible ? 1.0 : 0.0
        visible: opacity > 0.0
        model: webView.tabModel
        parent: browserPage

        Behavior on opacity { Browser.FadeAnimation {} }

        onHide: tabsVisible = false
        // rename this signal. To showOverlay or similar
        onNewTab: openNewTabView()
        onActivateTab: {
            tabsVisible = false
            webView.tabModel.activateTab(index)
        }
        onCloseTab: {
            //tabsVisible = false
            console.log("All tabs closed what to do!!")
            webView.tabModel.remove(index)
        }

        onAddBookmark: webView.bookmarkModel.addBookmark(url, title, favicon)
        onRemoveBookmark: webView.bookmarkModel.removeBookmarks(url)
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
                var icon = browserPage.imageLoader.source
                var minimumIconSize = browserPage.desktopBookmarkWriter.minimumIconSize
                if (browserPage.imageLoader.width < minimumIconSize || browserPage.imageLoader.height < minimumIconSize) {
                    if (!browserPage.desktopBookmarkWriter.exists(browserPage.thumbnailPath)) {
                        icon = ""
                    } else {
                        icon = browserPage.thumbnailPath
                    }
                }
                browserPage.desktopBookmarkWriter.link = editedUrl
                browserPage.desktopBookmarkWriter.title = editedTitle
                browserPage.desktopBookmarkWriter.icon = icon
                browserPage.desktopBookmarkWriter.save()
                browserPage.imageLoader.source = ""
            }
        }
    }
}
