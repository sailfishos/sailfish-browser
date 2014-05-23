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
    property bool newTab

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
            if (!favoriteList.model) {
                favoriteList.model = browserPage.favorites
            }
        }
    }

    backNavigation: browserPage.tabs.count > 0 && browserPage.url != ""

    Browser.FavoriteList {
        id: favoriteList

        header: Column {
            width: page.width

            PageHeader {
                id: pageHeader
                //% "New tab"
                title: (browserPage.tabs.count == 0 || newTab) ? qsTrId("sailfish_browser-la-new_tab") :
                                                                 //: Header at the Tab page
                                                                 //% "Tabs"
                                                                 qsTrId("sailfish_browser-he-tabs")
                height: Theme.itemSizeLarge
            }

            Loader {
                id: textFieldLoader

                property bool searchFieldFocused: item ? item.focus : false
                property bool focusOnceLoaded

                asynchronous: true
                width: page.width
                height: Theme.itemSizeMedium
                visible: newTab
                active: initialized && newTab

                sourceComponent: TextField {
                    id: searchField

                    function focusAndSelect() {
                        forceActiveFocus()
                        selectAll()
                    }

                    width: parent.width

                    // Reuse translation
                    placeholderText: qsTrId("sailfish_browser-la-new_tab")
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

                    Behavior on opacity { FadeAnimation {} }
                    Binding { target: page; property: "_search"; value: searchField.text }
                }

                // These steps cannot be done in TextField's Component.onCompleted as
                // item of the Loader might be still null.
                onLoaded: {
                    item.opacity = 1.0
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

            VisibilityCull {
                target: tabsGrid
            }

            Grid {
                id: tabsGrid

                // Fill initial view immediately
                property int loadIndex: columns * Math.round(page.height / (page.width / columns))

                //visible: !page.newTab
                columns: page.isPortrait ? 2 : 4
                rows: Math.ceil((browserPage.tabs.count - 1) / columns)
                //height: rows > 0 ? rows * (page.width / tabsGrid.columns) : 0
                //anchors.bottom: favoriteSectionHeader.top
                move: Transition {
                    NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad; duration: 200 }
                }
                add: Transition {
                    AddAnimation {}
                }

                Browser.TabItem {
                    active: true
                    width: page.width/tabsGrid.columns
                    height: width
                    activeTab: true
                    url: browserPage.url
                    title: browserPage.title
                    thumbnailPath: browserPage.thumbnailPath
                    onClicked: pageStack.pop()
                }

                Repeater {
                    model: browserPage.tabs

                    Browser.TabItem {
                        width: page.width/tabsGrid.columns
                        height: width
                        active: index < tabsGrid.loadIndex || initialized
                        asynchronous: index >= tabsGrid.loadIndex

                        url: model.url
                        title: model.title
                        thumbnailPath: model.thumbnailPath

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
            }
        }
        hasContextMenu: !page.newTab

        onLoad: {
            if (newTab) page.newTab = true
            page.load(url, title)
        }

        onRemoveBookmark: browserPage.favorites.removeBookmark(url)

        Browser.TabPageMenu {
            active: initialized
            visible: browserPage.tabs.count > 0 && !page.newTab
            shareEnabled: browserPage.url == _search
            browserPage: page.browserPage
            flickable: favoriteList
        }
    }
}
