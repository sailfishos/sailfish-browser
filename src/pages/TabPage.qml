/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import QtGraphicalEffects 1.0
import Sailfish.Browser 1.0
import Sailfish.Silica 1.0
import Sailfish.Silica.private 1.0
import "components" as Browser

Page {
    id: page

    property BrowserPage browserPage
    // focus to input field on opening
    property bool initialSearchFocus
    property bool newTab
    property bool historyVisible: initialSearchFocus

    property Item historyHeader
    property Item favoriteHeader

    property bool initialized
    property string _search

    function load(url, title) {
        if (page.newTab) {
            browserPage.tabs.newTab(url, title)
        } else {
            browserPage.load(url, title)
        }
        pageStack.pop(browserPage)
    }

    function activateTab(index) {
        browserPage.tabs.activateTab(index)
        pageStack.pop(browserPage)
    }

    onStatusChanged: {
        if (status === PageStatus.Active) {
            initialized = true
        }
    }

    backNavigation: browserPage.tabs.count > 0 && browserPage.url != ""
    states: [
        State {
            when: page.status < PageStatus.Active && !initialized
        },
        State {
            name: "historylist"
            when: historyVisible
        },
        State {
            name: "favoritelist"
            when: !historyVisible
        }
    ]

    transitions: [
        Transition {
            to: "favoritelist"
            ParallelAnimation {
                SequentialAnimation {
                    PropertyAction { target: commonHeader; property: "parent"; value: page }
                    FadeAnimation { target: favoriteList; to: 1.0 }
                    PropertyAction { target: commonHeader; property: "parent"; value: page.favoriteHeader }
                    ScriptAction { script: {
                            if (!favoriteList.model) {
                                favoriteList.model = browserPage.favorites
                            }
                            favoriteList.positionViewAtBeginning()
                        }
                    }
                }
                FadeAnimation { target: historyList; to: 0.0 }
            }
        },
        Transition {
            to: "historylist"
            ParallelAnimation {
                SequentialAnimation {
                    PropertyAction { target: commonHeader; property: "parent"; value: page }
                    FadeAnimation { target: historyList; to: 1.0 }
                    PropertyAction { target: commonHeader; property: "parent"; value: page.historyHeader }
                    PropertyAction { target: page.historyHeader; property: "opacity"; value: 1.0 }
                    ScriptAction { script: {
                            if (!historyList.model) {
                                historyList.model = browserPage.history
                            }

                            historyList.positionViewAtBeginning()
                            focusTimer.restart()
                        }
                    }
                }
                FadeAnimation { target: favoriteList; to: 0.0 }
            }
        }
    ]

    Browser.HistoryList {
        id: historyList

        visible: opacity > 0.0

        header: Item {
            id: historyHeader
            width: commonHeader.width
            height: commonHeader.height + historySectionHeader.height
            opacity: 0.0

            Behavior on opacity { FadeAnimation {} }

            SectionHeader {
                id: historySectionHeader
                //: Section header for history items
                //% "History"
                text: qsTrId("sailfish_browser-he-history")
                anchors.bottom: historyHeader.bottom
            }

            Component.onCompleted: page.historyHeader = historyHeader
        }
        search: _search

        onLoad: page.load(url, title)

        Browser.TabPageMenu {
            active: initialized
            visible: browserPage.tabs.count > 0 && !page.newTab
            shareEnabled: browserPage.url == _search
            browserPage: page.browserPage
            flickable: historyList
        }
    }

    Browser.FavoriteList {
        id: favoriteList

        visible: opacity > 0.0
        opacity: initialSearchFocus ? 0.0 : 1.0

        header: Item {
            id: favoriteHeader
            width: commonHeader.width
            height: (page.newTab ? commonHeader.height : commonHeader.height + tabsGrid.height) + favoriteSectionHeader.height

            VisibilityCull {
                target: tabsGrid
            }

            Grid {
                id: tabsGrid

                // Fill initial view immediately
                property int loadIndex: columns * Math.round(page.height / (page.width / columns))

                visible: !page.newTab
                columns: page.isPortrait ? 2 : 4
                rows: Math.ceil((browserPage.tabs.count - 1) / columns)
                height: rows > 0 ? rows * (page.width / tabsGrid.columns) : 0
                anchors.bottom: favoriteSectionHeader.top
                move: Transition {
                    NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad; duration: 200 }
                }
                add: Transition {
                    AddAnimation {}
                }

                Repeater {
                    model: browserPage.tabs

                    Browser.TabItem {
                        width: page.width/tabsGrid.columns
                        height: width
                        active: index < tabsGrid.loadIndex || initialized
                        asynchronous: index >= tabsGrid.loadIndex
                        // activateTab doesn't work inside delagate because this tab is removed (deleted)
                        // from the model and old active tab pushed to first.
                        onClicked: activateTab(model.index)
                    }
                }
                Behavior on height {
                    NumberAnimation { easing.type: Easing.InOutQuad; duration: 200 }
                }
            }

            SectionHeader {
                id: favoriteSectionHeader
                //: Section header for favorites
                //% "Favorites"
                text: qsTrId("sailfish_browser-he-favorites")
                anchors.bottom: favoriteHeader.bottom
            }

            Component.onCompleted: page.favoriteHeader = favoriteHeader
        }
        hasContextMenu: !page.newTab

        onLoad: {
            if (newTab) page.newTab = true
            page.load(url, title)
        }

        onEdit: {
            pageStack.push(editDialog,
                           {
                               "url": url,
                               "title": title,
                               "index": index,
                           })
        }

        onRemoveBookmark: browserPage.favorites.removeBookmark(url)

        Browser.TabPageMenu {
            active: initialized
            visible: browserPage.tabs.count > 0 && !page.newTab
            shareEnabled: browserPage.url == _search
            browserPage: page.browserPage
            flickable: favoriteList

            onAddToLauncher: {
                pageStack.push(addToLauncher,
                               {
                                   "url": browserPage.url,
                                   "title": browserPage.title
                               })
                browserPage.imageLoader.source = browserPage.webView.favicon
            }
        }
    }

    Column {
        id: commonHeader
        width: page.width

        PageHeader {
            id: pageHeader
            //% "New tab"
            title: (browserPage.tabs.count == 0 || newTab) ? qsTrId("sailfish_browser-la-new_tab") :
                                                           //% "Search"
                                                           (historyVisible ? qsTrId("sailfish_browser-la-search")
                                                                             //: Header at the Tab page
                                                                             //% "Tabs"
                                                                           : qsTrId("sailfish_browser-he-tabs"))
            visible: page.isPortrait
            height: Theme.itemSizeLarge
        }

        Rectangle {
            id: headerContent
            width: parent.width
            height: Screen.width / 2
            color: Theme.rgba(Theme.highlightColor, 0.1)

            Image {
                id: headerThumbnail
                width: height
                height: parent.height
                sourceSize.height: height
                anchors.right: parent.right
                asynchronous: true
                source: browserPage.thumbnailPath
                cache: false
                visible: status !== Image.Error && source !== "" && !page.newTab
            }

            OpacityRampEffect {
                opacity: 0.6
                sourceItem: headerThumbnail
                slope: 1.0
                offset: 0.0
                direction: OpacityRamp.RightToLeft
            }

            Column {
                id: urlColumn

                property int indicatorWidth: window.indicatorParentItem.childrenRect.width

                x: page.isPortrait ?  0 : indicatorWidth + Theme.paddingLarge
                anchors.topMargin: Theme.itemSizeLarge / 2 - titleLabel.height / 2

                states: [
                    State {
                        when: page.isLandscape
                        AnchorChanges {
                            target: urlColumn
                            anchors.bottom: undefined
                            anchors.top: headerContent.top
                        }
                    },
                    State {
                        when: page.isPortrait
                        AnchorChanges {
                            target: urlColumn
                            anchors.bottom: headerContent.bottom
                            anchors.top: undefined
                        }
                    }
                ]

                Label {
                    id: titleLabel
                    x: Theme.paddingLarge
                    // Reuse new tab label (la-new_tab)
                    text: (browserPage.tabs.count == 0 || newTab) ? qsTrId("sailfish_browser-la-new_tab") : (browserPage.url == _search ? browserPage.title : "")
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeSmall
                    width: textFieldLoader.width - x - Theme.paddingMedium
                    truncationMode: TruncationMode.Fade
                    opacity: 0.0

                    Behavior on opacity { FadeAnimation {} }
                }

                Loader {
                    id: textFieldLoader

                    property bool searchFieldFocused: item ? item.focus : false
                    property bool focusOnceLoaded

                    asynchronous: true
                    width: page.isPortrait ? page.width - (closeActiveTabButton.visible ? closeActiveTabButton.width : 0)
                                           : page.width - urlColumn.x - Theme.paddingMedium
                    height: Theme.itemSizeMedium
                    active: initialized

                    sourceComponent: TextField {
                        id: searchField

                        function focusAndSelect() {
                            forceActiveFocus()
                            selectAll()
                            _updateFlickables()
                            historyList.currentIndex = -1
                        }

                        width: parent.width
                        // Handle initially newTab state. Currently newTab initially
                        // true when triggering new tab cover action.
                        text: newTab ? "" : browserPage.url

                        //: Placeholder for the search/address field
                        //% "Search or Address"
                        placeholderText: qsTrId("sailfish_browser-ph-search_or_url")
                        color: searchField.focus ? Theme.highlightColor : Theme.primaryColor
                        focusOutBehavior: FocusBehavior.KeepFocus
                        inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
                        opacity: 0.0

                        label: {
                            if (text.length === 0) return ""

                            if (searchField.activeFocus || text !== browserPage.url) {
                                // Reuse search label
                                return qsTrId("sailfish_browser-la-search")
                            }

                            //: Active browser tab.
                            //% "Active Tab"
                            var activeTab = qsTrId("sailfish_browser-la-active-tab")

                            if (text === browserPage.url && browserPage.viewLoading) {
                                //: Current browser page loading.
                                //% "Loading"
                                return activeTab + " • " + qsTrId("sailfish_browser-la-loading")
                            } else {
                                //: Current browser page loaded.
                                //% "Done"
                                return activeTab + " • " + qsTrId("sailfish_browser-la-done")
                            }
                        }

                        EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                        EnterKey.onClicked: {
                            Qt.inputMethod.hide()
                            // let gecko figure out how to handle malformed URLs
                            page.load(searchField.text)
                        }

                        onTextChanged: if (text != browserPage.url) browserPage.history.search(text)
                        onFocusChanged: historyVisible = focus

                        Behavior on opacity { FadeAnimation {} }
                        Binding { target: page; property: "_search"; value: searchField.text }
                    }

                    // These steps cannot be done in TextField's Component.onCompleted as
                    // item of the Loader might be still null.
                    onLoaded: {
                        item.opacity = 1.0
                        titleLabel.opacity = 1.0
                        if (textFieldLoader.focusOnceLoaded) {
                            focusTimer.restart()
                        }
                    }

                    Timer {
                        id: focusTimer
                        interval: 1
                        onTriggered: {
                            if (textFieldLoader.item) {
                                textFieldLoader.item.focusAndSelect()
                            } else {
                                textFieldLoader.focusOnceLoaded = true
                            }
                        }
                    }
                }

                Connections {
                    target: page
                    onStatusChanged: {
                        // break binding when pushed to stick with proper value for this depth of pagestack
                        if (status === PageStatus.Active && window.indicatorParentItem.childrenRect.width == urlColumn.indicatorWidth)
                            urlColumn.indicatorWidth = window.indicatorParentItem.childrenRect.width
                    }
                }
            }

            Browser.CloseTabButton {
                id: closeActiveTabButton
                visible: browserPage.tabs.count > 0 && !page.newTab && !textFieldLoader.searchFieldFocus
                closeActiveTab: true
            }
        }
    }

    Component {
        id: editDialog
        Browser.BookmarkEditDialog {
            onAccepted: favoriteList.model.editBookmark(index, editedUrl, editedTitle)
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
