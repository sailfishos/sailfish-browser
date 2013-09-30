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

    property bool _editing
    property string _search

    backNavigation: browserPage.tabs.count > 0

    Component {
        id: favoriteContextMenuComponent
        ContextMenu {
            id: favoriteContextMenu

            property string url: ""
            MenuItem {
                //% "Open in new tab"
                text: qsTrId("sailfish_browser-me-open_new_tab")
                onClicked: {
                    browserPage.newTab(url, true)
                    pageStack.pop(undefined, true)
                    favoriteContextMenu.hide()
                }
            }

            MenuItem {
                //: "Remove favorited / bookmarked web page"
                //% "Remove favorite"
                text: qsTrId("sailfish_browser-me-remove_favorite")
                onClicked: browserPage.favorites.removeBookmark(url)
            }
        }
    }

    Loader {
        anchors.fill: parent
        sourceComponent: _editing || initialSearchFocus ? historyList : favoriteList
    }

    Item {
        id: commonHeader
        width: page.width
        height: _editing ? headerContent.height : headerContent.height + tabsGrid.height

        function newTab() {
            searchField.forceActiveFocus()
            browserPage.newTab("", true)
        }

        Rectangle {
            id: headerContent
            width: parent.width
            height: width / 2
            color: Theme.rgba(Theme.highlightColor,0.1)

            Row {
                spacing: Theme.paddingSmall

                anchors {
                    bottom: searchField.top
                    bottomMargin: Theme.paddingLarge
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                }

                Rectangle {
                    height: Theme.iconSizeSmall
                    width: height
                    radius: 5.0
                    color: Theme.highlightColor
                    opacity: 0.1
                }

                Label {
                    id: titleLabel
                    //% "New tab"
                    text: (browserPage.tabs.count == 0 || browserPage.currentTab.url == "") ? qsTrId("sailfish_browser-la-new_tab") : (browserPage.currentTab.url == _search ? browserPage.currentTab.title : "")
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeSmall
                    width: page.width * 3.0 / 4.0
                    truncationMode: TruncationMode.Fade
                }
            }

            TextField {
                id: searchField
                width: page.width

                anchors.bottom: parent.bottom
                anchors.bottomMargin: Theme.paddingMedium
                text: browserPage.currentTab.url

                //: Placeholder for the search field
                //% "Search"
                placeholderText: qsTrId("sailfish_browser-ph-search")
                color: searchField.focus? Theme.highlightColor : Theme.primaryColor
                focusOutBehavior: FocusBehavior.KeepFocus
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase

                EnterKey.onClicked: {
                    Qt.inputMethod.hide()
                    // let gecko figure out how to handle malformed URLs
                    browserPage.load(searchField.text)
                    pageStack.pop(undefined, true)
                }

                onTextChanged: if (text != browserPage.currentTab.url) browserPage.history.search(text)
                onActiveFocusChanged: if (activeFocus) selectAll()

                Binding { target: page; property: "_search"; value: searchField.text }
                Binding { target: page; property: "_editing"; value: searchField.focus }

                Component.onCompleted: {
                    if (initialSearchFocus) {
                        searchField.forceActiveFocus()
                        initialSearchFocus = false
                    }
                }
            }
        }

        Grid {
            visible: !page._editing
            id: tabsGrid
            columns: 2
            rows: Math.ceil(browserPage.tabs.count / 2) + 1
            anchors {
                top: headerContent.bottom
            }
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
                        asynchronous: true
                        source: thumbnailPath
                        fillMode: Image.PreserveAspectCrop
                        sourceSize {
                            width: parent.width
                            height: width
                        }
                        visible: status !== Image.Error && thumbnailPath !== ""

                        RadialGradient {
                            id: gradient
                            source: thumb.visible? thumb : placeHolder
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

                    IconButton {
                        anchors {
                            bottom: parent.bottom; bottomMargin: Theme.paddingMedium
                            right: parent.right; rightMargin: Theme.paddingMedium
                        }
                        icon.source: "image://theme/icon-m-close"
                        onClicked: browserPage.closeTab(index)
                    }

                    onClicked: {
                        browserPage.loadTab(model.index)
                        window.pageStack.pop(browserPage, true)
                    }
                }
            }
        }
    }

    Component {
        id: favoriteList
        SilicaListView {
            PullDownMenu {
                MenuItem {
                    //% "Close all tabs"
                    text: qsTrId("sailfish_browser-me-close_all")
                    onClicked: browserPage.closeAllTabs()
                    enabled: browserPage.tabs.count > 0
                }
                MenuItem {
                    enabled: browserPage.currentTab.url == _search
                    //: Share link from browser pulley menu
                    //% "Share"
                    text: qsTrId("sailfish_browser-me-share_link")

                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("ShareLinkPage.qml"), {"link" : browserPage.currentTab.url, "linkTitle": browserPage.currentTab.title})
                    }
                }
                MenuItem {
                    //% "New tab"
                    text: qsTrId("sailfish_browser-me-new_tab")
                    onClicked: {
                        // browserPage.newTab("", true)
                        // Open URL editor
                        commonHeader.newTab()
                    }
                }
            }

            header: Item {
                id: listHeader
                width: commonHeader.width
                height: commonHeader.height

                Component.onCompleted: commonHeader.parent = listHeader
            }

            anchors.fill: parent
            model: browserPage.favorites
            delegate: ListItem {
                width: page.width
                menu: favoriteContextMenuComponent
                showMenuOnPressAndHold: false

                Row {
                    height: parent.height
                    spacing: Theme.paddingMedium
                    anchors {
                        left: parent.left
                        right: parent.right
                        leftMargin: Theme.paddingLarge
                        rightMargin: Theme.paddingLarge
                    }

                    Browser.FaviconImage {
                        id: faviconImage
                        anchors.verticalCenter: titleLabel.verticalCenter
                        favicon: model.favicon
                        link: url
                    }

                    Label {
                        id: titleLabel
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - faviconImage.width
                        text: title
                        color: highlighted ? Theme.highlightColor : Theme.primaryColor
                        truncationMode: TruncationMode.Fade
                    }
                }

                onClicked: {
                    browserPage.load(url)
                    window.pageStack.pop(browserPage, true)
                }
                onPressAndHold: showMenu({"url": url})
            }
            VerticalScrollDecorator {}
        }
    }

    Component {
        id: historyList
        SilicaListView {
            header: Item {
                id: listHeader
                width: commonHeader.width
                height: commonHeader.height

                Component.onCompleted: commonHeader.parent = listHeader
            }

            model: browserPage.history
            // To prevent model to steal focus
            currentIndex: -1

            delegate: BackgroundItem {
                width: page.width
                height: Theme.itemSizeLarge

                Column {
                    width: page.width - Theme.paddingLarge * 2
                    x: Theme.paddingLarge
                    anchors.verticalCenter: parent.verticalCenter

                    Label {
                        text: Theme.highlightText(title, _search, Theme.highlightColor)
                        truncationMode: TruncationMode.Fade
                        width: parent.width
                        color: highlighted ? Theme.highlightColor : Theme.primaryColor
                    }
                    Label {
                        text: Theme.highlightText(url, _search, Theme.highlightColor)
                        width: parent.width
                        font.pixelSize: Theme.fontSizeSmall
                        opacity: 0.6
                        color: highlighted ? Theme.highlightColor : Theme.primaryColor
                        truncationMode: TruncationMode.Elide
                    }
                }

                onClicked: {
                    Qt.inputMethod.hide()
                    browserPage.load(url)
                    pageStack.pop(undefined, true)
                }

            }
            VerticalScrollDecorator {}
        }
    }

    /*
       TODO
       For the first one, after the model code has been changed to filter out current tab
                            OpacityRampEffect {
                                enabled: !gradient.visible
                                visible: enabled
                                sourceItem: thumb.visible? thumb : placeHolder
                                slope: 1.3
                                offset: 0.20
                                direction: OpacityRamp.RightToLeft
                            }
    */
}
