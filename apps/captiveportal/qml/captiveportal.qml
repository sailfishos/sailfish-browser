/****************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC.
** Copyright (c) 2021 Jolla Ltd.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import "pages"
import "shared"

BrowserWindow {
    id: window

    //% "Captive portal"
    activityDisabledByMdm: qsTrId("sailfish_captiveportal-la-captive_portal");
    coverMode: false
    initialPage: Component {
        CaptivePortalPage {
            id: browserPage

            Component.onCompleted: {
                window.webView = webView
                window.rootPage = browserPage
            }

            Component.onDestruction: {
                window.webView = null
            }
        }
    }
}
