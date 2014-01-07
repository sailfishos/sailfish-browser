/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0

BackgroundItem {
    id: backgroundItem

    property string title
    property string url

    implicitHeight: texts.height
    contentItem.color: "transparent"

    Column {
        id: texts
        x: Theme.paddingSmall
        width: parent.width - Theme.paddingSmall * 2
        anchors.verticalCenter: parent.verticalCenter

        Label {
            text: title
            width: parent.width
            color: backgroundItem.highlighted ? Theme.highlightColor : Theme.highlightDimmerColor
            font.pixelSize: Theme.fontSizeExtraSmall
            font.weight: Font.Normal
            truncationMode: TruncationMode.Fade
        }
        Label {
            text: url
            width: parent.width
            color: backgroundItem.highlighted ? Theme.highlightColor : Theme.highlightDimmerColor
            font.pixelSize: Theme.fontSizeTiny
            font.weight: Font.Normal
            truncationMode: TruncationMode.Fade
        }
    }
}
