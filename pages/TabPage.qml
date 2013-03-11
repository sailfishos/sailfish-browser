/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 1.1
import Sailfish.Silica 1.0

Page {
    SilicaGridView {
        id: grid
        anchors.fill: parent
        header: PageHeader {
            title: "Tabs"
        }
        cellWidth: page.width / 2
        cellHeight: cellWidth
        model: browserPage.tabs

        delegate: Item {
            width: grid.cellWidth
            height: width

            Image {
                anchors {
                    margins: theme.paddingMedium
                    fill: parent
                }
                asynchronous: true
                source: thumbPath
                fillMode: Image.PreserveAspectCrop

                transform: Rotation {
                    origin.x: width / 2
                    origin.y: width / 2
                    angle: window.screenRotation
                }

                sourceSize {
                    width: grid.cellWidth - 2 * theme.paddingMedium
                    height: width
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        browserPage.url = url
                        browserPage.currentTab = index
                        window.pageStack.pop(browserPage, true)
                    }
                }
            }
        }
        ScrollDecorator {}
    }
}
