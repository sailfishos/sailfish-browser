/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 1.1
import Sailfish.Silica 1.0

Dialog {
    id: page

    property alias historyModel: historyList.model
    property Item contextMenu
    property Item urlField
    property string url

    acceptDestination: TabPage {}

    Component {
        id: historyContextMenuComponent
        ContextMenu {
            property string url: ""
            MenuItem {
                text: "Open in new tab"
                onClicked: {
                    browserPage.newTab()
                    browserPage.url = url
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
                acceptText: "All Tabs"
                dialog: page
            }

            Item {
                height: urlField.height
                width: parent.width
                Image {
                    source: "image://theme/icon-m-region"
                    width: urlField.height / 2
                    height: width
                    anchors {
                        top: urlField.top; topMargin: theme.paddingSmall
                        left: parent.left; leftMargin: theme.paddingMedium
                    }
                    smooth: true
                }

                TextField {
                    id:urlField

                    anchors {
                        left: parent.left
                        leftMargin: theme.paddingLarge + theme.paddingMedium
                        right: parent.right
                        rightMargin:theme.paddingLarge
                    }
                    text: url
                    placeholderText: "Search"
                    color: theme.primaryColor

                    function urlEntered() {
                        urlField.closeSoftwareInputPanel()
                        var url = urlField.text
                        if (url.indexOf("http://") < 0) {
                            url = "http://" + url
                        }
                        browserPage.url = url
                        pageStack.pop(undefined, true)
                    }

                    Keys.onEnterPressed: {
                        urlEntered()
                    }

                    Keys.onReturnPressed: {
                        urlEntered()
                    }

                    Component.onCompleted: {
                        page.urlField = urlField
                    }
                }
                Image {
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
                text: "New tab"
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
                height: theme.itemSizeMedium

                Image {
                    id: iconImage
                    source: icon
                    anchors.top: parent.top
                }
                Label {
                    text: title
                    anchors {
                        top: parent.top
                        left: iconImage.right
                        leftMargin: theme.paddingSmall
                        right: parent.right
                    }
                    height: parent.height / 2
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignBottom
                    truncationMode: TruncationMode.Fade
                }
                Label {
                    text: url
                    anchors {
                        bottom: parent.bottom
                        left: iconImage.right
                        leftMargin: theme.paddingSmall
                        right: parent.right
                    }
                    height: parent.height / 2
                    font.pixelSize: theme.fontSizeSmall
                    color: theme.secondaryColor
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignTop
                    truncationMode: TruncationMode.Fade
                }

                onClicked: {
                    browserPage.url = url
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
