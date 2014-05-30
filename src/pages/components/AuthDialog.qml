/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


import QtQuick 2.0
import Sailfish.Silica 1.0

UserPrompt {
    id: dialog

    property string hostname
    property string realm
    property bool passwordOnly
    property alias username: username.text
    property alias password: password.text
    property alias dontsave: dontsaveCheck.checked

    canAccept: username.text.length > 0
    //: Text on the Accept dialog button that accepts browser's auth request
    //% "Log In"
    acceptText: qsTrId("sailfish_browser-he-accept_login")

    Column {
        width: parent.width
        spacing: Theme.paddingMedium

        Label {
            x: Theme.paddingLarge
            width: parent.width - Theme.paddingLarge * 2
            //: %1 is server URL, %2 is HTTP auth realm
            //% "The server %1 requires authentication. The server says: %2"
            text: qsTrId("sailfish_browser-la-auth_requested").arg(hostname).arg(realm)
            wrapMode: Text.Wrap
            color: Theme.highlightColor
        }

        TextField {
            id: username

            width: parent.width
            visible: !passwordOnly
            focus: !passwordOnly
            //% "Enter your user name"
            placeholderText: qsTrId("sailfish_browser-la-enter_username")
            //% "User name"
            label: qsTrId("sailfish_browser-la-user_name")
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
            EnterKey.iconSource: "image://theme/icon-m-enter-next"
            EnterKey.onClicked: password.focus = true
        }

        TextField {
            id: password

            width: parent.width
            focus: passwordOnly
            echoMode: TextInput.Password
            //% "Enter password"
            placeholderText: qsTrId("sailfish_browser-la-enter_password")
            //% "Password"
            label: qsTrId("sailfish_browser-la-password")
            EnterKey.iconSource: (username.text.length > 0 && text.length > 0) ? "image://theme/icon-m-enter-accept"
                                                                               : "image://theme/icon-m-enter-next"
            EnterKey.onClicked: {
                if (username.text.length > 0) {
                    dialog.accept()
                } else {
                    username.focus = true
                }
            }
        }

        TextSwitch {
            id: dontsaveCheck

            checked: false

            //: Do not save the entered credentials for later use
            //% "Do not save"
            text: qsTrId("sailfish_browser-la-dont_save")

            //: Explain to the user that checking the switch will prevent entered credentials from saving
            //% "The credentials you enter won't be stored for later use"
            description: qsTrId("sailfish_browser-la-dont_save_description")
        }
    }
}
