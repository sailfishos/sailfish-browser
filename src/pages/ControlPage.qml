/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 1.1
import Sailfish.Silica 1.0
import "components"

Dialog {
    id: page

    property alias historyModel: historyList.model
    property Item contextMenu
    property Item urlField
    property string url

    acceptDestination: Component { TabPage {} }

    Component {
        id: historyContextMenuComponent
        ContextMenu {
            property string url: ""
            MenuItem {
                //% "Open in new tab"
                text: qsTrId("sailfish_browser-me-open_new_tab")
                onClicked: {
                    browserPage.newTab()
                    browserPage.load(url)
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

            DialogHeader {
                // We use the internal property to avoid transition
                // that is not appropriate for the case "Accept -> All tabs"
                //% "All Tabs"
                _defaultAcceptText: qsTrId("sailfish_browser-he-all_tabs")
                dialog: page
            }

            Item {
                height: urlField.height
                width: parent.width

                FaviconImage {
                    id: faviconIcon
                    anchors {
                        bottom: urlField.verticalCenter
                        left: parent.left; leftMargin: theme.paddingMedium
                    }

                    favicon: urlField.text === url ? browserPage.favicon : "image://theme/icon-m-region"
                    link: url
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
                //% "New tab"
                text: qsTrId("sailfish_browser-me-new_tab")
                onClicked: browserPage.newTab()
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
                    source: icon
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
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            if (url != "") {
                urlField.selectAll()
            }
            urlField.forceActiveFocus()
        }
    }
}
