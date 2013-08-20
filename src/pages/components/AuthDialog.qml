/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/


import QtQuick 2.0
import Sailfish.Silica 1.0

UserPrompt {
    id: dialog

    property string hostname
    property string realm
    property bool passwordOnly
    property alias username: username.text
    property alias password: password.text

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
            EnterKey.enabled: text.length > 0
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
            EnterKey.enabled: text.length > 0
            EnterKey.onClicked: {
                if (username.text.length > 0) {
                    dialog.accept()
                } else {
                    username.focus = true
                }
            }
        }
    }
}
