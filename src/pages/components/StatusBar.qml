/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {
    property string title
    property string url

    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.rgba(1.0, 1.0, 1.0, 0.0) }
        GradientStop { position: 1.0; color: Theme.highlightDimmerColor }
    }

    Column {
        width: parent.width
        anchors {
            bottom: parent.bottom; bottomMargin: Theme.paddingMedium
        }

        Label {
            text: title
            width: parent.width - Theme.paddingMedium * 2
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeSmall
            horizontalAlignment: Text.AlignHCenter
            truncationMode: TruncationMode.Fade
        }
        Label {
            text: url
            width: parent.width - Theme.paddingMedium * 2
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeExtraSmall
            horizontalAlignment: Text.AlignHCenter
            truncationMode: TruncationMode.Fade
        }
    }
}
