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

    SilicaListView {
        id: list
        anchors.fill: parent
        header: Column {
            width: list.width

            PageHeader {
                title: "Tabs"
            }

            Grid {
                columns: 2
                rows: Math.ceil(browserPage.tabs.count / 2)
                spacing: theme.paddingMedium
                anchors {
                    leftMargin: theme.paddingMedium
                    left: parent.left
                }

                Repeater {
                    model: browserPage.tabs
                    BackgroundItem {
                        width: list.width / 2 - 2 * theme.paddingMedium
                        height: width
                        Image {
                            asynchronous: true
                            source: thumbPath
                            fillMode: Image.PreserveAspectCrop
                            transform: Rotation {
                                origin.x: width / 2
                                origin.y: width / 2
                                angle: window.screenRotation
                            }
                            sourceSize {
                                width: parent.width
                                height: width
                            }
                        }
                        onClicked: {
                            browserPage.url = url
                            browserPage.currentTabIndex = model.index
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

            Label {
                anchors.margins: theme.paddingMedium
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: title
            }

            onClicked: {
                browserPage.url = url
                window.pageStack.pop(browserPage, true)
            }
        }

        VerticalScrollDecorator {}
    }
}
