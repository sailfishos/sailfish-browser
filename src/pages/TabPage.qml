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

    Component {
        id: tabContextMenuComponent

        ContextMenu {
            id: tabContextMenu

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

            // Here we could also have bookmark this tab but tab model doesn't contain title or favIcon
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
            width: parent.width
            height: childrenRect.height + Theme.paddingMedium

            PageHeader {
                //% "Tabs and Favorites"
                title: qsTrId("sailfish_browser-he-tabs_and_favorites")
            }

            Grid {
                id: tabs
                columns: 2
                rows: Math.ceil(browserPage.tabs.count / 2)
                spacing: Theme.paddingMedium
                anchors {
                    left: parent.left; leftMargin: Theme.paddingLarge
                    right: parent.right; rightMargin: Theme.paddingLarge
                }

                Repeater {
                    model: browserPage.tabs

                    ListItem {
                        id: tabDelegate

                        width: (tabs.width - (tabs.columns - 1) * tabs.spacing) / tabs.columns
                        contentHeight: width
                        showMenuOnPressAndHold: false
                        menu: tabContextMenuComponent

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
                        onPressAndHold: showMenu({"index": index})
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

        delegate: ListItem {
            width: list.width
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

                FaviconImage {
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
