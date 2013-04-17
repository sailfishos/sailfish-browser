/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 1.1
import Sailfish.Silica 1.0

CoverBackground {
    anchors.fill: parent

    CoverPlaceholder {
        //% "Browser"
        text: qsTrId("sailfish_browser-ap-browser")
        icon.source: 'image://theme/icon-launcher-browser'
    }

    Label {
        id: label
        anchors.centerIn: parent
        //% "Browser"
        text: qsTrId("sailfish_browser-la-browser")
        color: theme.secondaryColor
    }
}


