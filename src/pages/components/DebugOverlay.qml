/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0

Item {
    id: debugOverlay

    function dump(fileName) {
        webView.sendAsyncMessage("Memory:Dump", {"fileName": fileName})
        readTimer.restart()
    }

    anchors.fill: parent

    Rectangle {
        id: dumpOrange
        color: "orange"
        anchors.top: scaleIndicator.bottom
        width: dumpText.width + Theme.paddingLarge
        height: 100
        MouseArea {
            anchors.fill: parent
            onClicked: debugOverlay.dump(StandardPaths.data + "/browser-mem-dump.gz")
        }

        Text {
            id: dumpText
            anchors {
                left: parent.left
                leftMargin: Theme.paddingMedium
                verticalCenter: dumpOrange.verticalCenter
            }
            text: "Dump"
            color: "white"
            font.bold: true
            font.pixelSize: 36
        }
    }

    Rectangle {
        color: "red"
        width: parent.width
        height: 50
        anchors.bottom: cWidth.top
        Text {
            anchors.centerIn: parent
            color: "white"
            font.bold: true
            font.pixelSize: 36
            text: "Content height: " + (webView.contentItem ? (webView.contentItem.contentHeight).toFixed(1) : 0)
        }
    }

    Rectangle {
        id: cWidth
        color: "red"
        width: parent.width
        height: 50
        anchors.bottom: scaleIndicator.top
        Text {
            anchors.centerIn: parent
            color: "white"
            font.bold: true
            font.pixelSize: 36
            text: "Content width: " + (webView.contentItem ? (webView.contentItem.contentWidth).toFixed(1) : 0)
        }
    }

    Rectangle {
        id: scaleIndicator
        color: "red"
        width: parent.width
        height: 50
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.itemSizeLarge * 2
        Text {
            anchors.centerIn: parent
            color: "white"
            font.bold: true
            font.pixelSize: 36
            text: "Scale: " + (webView.contentItem ? (webView.contentItem.contentWidth / parent.width).toFixed(1) : "")

        }
    }

    Timer {
        id: readTimer
        interval: 2000
        onTriggered: console.log("MEMORY DUMPPED")
    }

    Connections {
        target: WebUtils
        onDumpMemoryInfo: debugOverlay.dump(fileName)
    }
}
