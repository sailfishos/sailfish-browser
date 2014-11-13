/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0

Component {
    ContextMenu {
        property Item view
        property Item delegate
        property string title
        property string url

        MenuItem {
            //% "Open in new tab"
            text: qsTrId("sailfish_browser-me-open_new_tab")
            onClicked: view.newTab(url, title)
        }

        MenuItem {
            //: Share link from browser pulley menu
            //% "Share"
            text: qsTrId("sailfish_browser-me-share_link")
            onClicked: view.share(url, title)
        }

        MenuItem {
            //: Add bookmark to launcher menu item. This should be relatively short to fit to the menu.
            //% "Add to launcher"
            text: qsTrId("sailfish_browser-me-add_to_launcher")
            onClicked: delegate.addToLauncher()
        }

        MenuItem {
            //% "Edit favorite"
            text: qsTrId("sailfish_browser-me-edit_favorite")
            onClicked: delegate.editBookmark()
        }

        MenuItem {
            //: "Remove favorited / bookmarked web page"
            //% "Remove favorite"
            text: qsTrId("sailfish_browser-me-remove_favorite")
            onClicked: delegate.remove()
        }
    }
}
