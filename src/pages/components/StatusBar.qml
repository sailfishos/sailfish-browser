/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0
import "." as Browser

Rectangle {
    id: statusBar

    property string title
    property string url

    signal searchClicked
    signal closeClicked

    gradient: Gradient {
        GradientStop { position: 0.0; color: "transparent" }
        GradientStop { position: 0.95; color: Theme.highlightColor}
    }
    enabled: opacity > 0.0

    Row {
        anchors{
            left: parent.left; leftMargin: Theme.paddingMedium
            right: parent.right; rightMargin: Theme.paddingMedium
            bottom: parent.bottom; bottomMargin: Theme.paddingLarge
        }
        height: texts.height

        Browser.IconButton {
            id: searchButton
            anchors.verticalCenter: parent.verticalCenter
            source: "image://theme/icon-m-search"
            onClicked: statusBar.searchClicked()
        }

        MouseArea {
            id: mouseArea
            width: parent.width - searchButton.width * 2
            height: texts.height

            onClicked: statusBar.searchClicked()

            Column {
                id: texts
                x: Theme.paddingSmall
                anchors.bottom: parent.bottom
                width: parent.width - Theme.paddingSmall * 2

                Label {
                    text: title
                    width: parent.width
                    color: mouseArea.pressed && mouseArea.containsMouse ? Theme.highlightColor : Theme.highlightDimmerColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    font.weight: Font.Normal
                    horizontalAlignment: Text.AlignLeft
                    truncationMode: TruncationMode.Elide
                }
                Label {
                    text: url
                    width: parent.width
                    color: mouseArea.pressed && mouseArea.containsMouse ? Theme.highlightColor : Theme.highlightDimmerColor
                    font.pixelSize: Theme.fontSizeTiny
                    font.weight: Font.Normal
                    horizontalAlignment: Text.AlignLeft
                    truncationMode: TruncationMode.Elide
                }
            }
        }
        Browser.IconButton {
            anchors.verticalCenter: parent.verticalCenter
            source: "image://theme/icon-m-close"
            onClicked: statusBar.closeClicked()
        }
    }
}
