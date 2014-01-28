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

Rectangle {
    color: "white"

    Image {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom

        source: "../../content/graphic-browsertutorial.png"

        /* Future use
        Label {
            anchors.centerIn: parent

            //% "Tap here to search or switch between tabs"
            text: qsTrId("sailfish_browser-la-first_use_welcome")
        }
        */
    }
}
