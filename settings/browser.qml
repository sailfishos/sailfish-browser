/****************************************************************************
**
** Copyright (c) 2013 - 2015 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.browser.settings 1.0
import Nemo.DBus 2.0
import com.jolla.settings 1.0

ApplicationSettings {
    Button {
        anchors.horizontalCenter: parent.horizontalCenter
        //% "Settings"
        text: qsTrId("settings_browser-la-settings")
        onClicked: browserApp.load("about:settings")
    }

    DBusInterface {
        id: browserApp

        service: "org.sailfishos.browser.ui"
        path: "/ui"
        iface: "org.sailfishos.browser.ui"

        function load(url) {
            call("openSettings", [], function() {
            }, function(error, message) {
                console.warn("Failed to open settings:", url, "error:", error, "message:", message)
            })
        }
    }
}
