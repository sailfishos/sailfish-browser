/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.WebView.Controls 1.0

Dialog {
    id: dialog

    property string host

    property alias rememberValue: remember.checked

    Column {
        width: parent.width
        spacing: Theme.paddingMedium

        DialogHeader {
            //: Allow the site to open a popup window
            //% "Allow"
            acceptText: qsTrId("sailfish_browser-he-load_popup")

            //: Deny the site from opening a popup window
            //% "Deny"
            cancelText: qsTrId("sailfish_browser-he-block_popup")
        }

        Label {
            x: Theme.horizontalPageMargin
            width: parent.width - x * 2

            //: %1 is the site that wants to load a popup
            //% "The site %1 has requested to open a new tab. Would you like to allow it to open new tabs?"
            text: qsTrId("sailfish_browser-la-popup_request").arg(dialog.host)

            wrapMode: Text.WordWrap
            color: Theme.highlightColor
        }

        TextSwitch {
            id: remember

            //: Remember decision for this site for later use
            //% "Remember for this site"
            text: qsTrId("sailfish_browser-remember_for_site")
            //% "You can change this setting in future by selecting the padlock icon in the toolbar, or from the Permissions page in Settings"
            description: qsTrId("sailfish_browser-rembember_description")
        }
    }
    onAccepted: {
        if (rememberValue) {
            PermissionManager.add(dialog.host, "popup", PermissionManager.Allow, PermissionManager.Never)
        } else {
            PermissionManager.add(dialog.host, "popup", PermissionManager.Allow, PermissionManager.Session)
        }
    }
    onRejected: {
        if (rememberValue) {
            PermissionManager.add(dialog.host, "popup", PermissionManager.Deny)
        }
    }
}
