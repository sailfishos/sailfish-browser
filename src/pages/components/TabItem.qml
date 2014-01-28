/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import QtGraphicalEffects 1.0
import Sailfish.Silica 1.0

Rectangle {
    id: tabItem

    signal clicked

    color: Theme.rgba(Theme.highlightColor, 0.1)

    Column {
        visible: !thumb.visible
        anchors {
            topMargin: Theme.paddingMedium
            top: parent.top
        }
        width: parent.width
        spacing: Theme.paddingSmall
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - Theme.paddingMedium * 2
            text: title
            font.pixelSize: Theme.fontSizeExtraSmall
            color: highlight._showPress ? Theme.highlightColor : Theme.primaryColor
            truncationMode: TruncationMode.Fade
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - Theme.paddingMedium * 2
            text: url
            font.pixelSize: Theme.fontSizeExtraSmall
            color: highlight._showPress ? Theme.rgba(Theme.secondaryHighlightColor, 0.6) : Theme.rgba(Theme.secondaryColor, 0.6)
            wrapMode: Text.WrapAnywhere
            maximumLineCount: 3
        }
    }

    Image {
        id: thumb
        anchors.fill: parent
        asynchronous: true
        source: thumbnailPath
        sourceSize.width: Screen.width / 2
        visible: status !== Image.Error && thumbnailPath !== ""

        RadialGradient {
            source: thumb
            width: thumb.width
            height: thumb.height
            anchors.centerIn: parent
            horizontalOffset: - width/2
            verticalOffset: - width/2
            verticalRadius: Theme.itemSizeExtraLarge * 3
            horizontalRadius: Theme.itemSizeExtraLarge * 3
            clip: true

            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent"}
                GradientStop { position: 1.0; color: Theme.highlightDimmerColor}
            }
        }
    }

    BackgroundItem {
        id: highlight
        anchors.fill: parent
        onClicked: tabItem.clicked()
    }

    CloseTabButton {}
}
