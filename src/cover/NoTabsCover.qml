/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0

CoverBackground {
    CoverActionList {
        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: window.newTab()
        }
    }

    CoverPlaceholder {
        //: Create a new tab cover text
        //% "Create a new tab"
        text: qsTrId("sailfish_browser-he-create_new_tab")
        icon.source: "image://theme/icon-launcher-browser"
    }
}
