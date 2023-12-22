/****************************************************************************
**
** Copyright (c) 2013 - 2021 Jolla Ltd.
** Copyright (c) 2019 - 2020 Open Mobile Platform LLC.
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

    function setBrowserCover(model) {
        if (!model || model.count === 0 || !WebUtils.firstUseDone) {
            cover = Qt.resolvedUrl("cover/NoTabsCover.qml")
        } else {
            if (cover != null && window.webView) {
                window.webView.clearSurface()
            }
            cover = null
        }
    }

    //% "Web browsing"
    activityDisabledByMdm: qsTrId("sailfish_browser-la-web_browsing")
    initialPage: Component {
        BrowserPage {
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
