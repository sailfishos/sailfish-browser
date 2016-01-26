/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


import QtQuick 2.0
import Sailfish.Silica 1.0

UserPrompt {
    id: dialog

    property string host

    //: Allow the server to use location
    //% "Allow"
    acceptText: qsTrId("sailfish_browser-he-accept_location")

    Label {
        x: Theme.paddingLarge
        width: parent.width - Theme.paddingLarge * 2
        //: The site that wants know user location
        //% "The site '%1' wants to use your current location."
        text: qsTrId("sailfish_browser-la-location_requested").arg(host)
        wrapMode: Text.WordWrap
        color: Theme.highlightColor
    }
}
