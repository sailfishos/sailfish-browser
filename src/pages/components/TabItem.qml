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

MouseArea {
    readonly property bool down: pressed && containsMouse
    // Expose ListView for all items
    property Item view: ListView.view
    layer.effect: PressEffect {}
    layer.enabled: down
    width: parent.width
    height: Screen.width / 2

    onClicked: {
        if (index === 0) {
            view.hide()
        } else {
            view.activateTab(index)
        }
    }

    Image {
        source: thumbnailPath
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        cache: false
    }

    Rectangle {
        width: parent.width
        height: parent.height / 2
        anchors.bottom: parent.bottom
        gradient: Gradient {
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: Theme.highlightDimmerColor }
        }
    }

    IconButton {
        id: favorite

        anchors.bottom: parent.bottom
        icon.source: favorited ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"
        onClicked: {
            if (favorited) {
                view.removeBookmark(url)
            } else {
                view.addBookmark(url, title, favicon)
            }
        }
    }

    Text {
        anchors {
            left: favorite.right
            leftMargin: Theme.paddingMedium
            right: close.left
            rightMargin: Theme.paddingMedium
            verticalCenter: favorite.verticalCenter
        }

        text: title
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        color: down ? Theme.highlightColor : Theme.primaryColor
    }

    IconButton {
        id: close

        anchors {
            bottom: parent.bottom
            right: parent.right
        }

        icon.source: "image://theme/icon-m-close"
        onClicked: {
            view.closeTab(index)
        }
    }
}
