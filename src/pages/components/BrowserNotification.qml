/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

import QtQuick 1.1
import Sailfish.Silica 1.0

Rectangle {
    id: notification

    anchors.horizontalCenter: parent.horizontalCenter
    anchors.top: parent.top
    width: parent.width
    height: theme.itemSizeSmall
    color: theme.highlightBackgroundColor
    opacity: notificationTimer.running ? 1.0 : 0.0
    Behavior on opacity { FadeAnimation {} }

    function show(text) {
        notificationLabel.text = text
        notificationTimer.restart()
    }

    Label {
        id: notificationLabel

        anchors.centerIn: parent
        color: theme.primaryColor
    }

    Timer {
        id: notificationTimer
        interval: 2000
    }
}
