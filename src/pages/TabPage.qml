/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 1.1
import Sailfish.Silica 1.0
import "components"

Page {
    id: page

    SilicaListView {
        id: list
        anchors.fill: parent

        header: Item {
            width: list.width
            height: tabs.height + 2 * theme.paddingMedium

            Grid {
                id: tabs
                columns: 2
                rows: Math.ceil(browserPage.tabs.count / 2)
                spacing: theme.paddingMedium
                anchors {
                    margins: theme.paddingMedium
                    top: parent.top;
                    left: parent.left
                }

                Repeater {
                    model: browserPage.tabs
                    BackgroundItem {
                        width: list.width / 2 - 2 * theme.paddingMedium
                        height: width

                        Rectangle {
                            anchors.fill: parent
                            color: theme.highlightBackgroundColor
                            opacity: theme.highlightBackgroundOpacity
                            visible: !thumb.visible
                        }

                        Label {
                            width: parent.width
                            anchors.centerIn: parent.Center
                            text: url
                            visible: !thumb.visible
                            font.pixelSize: theme.fontSizeSmall
                            color: theme.secondaryColor
                            truncationMode: TruncationMode.Elide
                        }

                        Image {
                            id: thumb
                            asynchronous: true
                            source: thumbPath
                            fillMode: Image.PreserveAspectCrop
                            sourceSize {
                                width: parent.width
                                height: width
                            }
                            visible: status !== Image.Error && thumbPath !== ""
                        }
                        onClicked: {
                            browserPage.loadTab(model.index)
                            window.pageStack.pop(browserPage, true)
                        }

                        Rectangle {
                            anchors.fill: parent

                            property bool active: pressed
                            property real highlightOpacity: 0.5

                            color: theme.highlightBackgroundColor
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

        PullDownMenu {
            MenuItem {
                //% "Close all tabs"
                text: qsTrId("sailfish_browser-me-close_all")
                onClicked: browserPage.closeAllTabs()
            }
        }

        model: browserPage.favorites

        delegate: BackgroundItem {
            width: list.width
            anchors.topMargin: theme.paddingLarge

            FaviconImage {
                id: faviconImage
                anchors {
                    verticalCenter: titleLabel.verticalCenter
                    left: parent.left; leftMargin: theme.paddingMedium
                }
                favicon: model.favicon
                link: url
            }

            Label {
                id: titleLabel
                anchors {
                    leftMargin: theme.paddingMedium
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
        }

        VerticalScrollDecorator {}
    }

    onStatusChanged: {
        if (status == PageStatus.Active) {
            backNavigation = false
        }
    }
}
