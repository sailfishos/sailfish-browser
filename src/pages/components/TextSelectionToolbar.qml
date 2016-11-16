/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import Sailfish.Silica 1.0
import "." as Browser

Row {
    id: root

    property int horizontalOffset
    property int iconWidth
    property bool isPhoneNumber

    signal call
    signal clear
    signal search
    signal share

    // Suppress warnings
    width: parent && parent.width || 0
    height: parent && parent.height || 0

    Repeater {
        id: repeater

        readonly property int visibleCount: {
            var count = 0;
            for (var i = 0; i < model.length; ++i) {
                if (model[i].visible) {
                    ++count;
                }
            }
            return count;
        }

        model: [
            {
                icon: "image://theme/icon-m-answer",
                offset: root.isPhoneNumber ? 0 : root.horizontalOffset,
                signalFunc: "call",
                visible: root.isPhoneNumber
            },
            {
                icon: "image://theme/icon-m-share",
                offset: root.horizontalOffset,
                signalFunc: "share",
                visible: true
            },
            {
                icon: "image://theme/icon-m-search",
                offset: 0,
                signalFunc: "search",
                visible: true
            },
            {
                icon: "image://theme/icon-m-clear",
                offset: root.isPhoneNumber ? 0 : -root.horizontalOffset,
                signalFunc: "clear",
                visible: true
            }
        ]

        Browser.IconButton {
            visible: modelData.visible
            width: parent.width / repeater.visibleCount
            icon.source: modelData.icon
            icon.anchors.horizontalCenterOffset: modelData.offset
            onTapped: root[modelData.signalFunc]()
        }
    }
}
