/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import "." as Browser

Rectangle {
    id: statusBar

    property alias title: titleBar.title
    property alias url: titleBar.url

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
