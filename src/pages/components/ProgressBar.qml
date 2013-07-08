/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/
import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0

Item {
    id: progressBar

    // From 0 to 1.0
    property real progress: 0.0

    visible: opacity > 0.0

    Rectangle {
        id: progressRect
        anchors {
            top: parent.top
            left: parent.left
        }
        height: parent.height
        width: opacity > 0.0 ? progressBar.progress * parent.width : 0
        color: Theme.highlightBackgroundColor
        opacity: Theme.highlightBackgroundOpacity

        Behavior on width {
            enabled: progressBar.opacity == 1.0
            SmoothedAnimation {
                velocity: 480; duration: 200
            }
        }
    }
    onVisibleChanged: {
        if (!visible) {
            progress = 0
        }
    }

    Behavior on opacity { FadeAnimation {} }
}
