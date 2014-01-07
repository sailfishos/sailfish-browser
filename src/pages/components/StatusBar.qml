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
            bottom: parent.bottom; bottomMargin: Theme.paddingMedium
        }

        Browser.IconButton {
            id: searchButton
            anchors.verticalCenter: parent.verticalCenter
            source: "image://theme/icon-m-search"
            onClicked: statusBar.searchClicked()
        }

        Browser.TitleBar {
            id: titleBar
            title: statusBar.title
            url: statusBar.url
            width: parent.width - searchButton.width * 2
            height: searchButton.height
            onClicked: statusBar.searchClicked()
        }

        Browser.IconButton {
            anchors.verticalCenter: parent.verticalCenter
            source: "image://theme/icon-m-close"
            onClicked: statusBar.closeClicked()
        }
    }
}
