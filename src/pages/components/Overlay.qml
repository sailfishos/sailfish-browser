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

    y: webView.fullscreenHeight - toolBar.height
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

        opacity: !overlay.tabsViewVisible ? 1.0 : 0.0
        visible: opacity > 0.0
        enabled: !webView.fullscreenMode && overlayAnimator.state !== "doubleToolBar"
        drag.target: overlay
        drag.filterChildren: true
        drag.axis: Drag.YAxis
        drag.minimumY: browserPage.isPortrait ? toolBar.height : 0
        drag.maximumY: browserPage.isPortrait ? webView.fullscreenHeight - toolBar.height : webView.fullscreenHeight

        drag.onActiveChanged: {
            if (!drag.active) {
                if (overlay.y < dragThreshold) {
                    overlayAnimator.state = "fullscreenOverlay"
                } else {
                    overlayAnimator.state = "chromeVisible"
                }
            } else {
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

        SecondaryBar {
            visible: opacity > 0.0
            opacity: overlayAnimator.state == "doubleToolBar" ? 1.0 : 0.0

            Behavior on opacity { FadeAnimation {} }

            clip: true
            height: dragArea.drag.maximumY - overlay.y
        }




        Item {
            id: historyContainer
            width: parent.width
            height: toolBar.height + historyList.height
            property bool favoritesVisible: !searchField.text || searchField.text == webView.url

            Browser.ToolBar {
                id: toolBar

                y: !doubleHeight ? 0 : dragArea.drag.maximumY - overlay.y

                title: overlay.webView.url
                onShowChrome: overlayAnimator.showChrome()
                onShowOverlay: overlayAnimator.showOverlay()
                onShowTabs: overlay.tabsViewVisible = true
                onShowShare: doubleHeight = !doubleHeight
                onLoad: overlay.loadPage(text)
                opacity: (overlay.y - webView.fullscreenHeight/2)  / (webView.fullscreenHeight/2 - toolBar.height)
                visible: opacity > 0

                doubleHeight: overlayAnimator.state == "doubleToolBar"
                onDoubleHeightChanged: {
                    if (doubleHeight) {
                        overlayAnimator.showSecondaryTools()
                    } else {
                        overlayAnimator.showChrome()
                    }
                }
            }

            TextField {
                id: searchField
                width: parent.width
                text: overlay.loadInNewTab ? "" : webView.url
                placeholderText: "Type URL or search"
                EnterKey.onClicked: overlay.loadPage(text)
                background: Item {}

                readonly property bool focusSearchField: overlayAnimator.atTop

                opacity: (webView.fullscreenHeight/2 - overlay.y ) / (webView.fullscreenHeight/2 - toolBar.height)
                visible: opacity > 0


                onFocusSearchFieldChanged: {
                    if (focusSearchField) {
                        searchField.forceActiveFocus()
                        searchField.selectAll()
                    } else {
                        searchField.focus = false
                    }
                }
            }

            Browser.HistoryList {
                id: historyList

                width: parent.width
                height: browserPage.height - toolBar.height - dragArea.drag.minimumY
                search: searchField.text
                opacity: historyContainer.favoritesVisible ? 0.0 : 1.0
                visible: !overlayAnimator.atBottom && opacity > 0.0
                anchors.top: searchField.bottom

                onSearchChanged: if (search !== webView.url) historyModel.search(search)
                onLoad: overlay.loadPage(url, title)

                Behavior on opacity { Browser.FadeAnimation {} }
            }

            Browser.FavoriteGrid {
                id: favoriteGrid
                anchors {
                    top: toolBar.bottom
                    horizontalCenter: parent.horizontalCenter
                }

                height: historyList.height
                opacity: historyContainer.favoritesVisible ? 1.0 : 0.0
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
    }

    // TODO: Test if Loader would be make sense here.
    Browser.TabView {
        id: tabView
        opacity: tabsViewVisible ? 1.0 : 0.0
        visible: opacity > 0.0
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
