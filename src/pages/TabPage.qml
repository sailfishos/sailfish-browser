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

        header: Column {
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
                            visible: thumb.status == Image.Error
                        }

                        Label {
                            width: parent.width
                            anchors.centerIn: parent.Center
                            text: url
                            visible: thumb.status == Image.Error
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
                        }
                        onClicked: {
                            if (browserPage.currentTabIndex !== model.index) {
                                browserPage.currentTabIndex = model.index
                                browserPage.load(url)
                            }
                            window.pageStack.pop(browserPage, true)
                        }
                    }
                }
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
