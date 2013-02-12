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
        header: PageHeader { title: "Tabs" }
        cellWidth: page.width / 2
        cellHeight: page.height / 2
        anchors.fill: parent
        model: browserPage.tabs
        delegate: Image {
            asynchronous: false
            source: thumbPath
            fillMode: Image.PreserveAspectCrop

            transform: [Rotation {
                    origin.x: 0
                    origin.y: 0
                    angle: window.screenRotation
                }, Translate {
                    x: window.screenRotation!==0 ? grid.cellWidth : 0
                }]

            sourceSize {
                width:  window.screenRotation!==0 ? grid.cellHeight : grid.cellWidth
                height: window.screenRotation!==0 ? grid.cellWidth : grid.cellHeight
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    browserPage.url = url
                    browserPage.currentTab = index
                    window.pageStack.pop(browserPage)
                }
            }
        }
        ScrollDecorator {}
    }
}
