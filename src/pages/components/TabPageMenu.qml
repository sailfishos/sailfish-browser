/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0
import ".."

Loader {
    id: tabPageMenuLoader
    property BrowserPage browserPage
    property Item flickable
    property bool shareEnabled

    signal addToLauncher

    asynchronous: true
    sourceComponent: PullDownMenu {
        opacity: 0.0
        flickable: tabPageMenuLoader.flickable

        Behavior on opacity { FadeAnimation {} }

        MenuItem {
            //% "Close all tabs"
            text: qsTrId("sailfish_browser-me-close_all")
            onClicked: browserPage.tabs.clear()
        }
        MenuItem {
            enabled: shareEnabled
            //: Share link from browser pulley menu
            //% "Share"
            text: qsTrId("sailfish_browser-me-share_link")
            onClicked: pageStack.push(Qt.resolvedUrl("../ShareLinkPage.qml"), {"link" : browserPage.url, "linkTitle": browserPage.title})
        }
        MenuItem {
            //% "New tab"
            text: qsTrId("sailfish_browser-me-new_tab")
            onClicked: pageStack.push(Qt.resolvedUrl("../TabPage.qml"), {"newTab": true, "browserPage": browserPage})
        }

        MenuItem {
            enabled: shareEnabled
            //: Add bookmark to launcher menu item. This should be relatively short to fit to the menu.
            //% "Add to launcher"
            text: qsTrId("sailfish_browser-me-add_to_launcher")
            onClicked: tabPageMenuLoader.addToLauncher()
        }

        Component.onCompleted: opacity = 1.0
    }
}
