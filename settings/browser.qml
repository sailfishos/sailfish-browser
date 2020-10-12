/****************************************************************************
**
** Copyright (c) 2013 - 2015 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.dbus 2.0

Page {
    id: page

    Connections {
        target: pageStack
        onCurrentPageChanged: {
            if (pageStack.currentPage === page) {
                window.deactivate()
                browserApp.load("about:settings")
            }
        }
        onBusyChanged: {
            if (pageStack.busy === false && pageStack.currentPage === page) {
                pageStack.pop()
            }
        }
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
