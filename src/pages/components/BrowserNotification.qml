/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {
    id: notification

    anchors.horizontalCenter: parent.horizontalCenter
    anchors.top: parent.top
    width: parent.width
    height: Theme.itemSizeSmall
    color: Theme.highlightBackgroundColor
    opacity: notificationTimer.running ? 1.0 : 0.0
    Behavior on opacity { FadeAnimation {} }

    function show(text) {
        notificationLabel.text = text
        notificationTimer.restart()
    }

    Label {
        id: notificationLabel

        anchors.centerIn: parent
        color: Theme.primaryColor
    }

    Timer {
        id: notificationTimer
        interval: 2000
    }
}
