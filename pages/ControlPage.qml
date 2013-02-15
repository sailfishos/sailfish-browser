/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/


import QtQuick 1.1
import Sailfish.Silica 1.0

Page {
    id: page

    property Item contextMenu
    property alias historyModel: historyList.model
    property string url

    function urlEntered() {
        urlField.closeSoftwareInputPanel()

        var url = urlField.text
        if (url.indexOf("http://") < 0) {
            url = "http://" + url
        }

        browserPage.url = url
        pageStack.pop(undefined, true)
    }

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
        clip : true

        anchors {
            top: parent.top
            bottom: urlField.top
            left: parent.left
            right: parent.right
        }

        header: PageHeader {
            title: "History"
        }

        PullDownMenu {
            MenuItem {
                text: "Tabs"
                onClicked:  {
                    var component = Qt.createComponent("TabPage.qml");
                    if (component.status === Component.Ready) {
                        pageStack.push(component, {}, false);
                    } else {
                        console.log("Error loading component:", component.errorString());
                    }
                }
            }
            MenuItem {
                text: "New tab"
                onClicked: {
                    browserPage.newTab()
                }
            }
        }

        delegate: Item {
            id: historyItem
            property bool menuOpen: contextMenu!=null && contextMenu.parent == historyItem

            width: page.width
            height: menuOpen ?  historyRow.height + contextMenu.height : historyRow.height

            BackgroundItem {
                id: historyRow
                width: page.width
                height: 80

                Image {
                    id: iconImage
                    source: icon
                    x: 30
                    anchors.top: parent.top
                }
                Label {
                    text: title
                    anchors{
                        top: parent.top
                        left: iconImage.right
                        right: parent.right
                    }
                    height: parent.height / 2
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignBottom
                    truncationMode: TruncationMode.Fade
                }
                Label {
                    text: url
                    anchors{
                        bottom: parent.bottom
                        left: iconImage.right
                        right: parent.right
                    }
                    height: parent.height / 2
                    font.pixelSize: theme.fontSizeSmall
                    color: theme.secondaryColor
                    horizontalAlignment: Text.AlignCenter
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

    TextField {
        id:urlField
        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
        }
        text: url
        width: parent.width - 2 * 30
        placeholderText: "url"

        Keys.onEnterPressed: {
            urlEntered()
        }

        Keys.onReturnPressed: {
            urlEntered()
        }
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            if (url!="") {
                urlField.selectAll()
            }

            urlField.forceActiveFocus()
        }
    }
}
