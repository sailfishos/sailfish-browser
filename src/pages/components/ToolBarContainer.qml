/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0

Rectangle {
    color: Theme.highlightBackgroundColor
    // Height of toolbar should be such that viewport height is
    // even number both chrome and fullscreen modes. For instance
    // height of 110px for toolbar would result would result 1px rounding
    // error in chrome mode as viewport height would be 850px. This would
    // result in CSS pixels viewport height of 566.66..px -> rounded to 566px.
    // So, check that (device height - toolbar height) / pixel ratio is even number.
    height: Theme.itemSizeMedium * 1.2

    MouseArea {
        anchors.fill: parent
        onClicked: {}
    }
}
