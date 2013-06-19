/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0

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
