/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 1.1
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import "components"

Page {
    id: page

    property alias historyModel: historyList.model
    property Item contextMenu
    property Item urlField
    property string url



    Component {
        id: historyContextMenuComponent
        ContextMenu {
            property string url: ""
            MenuItem {
                //% "Open in new tab"
                text: qsTrId("sailfish_browser-me-open_new_tab")
                onClicked: {
                    browserPage.newTab(url, true)
                    pageStack.pop(undefined, true)
                }
            }
        }
    }

    SilicaListView {
        id: historyList

        anchors.fill: parent

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

                    property bool favourited: browserPage.favorites.count > 0 && browserPage.favorites.contains(webEngine.url)
                    enabled: urlField.text === url

                    anchors {
                        top: urlField.top; topMargin: theme.paddingSmall
                        left: parent.left; leftMargin: theme.paddingMedium
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
                                browserPage.favorites.addBookmark(url, webEngine.title, browserPage.favicon)
                            }
                        }
                    }
                }

                TextField {
                    id:urlField

                    anchors {
                        left: faviconIcon.right; leftMargin: theme.paddingSmall - theme.paddingLarge
                        right: clearIcon.left; rightMargin: theme.paddingSmall - theme.paddingLarge
                    }
                    text: url
                    //: Placeholder for the search field
                    //% "Search"
                    placeholderText: qsTrId("sailfish_browser-ph-search")
                    color: theme.primaryColor
                    focusOutBehavior: FocusBehavior.KeepFocus

                    EnterKey.onClicked: {
                        urlField.closeSoftwareInputPanel()
                        var url = urlField.text

                        if (url.indexOf("http://") < 0
                                && url.indexOf("https://") < 0
                                && url.indexOf("file://") < 0) {
                            url = "http://" + url
                        }
                        browserPage.load(url)
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
                        top: urlField.top; topMargin: theme.paddingSmall
                        right: parent.right; rightMargin: theme.paddingMedium
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
                    urlField.closeSoftwareInputPanel()
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

        delegate: Item {
            id: historyItem
            property bool menuOpen: contextMenu != null && contextMenu.parent == historyItem

            width: page.width
            height: menuOpen ?  historyRow.height + contextMenu.height : historyRow.height

            BackgroundItem {
                id: historyRow
                width: page.width
                height: theme.itemSizeLarge

                Image {
                    id: iconImage
                    source: icon.path !== "" ? icon.path : "image://theme/icon-m-region"
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
                           leftMargin: theme.paddingMedium
                           verticalCenter: iconImage.verticalCenter
                    }

                    Label {
                        text: title
                        truncationMode: TruncationMode.Fade
                        width: parent.width
                    }
                    Label {
                        text: url
                        width: parent.width
                        font.pixelSize: theme.fontSizeSmall
                        color: theme.secondaryColor
                        truncationMode: TruncationMode.Elide
                    }
                }

                onClicked: {
                    urlField.closeSoftwareInputPanel()
                    browserPage.load(url)
                    pageStack.pop(undefined, true)
                }
                onPressAndHold: {
                    if (!contextMenu) {
                        contextMenu = historyContextMenuComponent.createObject(historyList)
                    }
                    contextMenu.url = url
                    contextMenu.show(historyItem)
                }
            }
        }
        VerticalScrollDecorator {}
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            if (url != "") {
                urlField.selectAll()
            }
            urlField.forceActiveFocus()
        } else {
            urlField.closeSoftwareInputPanel()
        }
    }
}
