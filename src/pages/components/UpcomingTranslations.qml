/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0

// The file is not used anywhere. This is a placeholder for upcoming translation strings.
Item {
    //: Flick down to reveal the toolbar
    //% "Flick down to reveal the toolbar"
    property string toolbarHint: qsTrId("sailfish_browser-la-toolbar_hint")

    //: Hello, here's few tips about the browser toolbar
    //% "Hello, here's few tips about the browser toolbar"
    property string firstUseHeader: qsTrId("sailfish_browser-he-first_use")

    //: Tab to go to previous or next page
    //% "Tab to go to previous or next page"
    property string navigationHint: qsTrId("sailfish_browser-la-first_use_navigation_hint")

    //: Mark a page as favorite for easy access later on
    //% "Mark a page as favorite for easy access later on"
    property string bookmarkHint: qsTrId("sailfish_browser-la-first_use_bookmark_hint")

    //: Switch between open tabs, edit the address of the current one or open a bookmark
    //% "Switch between open tabs, edit the address of the current one or open a bookmark"
    property string tabsHint: qsTrId("sailfish_browser-la-first_use_tabs_hint")

    //: Reload the page
    //% "Reload the page"
    property string reloadHint: qsTrId("sailfish_browser-la-first_reload_hint")

    //: Now, try moving this text up to hide the toolbar.
    //% "Now, try moving this text up to hide the toolbar."
    property string hideToolbarHint: qsTrId("sailfish_browser-la-first_hide_toolbar_hint")
}
