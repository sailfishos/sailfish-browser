/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Siteshwar Vashisht <siteshwar AT gmail.com>
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import Sailfish.Silica 1.0

ConfirmDialog {
    property Item browserPage

    //: Warning of changing browser configurations.
    //% "Changing these advanced settings can cause issues with stability, security and performance of Sailfish Browser. Continue ?"
    text: qsTrId("sailfish_browser-la-config-warning");
    acceptDestination: Component {
        ConfigDialog {
            // On accept pop back to browserPage
            acceptDestination: browserPage
            acceptDestinationAction: PageStackAction.Pop
        }
    }
}
