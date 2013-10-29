/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/


import QtQuick 2.0
import Sailfish.Silica 1.0

UserPrompt {
    id: dialog

    //: Allow the server to use location
    //% "Allow"
    acceptText: qsTrId("sailfish_browser-he-accept_location")

    Label {
        x: Theme.paddingLarge
        width: parent.width - Theme.paddingLarge * 2
        //: The site that wants know user location
        //% "The site wants to use your current location."
        text: qsTrId("sailfish_browser-la-location_requested")
        wrapMode: Text.WordWrap
        color: Theme.highlightColor
    }
}
