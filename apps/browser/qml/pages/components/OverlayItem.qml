/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC.
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
import Sailfish.WebView.Controls 1.0
import Sailfish.WebEngine 1.0
import com.jolla.settings.system 1.0
import "." as Browser
import "../../shared" as Shared

SilicaFlickable {
    property bool active
    property QtObject webView
    property Component tabView
    property Item browserPage
    property alias historyItem: historyList
    property alias favoriteItem: favoriteGrid
    property alias historyModel: historyList.model
    property alias bookmarkModel: bookmarkModel
    property Item toolBar
    property Item overlayAnimator
    property alias progressBar: progressBar
    property var dragArea
    property alias searchField: searchField
    readonly property alias enteringNewTabUrl: searchField.enteringNewTabUrl
    property string enteredUrl

    property real _overlayHeight: browserPage.isPortrait ? toolBar.rowHeight : 0
    property bool _showFindInPage
    property bool _showUrlEntry
    readonly property bool _topGap: _showUrlEntry || _showFindInPage

    readonly property bool showFavorites: !overlayAnimator.atBottom && !toolBar.findInPageActive && _showUrlEntry
    readonly property bool showHistoryList: showFavorites && (searchField.edited
                                                              && searchField.text !== webView.url
                                                              && searchField.text)
    readonly property bool showHistoryButton: showFavorites && (!searchField.edited
                                                                && searchField.text === webView.url
                                                                || !searchField.text)

    property int panelSize: favoriteGrid.contextMenu && favoriteGrid.contextMenu.active ? 0 : virtualKeyboardObserver.panelSize
    width: parent.width
    height: webView.tabModel.count || webView.privateMode ? browserPage.height - _overlayHeight - panelSize : browserPage.height - panelSize

    contentHeight: content.height

    // Clip only when content has been moved and we're at top or animating downwards.
    clip: (overlayAnimator.atTop ||
           overlayAnimator.direction === "downwards" ||
           overlayAnimator.direction === "upwards" ||
           favoriteGrid.opacity != 0.0 ||
           historyButton.opacity != 0.0 ||
           historyList.opacity != 0.0)
    
    BookmarkModel {
        id: bookmarkModel
        activeUrl: toolBar.url
    }

    Shared.ProgressBar {
        id: progressBar
        width: parent.width
        height: toolBar.rowHeight
        visible: !searchField.enteringNewTabUrl
        opacity: webView.loading ? 1.0 : 0.0
        progress: webView.loadProgress / 100.0
    }

    Column {
        id: content
        width: parent.width

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

            // On top of HistoryList and FavoriteGrid
            z: 1
            height: overlay.toolBar.rowHeight
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
            background: Rectangle{
                anchors.fill: parent
                color: Theme.primaryColor
                opacity: 0.1
            }

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

            onYChanged: {
                if (y < 0) {
                    dragArea.moved = true
                }
            }

            onRequestingFocusChanged: {
                if (requestingFocus && webView.tabModel.count) {
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

        OverlayListItem {
            id: historyButton
            height: overlay.toolBar.rowHeight
            iconWidth: overlay.toolBar.iconWidth
            horizontalOffset: overlay.toolBar.horizontalOffset
            text: qsTrId("sailfish_browser-la-history")
            iconSource: "image://theme/icon-m-history"
            opacity: visible && toolBar.opacity < 0.9 ? 1.0 : 0.0
            enabled: overlayAnimator.atTop
            visible: showHistoryButton
            onClicked: {
                var historyPage = pageStack.push("../HistoryPage.qml", { model: historyModel })
                historyPage.loadPage.connect(loadPage)
            }
            Behavior on opacity { FadeAnimator {} }
        }

        Browser.FavoriteGrid {
            id: favoriteGrid
            height: contentHeight
            opacity: visible && toolBar.opacity < 0.9 ? 1.0 : 0.0
            enabled: overlayAnimator.atTop
            visible: showFavorites
            _quickScrollRightMargin: -(browserPage.width - width) / 2
            model: BookmarkFilterModel {
                id: bookmarkFilterModel
                sourceModel: bookmarkModel
                search: searchField.text !== webView.url ? searchField.text : ""
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

        Browser.HistoryList {
            id: historyList

            width: parent.width
            height: contentHeight
            search: searchField.text
            showDeleteButton: true
            showFavicon: true
            opacity: visible && toolBar.opacity < 0.9 ? 1.0 : 0.0
            enabled: overlayAnimator.atTop
            visible: showHistoryList
            onMovingChanged: if (moving) historyList.focus = true
            onSearchChanged: if (search !== webView.url) historyModel.search(search)
            onLoad: {
                historyList.focus = true
                overlay.loadPage(url)
            }
            Behavior on opacity { FadeAnimator {} }
        }
    }
}

