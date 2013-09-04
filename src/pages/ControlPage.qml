/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import "components"

Page {
    id: page

    property alias historyModel: historyList.model
    property Item urlField
    property string url
    property string title

    Component {
        id: historyContextMenuComponent
        ContextMenu {
            id: historyContextMenu

            property string url: ""
            MenuItem {
                //% "Open in new tab"
                text: qsTrId("sailfish_browser-me-open_new_tab")
                onClicked: {
                    browserPage.newTab(url, true)
                    pageStack.pop(undefined, true)
                    historyContextMenu.hide()
                }
            }
        }
    }

    SilicaListView {
        id: historyList

        anchors.fill: parent
        // Following is to prevent editor from losing focus when model count
        // becomes non-zero
        currentIndex: -1

        header: Column {
            width: parent.width

            PageHeader {
                // We use the internal property to avoid transition
                // that is not appropriate for the case "Accept -> All tabs"
                //% "Search"
                title: qsTrId("sailfish_browser-he-search")
            }

            Item {
                height: urlField.height
                width: parent.width

                Image {
                    id: faviconIcon

                    width: urlField.height / 2
                    height: width
                    smooth: true

                    property bool favourited: browserPage.favorites.count > 0 && browserPage.favorites.contains(url)
                    enabled: urlField.text === url

                    anchors {
                        top: urlField.top; topMargin: Theme.paddingSmall
                        left: parent.left; leftMargin: Theme.paddingMedium
                    }

                    source: {
                        if (!enabled) {
                            return "image://theme/icon-m-region"
                        } else if (favourited) {
                            "image://theme/icon-m-favorite-selected"
                        } else {
                            return "image://theme/icon-m-favorite"
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (faviconIcon.favourited) {
                                browserPage.favorites.removeBookmark(url)
                            } else {
                                browserPage.favorites.addBookmark(url, title, browserPage.favicon)
                            }
                        }
                    }
                }

                TextField {
                    id: urlField

                    anchors {
                        left: faviconIcon.right; leftMargin: Theme.paddingSmall - Theme.paddingLarge
                        right: clearIcon.left; rightMargin: Theme.paddingSmall - Theme.paddingLarge
                    }
                    text: url
                    //: Placeholder for the search field
                    //% "Search"
                    placeholderText: qsTrId("sailfish_browser-ph-search")
                    color: Theme.primaryColor
                    focusOutBehavior: FocusBehavior.KeepFocus
                    inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase

                    EnterKey.onClicked: {
                        Qt.inputMethod.hide()
                        // let gecko figure out how to handle malformed URLs
                        browserPage.load(urlField.text)
                        pageStack.pop(undefined, true)
                    }

                    Component.onCompleted: {
                        page.urlField = urlField
                    }
                }
                Image {
                    id: clearIcon
                    source: "image://theme/icon-m-reset"
                    width: urlField.height / 2
                    height: width

                    anchors {
                        top: urlField.top; topMargin: Theme.paddingSmall
                        right: parent.right; rightMargin: Theme.paddingMedium
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: urlField.text = ""
                    }
                }
            }
        }

        PullDownMenu {
            MenuItem {
                //% "Clear tab history"
                text: qsTrId("sailfish_browser-me-clear_tab")
                onClicked: {
                    browserPage.deleteTabHistory()
                }
            }
            MenuItem {
                //% "Close tab"
                text: qsTrId("sailfish_browser-me-close_tab")
                onClicked: {
                    Qt.inputMethod.hide()
                    browserPage.closeTab()

                    if(browserPage.tabs.count === 0) {
                        var component = Qt.createComponent("TabPage.qml")
                        if (component.status !== Component.Ready) {
                            console.log("Error loading TabPage:", component.errorString());
                            return
                        }
                        pageStack.push(component.createObject(browserPage), {"browserPage": browserPage}, PageStackAction.Immediate)
                    }
                }
            }
            MenuItem {
                //% "New tab"
                text: qsTrId("sailfish_browser-me-new_tab")
                onClicked: {
                    urlField.text = ""
                    urlField.forceActiveFocus()
                    browserPage.newTab("",true)
                }
            }
        }

        delegate: ListItem {
            id: historyItem

            width: page.width
            contentHeight: Theme.itemSizeLarge
            showMenuOnPressAndHold: false
            menu: historyContextMenuComponent

            Image {
                id: iconImage
                source: thumbnailPath !== "" ? thumbnailPath : "image://theme/icon-m-region"
                asynchronous: true
                anchors.top: parent.top
                sourceSize {
                    height: parent.height
                    width: height
                }
                height: parent.height
                width: height

                onStatusChanged: {
                    if (status == Image.Error) {
                        source = "image://theme/icon-m-region"
                    }
                }
            }

            Column {
                width: parent.width - iconImage.width - anchors.leftMargin

                anchors {
                    left: iconImage.right
                    leftMargin: Theme.paddingMedium
                    verticalCenter: iconImage.verticalCenter
                }

                Label {
                    text: Theme.highlightText(title, urlField.text, Theme.highlightColor)
                    truncationMode: TruncationMode.Fade
                    width: parent.width
                }
                Label {
                    text: Theme.highlightText(url, urlField.text, Theme.highlightColor)
                    width: parent.width
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.secondaryColor
                    truncationMode: TruncationMode.Elide
                }
            }

            onClicked: {
                Qt.inputMethod.hide()
                browserPage.load(url)
                pageStack.pop(undefined, true)
            }
            onPressAndHold: showMenu({"url": url})
        }
        VerticalScrollDecorator {}
    }

    Connections {
        target: page.status === PageStatus.Active ? urlField : null
        onTextChanged: historyModel.search(urlField.text)
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            if (url != "") {
                urlField.selectAll()
            }
            urlField.forceActiveFocus()
        } else {
            Qt.inputMethod.hide()
        }
    }
}
