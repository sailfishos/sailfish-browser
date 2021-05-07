/****************************************************************************
**
** Copyright (c) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


import QtQuick 2.1
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0

ApplicationWindow {
    id: window

    property alias webView: webView
    property alias tabModel: webView.tabModel
    property alias historyModel: historyModel

    property bool completed

    readonly property bool testSetupReady: webView && webView.completed && window.completed

    allowedOrientations: Orientation.Portrait
    _defaultPageOrientations: allowedOrientations

    // To react to window change.
    Item {
        onWindowChanged: {
            if (window) {
                webView.chromeWindow = window
            }
        }
    }

    initialPage: Page {
        id: browserPage

        // Mimic the real BrowserPage.qml
        readonly property bool largeScreen: false
        readonly property size thumbnailSize: Qt.size(100, 100)

        WebView {
            id: webView

            portrait: true
            toolbarHeight: 0
            fullscreenHeight: Screen.height
            width: window.width
            height: window.height
            privateMode: false
            rotationHandler: browserPage
            historyModel: historyModel
        }
        HistoryModel {
            id: historyModel
        }
    }

    cover: undefined

    Component.onCompleted: completed = true
}

