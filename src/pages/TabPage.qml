/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 2.0
import QtGraphicalEffects 1.0
import Sailfish.Browser 1.0
import Sailfish.Silica 1.0
import "components" as Browser

Page {
    id: page

    property BrowserPage browserPage
    // focus to input field on opening
    property bool initialSearchFocus
    property bool newTab

    property bool _loadRequested
    property bool _editing
    property string _search

    function load(url, title) {
        if (page.newTab) {
            browserPage.newTab(url, true, title)
        } else {
            browserPage.load(url, title)
        }
        _loadRequested = true
        pageStack.pop(browserPage)
    }

    backNavigation: browserPage.tabs.count > 0 && browserPage.url != ""
    onStatusChanged: {
        // If tabs have been closed and user swipes
        // away from TabPage, then load current tab. backNavigation is disabled when
        // all tabs have been closed. In addition, if user tabs on favorite,
        // opens favorite in new tab via context menu, selects history item,
        // enters url on search field, or actives loading by tapping on an open tab
        // then loadRequested is set true and this code
        // path does not trigger loading again.
        if (!_loadRequested && status == PageStatus.Deactivating) {
            browserPage.load(browserPage.currentTab.url, browserPage.currentTab.title)
        }
    }

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
            height: commonHeader.height

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
            height: page.newTab ? commonHeader.height : commonHeader.height + tabsGrid.height

            Component.onCompleted: page.favoriteHeader = favoriteHeader

            Grid {
                id: tabsGrid
                visible: !page.newTab
                columns: page.isPortrait ? 2 : 4
                rows: Math.ceil(browserPage.tabs.count / columns)
                anchors.bottom: favoriteHeader.bottom
                move: Transition {
                    NumberAnimation { properties: "x,y"; easing.type: Easing.InOutQuad; duration: 200 }
                }

                Repeater {
                    model: browserPage.tabs
                    BackgroundItem {
                        id: tabDelegate
                        width: page.width/tabsGrid.columns
                        height: width

                        Rectangle {
                            id: placeHolder
                            anchors.fill: parent
                            visible: !thumb.visible
                            color: Theme.rgba(Theme.highlightColor, 0.1)

                            Column {
                                anchors {
                                    topMargin: Theme.paddingMedium
                                    top: parent.top
                                }
                                width: parent.width
                                spacing: Theme.paddingSmall
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: parent.width - Theme.paddingMedium * 2
                                    text: title
                                    font.pixelSize: Theme.fontSizeExtraSmall
                                    color: Theme.primaryColor
                                    truncationMode: TruncationMode.Fade
                                }
                                Label {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    width: parent.width - Theme.paddingMedium * 2
                                    text: url
                                    font.pixelSize: Theme.fontSizeExtraSmall
                                    color: Theme.rgba(Theme.secondaryColor, 0.6)
                                    wrapMode: Text.WrapAnywhere
                                    maximumLineCount: 3
                                }
                            }
                        }

                        Image {
                            id: thumb
                            anchors.fill: parent
                            asynchronous: true
                            source: thumbnailPath
                            sourceSize.width: Screen.width / 2
                            visible: status !== Image.Error && thumbnailPath !== ""

                            RadialGradient {
                                source: thumb
                                width: thumb.width
                                height: thumb.height
                                anchors.centerIn: parent
                                horizontalOffset: - width/2
                                verticalOffset: - width/2
                                verticalRadius: Theme.itemSizeExtraLarge * 3
                                horizontalRadius: Theme.itemSizeExtraLarge * 3
                                clip: true

                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "transparent"}
                                    GradientStop { position: 1.0; color: Theme.highlightDimmerColor}
                                }
                            }
                        }
                        Browser.CloseTabButton {}

                        onClicked: {
                            _loadRequested = true
                            browserPage.loadTab(model.index, model.url, model.title)
                        }
                    }
                }
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


    Item {
        id: commonHeader
        width: page.width
        height: headerContent.height

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

            Label {
                id: titleLabel
                anchors {
                    bottom: searchField.top
                    bottomMargin: Theme.paddingMediun
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: searchField.right
                    rightMargin: Theme.paddingMedium
                }
                //% "New tab"
                text: (browserPage.tabs.count == 0 || newTab) ? qsTrId("sailfish_browser-la-new_tab") : (browserPage.currentTab.url == _search ? browserPage.currentTab.title : "")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                width: parent.width - Theme.iconSizeSmall - Theme.paddingSmall
                truncationMode: TruncationMode.Fade
            }

            TextField {
                id: searchField

                anchors.bottom: parent.bottom
                width: page.width - (closeActiveTabButton.visible ? closeActiveTabButton.width : 0)
                // Handle initially newTab state. Currently newTab initially
                // true when triggering new tab cover action.
                text: newTab ? "" : browserPage.currentTab.url

                //: Placeholder for the search/address field
                //% "Search or Address"
                placeholderText: qsTrId("sailfish_browser-ph-search_or_url")
                color: searchField.focus ? Theme.highlightColor : Theme.primaryColor
                focusOutBehavior: FocusBehavior.KeepFocus
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly

                label: text.length == 0 ? "" : (text == browserPage.currentTab.url
                                                //: Current browser page loaded
                                                //% "Done"
                                                && !browserPage.viewLoading ? qsTrId("sailfish_browser-la-done")
                                                                              //% "Search"
                                                                            : qsTrId("sailfish_browser-la-search"))

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

            Browser.CloseTabButton {
                id: closeActiveTabButton
                visible: browserPage.currentTab.valid && !page.newTab && !searchField.focus
                closeActiveTab: true
            }
        }
    }
}
