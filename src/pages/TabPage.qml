/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0
import "components"

Page {
    id: page

    property BrowserPage browserPage
    property Item contextMenu
    property Item tabContextMenu

    backNavigation: browserPage.tabs.count > 0

    Component {
        id: favoriteContextMenuComponent

        ContextMenu {
            property string url: ""
            MenuItem {
                //% "Open in new tab"
                text: qsTrId("sailfish_browser-me-open_new_tab")
                onClicked: {
                    browserPage.newTab(url, true)
                    pageStack.pop(undefined, true)
                    contextMenu.hide()
                }
            }
        }
    }

    Component {
        id: tabContextMenuComponent

        ContextMenu {
            property int index: 0
            MenuItem {
                //% "Close tab"
                text: qsTrId("sailfish_browser-me-close_tab")
                onClicked: {
                    tabContextMenu.hide()
                    if (browserPage.tabs.count > 1) {
                        browserPage.closeTab(index)
                    } else {
                        browserPage.closeAllTabs()
                    }
                }
            }
        }
    }

    Component {
        id: simpleListHeader

        PageHeader {
            //% "Tabs and Favorites"
            title: qsTrId("sailfish_browser-he-tabs_and_favorites")
        }
    }

    Component {
        id: listHeader

        Column {
            PageHeader {
                //% "Tabs and Favorites"
                title: qsTrId("sailfish_browser-he-tabs_and_favorites")
            }

            Item {
                width: list.width
                height: tabs.height + 2 * Theme.paddingMedium

                Grid {
                    id: tabs
                    columns: 2
                    rows: Math.ceil(browserPage.tabs.count / 2)
                    spacing: Theme.paddingMedium
                    anchors {
                        margins: Theme.paddingMedium
                        top: parent.top;
                        left: parent.left
                    }

                    Repeater {
                        model: browserPage.tabs

                        Item {
                            id: tabItem

                            property bool menuOpen: tabContextMenu !== null && tabContextMenu.parent === tabItem

                            width: tabDelegate.width
                            height: menuOpen ? width + tabContextMenu.height: width

                            BackgroundItem {
                                id: tabDelegate

                                width: list.width / 2 - 2 * Theme.paddingMedium
                                height: width

                                Rectangle {
                                    anchors.fill: parent
                                    color: Theme.highlightBackgroundColor
                                    opacity: Theme.highlightBackgroundOpacity
                                    visible: !thumb.visible
                                }

                                Label {
                                    width: parent.width
                                    anchors.centerIn: parent.Center
                                    text: url
                                    visible: !thumb.visible
                                    font.pixelSize: Theme.fontSizeExtraSmall
                                    color: Theme.secondaryColor
                                    wrapMode:Text.WrapAnywhere
                                }

                                Image {
                                    id: thumb
                                    asynchronous: true
                                    source: "" // thumbPath.path ? thumbPath.path : ""
                                    fillMode: Image.PreserveAspectCrop
                                    sourceSize {
                                        width: parent.width
                                        height: width
                                    }
                                    visible: false // TODO status !== Image.Error && thumbPath.path !== ""
                                }
                                onClicked: {
                                    browserPage.loadTab(model.index)
                                    window.pageStack.pop(browserPage, true)
                                }
                                onPressAndHold: {
                                    if (!tabContextMenu) {
                                        tabContextMenu = tabContextMenuComponent.createObject(tabs)
                                    }
                                    tabContextMenu.index = index
                                    tabContextMenu.show(tabItem)
                                }

                                Rectangle {
                                    anchors.fill: parent

                                    property bool active: pressed
                                    property real highlightOpacity: 0.5

                                    color: Theme.highlightBackgroundColor
                                    opacity: active ? highlightOpacity : 0.0
                                    Behavior on opacity {
                                        FadeAnimation {
                                            duration: 100
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    SilicaListView {
        id: list
        anchors.fill: parent

        PullDownMenu {
            MenuItem {
                //% "New tab"
                text: qsTrId("sailfish_browser-me-new_tab")
                onClicked: browserPage.newTab("", true)
            }
            MenuItem {
                //% "Close all tabs"
                text: qsTrId("sailfish_browser-me-close_all")
                onClicked: browserPage.closeAllTabs()
                enabled: browserPage.tabs.count > 0
            }
        }

        header: browserPage.tabs.count === 0 ?  simpleListHeader : listHeader

        model: browserPage.favorites

        delegate: Item {
            id: favoriteItem

            property bool menuOpen: contextMenu !== null && contextMenu.parent === favoriteItem

            width: list.width
            height: menuOpen ? favoriteRow.height + contextMenu.height : favoriteRow.height

            BackgroundItem {
                id: favoriteRow

                width: list.width
                anchors.topMargin: Theme.paddingLarge

                FaviconImage {
                    id: faviconImage
                    anchors {
                        verticalCenter: titleLabel.verticalCenter
                        left: parent.left; leftMargin: Theme.paddingMedium
                    }
                    favicon: model.favicon
                    link: url
                }

                Label {
                    id: titleLabel
                    anchors {
                        leftMargin: Theme.paddingMedium
                        left: faviconImage.right
                        verticalCenter: parent.verticalCenter
                    }
                    width: parent.width - x
                    text: title
                    truncationMode: TruncationMode.Fade
                }

                onClicked: {
                    browserPage.load(url)
                    window.pageStack.pop(browserPage, true)
                }
                onPressAndHold: {
                    if (!contextMenu) {
                        contextMenu = favoriteContextMenuComponent.createObject(list)
                    }
                    contextMenu.url = url
                    contextMenu.show(favoriteItem)
                }
            }
        }

        VerticalScrollDecorator {}
    }
}
