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

    property bool _editing
    property string _search

    function load(url, title) {
        if (page.newTab) {
            browserPage.newTab(url, true, title)
        } else {
            browserPage.load(url, title)
        }
        pageStack.pop(browserPage)
    }

    backNavigation: browserPage.tabs.count > 0 && browserPage.url != ""

    property bool historyVisible: _editing || initialSearchFocus
    property Item historyHeader
    property Item favoriteHeader

    states: [
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
                    FadeAnimation { target: favoriteList; from: 0.0; to: 1.0}
                    PropertyAction { target: commonHeader; property: "parent"; value: page.favoriteHeader }
                }
                FadeAnimation { target: historyList; from: 1.0; to: 0.0 }
            }
        },
        Transition {
            to: "historylist"
            ParallelAnimation {
                SequentialAnimation {
                    PropertyAction { target: commonHeader; property: "parent"; value: page }
                    FadeAnimation { target: historyList; from: 0.0; to: 1.0}
                    PropertyAction { target: commonHeader; property: "parent"; value: page.historyHeader }
                    ScriptAction { script: { focusTimer.running = true; page.initialSearchFocus = false } }
                }
                FadeAnimation { target: favoriteList; from: 1.0; to: 0.0 }
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

            // common header parented
            onChildrenChanged: historyList.contentY = -historyHeader.height

            SectionHeader {
                id: historySectionHeader
                //: Section header for history items
                //% "History"
                text: qsTrId("sailfish_browser-he-history")
                anchors.bottom: historyHeader.bottom
            }

            Component.onCompleted: page.historyHeader = historyHeader
        }
        model: browserPage.history
        search: _search

        onLoad: page.load(url, title)

        Browser.TabPageMenu {
            visible: browserPage.tabs.count > 0 && !page.newTab
            shareEnabled: browserPage.currentTab.url == _search
            browserPage: page.browserPage
        }
    }

    Browser.FavoriteList {
        id: favoriteList

        visible: opacity > 0.0

        header: Item {
            id: favoriteHeader
            width: commonHeader.width
            height: (page.newTab ? commonHeader.height : commonHeader.height + tabsGrid.height) + favoriteSectionHeader.height

            Component.onCompleted: page.favoriteHeader = favoriteHeader

            VisibilityCull {
                target: tabsGrid
            }

            Grid {
                id: tabsGrid
                visible: !page.newTab
                columns: page.isPortrait ? 2 : 4
                rows: Math.ceil((browserPage.tabs.count - 1) / columns)
                height: rows > 0 ? rows * (page.width / tabsGrid.columns) : 0
                anchors.bottom: favoriteSectionHeader.top
                move: Transition {
                    NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad; duration: 200 }
                }

                Repeater {
                    model: browserPage.tabs
                    Browser.TabItem {
                        width: page.width/tabsGrid.columns
                        height: width

                        onClicked: browserPage.loadTab(model.index, model.url, model.title)
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
        }
        model: browserPage.favorites
        hasContextMenu: !page.newTab

        onLoad: {
            if (newTab) page.newTab = true
            page.load(url, title)
        }

        onRemoveBookmark: browserPage.favorites.removeBookmark(url)

        Browser.TabPageMenu {
            visible: browserPage.tabs.count > 0 && !page.newTab
            shareEnabled: browserPage.currentTab.url == _search
            browserPage: page.browserPage
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
            height: visible ? Theme.itemSizeLarge : 0
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
                source: browserPage.currentTab.thumbnailPath
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
                    text: (browserPage.tabs.count == 0 || newTab) ? qsTrId("sailfish_browser-la-new_tab") : (browserPage.currentTab.url == _search ? browserPage.currentTab.title : "")
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeSmall
                    width: searchField.width - x - Theme.paddingMedium
                    truncationMode: TruncationMode.Fade
                }
                TextField {
                    id: searchField

                    width: page.isPortrait ? page.width - (closeActiveTabButton.visible ? closeActiveTabButton.width : 0)
                                           : page.width - urlColumn.x - Theme.paddingMedium
                    // Handle initially newTab state. Currently newTab initially
                    // true when triggering new tab cover action.
                    text: newTab ? "" : browserPage.currentTab.url

                    //: Placeholder for the search/address field
                    //% "Search or Address"
                    placeholderText: qsTrId("sailfish_browser-ph-search_or_url")
                    color: searchField.focus ? Theme.highlightColor : Theme.primaryColor
                    focusOutBehavior: FocusBehavior.KeepFocus
                    inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly


                    label: {
                        if (text.length === 0) return ""

                        if (searchField.activeFocus || text !== browserPage.currentTab.url) {
                            // Reuse search label
                            return qsTrId("sailfish_browser-la-search")
                        }

                        //: Active browser tab.
                        //% "Active Tab"
                        var activeTab = qsTrId("sailfish_browser-la-active-tab")

                        if (text === browserPage.currentTab.url && browserPage.viewLoading) {
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

                    onTextChanged: if (text != browserPage.currentTab.url) browserPage.history.search(text)

                    Binding { target: page; property: "_search"; value: searchField.text }
                    Binding { target: page; property: "_editing"; value: searchField.focus }

                    Timer {
                        id: focusTimer
                        interval: 1
                        onTriggered: {
                            searchField.forceActiveFocus()
                            searchField.selectAll()
                            searchField._updateFlickables()
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
                visible: browserPage.currentTab.valid && !page.newTab && !searchField.focus
                closeActiveTab: true
            }
        }
    }
}
