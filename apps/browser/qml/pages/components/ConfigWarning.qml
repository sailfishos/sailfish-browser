/****************************************************************************
**
** Copyright (c) 2014 Jolla Ltd.
** Contact: Siteshwar Vashisht <siteshwar AT gmail.com>
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import Sailfish.Silica 1.0
import Sailfish.WebView.Popups 1.0

Dialog {
    property Item browserPage

    acceptDestination: Component {
        ConfigDialog {
            // On accept pop back to browserPage
            acceptDestination: browserPage
            acceptDestinationAction: PageStackAction.Pop
        }
    }

    Column {
        width: parent.width

        DialogHeader {}

        Label {
            x: Theme.horizontalPageMargin
            y: Theme.itemSizeSmall
            width: parent.width - 2 * x
            font.pixelSize: Theme.fontSizeMedium
            color: Theme.highlightColor
            wrapMode: Text.Wrap

            //: Warning of changing browser configurations.
            //% "Changing these advanced settings can cause issues with stability, "
            //% "security and performance of Sailfish Browser. Continue ?"
            text: qsTrId("sailfish_browser-la-config-warning")
        }
    }
}
