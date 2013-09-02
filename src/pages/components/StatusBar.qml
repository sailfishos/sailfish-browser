/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {
    id: statusBar

    property string title
    property string url

    signal searchClicked
    signal closeClicked

    gradient: Gradient {
        GradientStop { position: 0.1; color: "transparent" }
        GradientStop { position: 0.75; color: Theme.highlightColor }
    }
    visible: opacity > 0.0

    Row {
        anchors{
            left: parent.left; leftMargin: Theme.paddingMedium
            right: parent.right; rightMargin: Theme.paddingMedium
            bottom: parent.bottom; bottomMargin: Theme.paddingLarge
        }
        spacing: Theme.paddingSmall
        height: texts.height

        IconButton {
            id: searchButton
            anchors.verticalCenter: parent.verticalCenter
            icon.source: "image://theme/icon-m-search"
            onClicked: statusBar.searchClicked()
        }

        MouseArea {
            width: parent.width - (searchButton.width * 2) - (parent.spacing * 2)
            height: texts.height

            onClicked: statusBar.searchClicked()

            Column {
                id: texts
                anchors.bottom: parent.bottom
                width: parent.width

                Label {
                    text: title
                    width: parent.width
                    color: "black"
                    font.pixelSize: Theme.fontSizeExtraSmall
                    horizontalAlignment: Text.AlignLeft
                    truncationMode: TruncationMode.Elide
                }
                Label {
                    text: url
                    width: parent.width
                    color: "black"
                    font.pixelSize: Theme.fontSizeTiny
                    horizontalAlignment: Text.AlignLeft
                    truncationMode: TruncationMode.Elide
                }
            }
        }
        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            icon.source: "image://theme/icon-m-dismiss"
            onClicked: statusBar.closeClicked()
        }
    }
}
