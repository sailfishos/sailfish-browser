/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/


import QtQuick 1.1
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    property string hostname
    property string realm
    property bool passwordOnly
    property alias username: username.text
    property alias password: password.text

    DialogHeader {
        //: Text on the Accept dialog button that accepts browser's auth request
        //% "Log In"
        acceptText: qsTrId("sailfish_browser-he-accept_login")
    }

    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: theme.paddingMedium
        width: parent.width - (2 * theme.paddingMedium)
        spacing: theme.paddingSmall

        Label {
            width: parent.width
            //: %1 is server URL, %2 is HTTP auth realm
            //% "The server %1 requires authentication. The server says: %2"
            text: qsTrId("sailfish_browser-la-auth_requested").arg(hostname).arg(realm)
            wrapMode: Text.Wrap
        }

        Label {
            width: parent.width
            //% "User name:"
            text: qsTrId("sailfish_browser-la-user_name")
            visible: !passwordOnly
        }

        TextField {
            id: username

            width: parent.width
            visible: !passwordOnly
            focus: !passwordOnly
            //% "Enter your user name"
            placeholderText: qsTrId("sailfish_browser-la-enter_username")
        }

        Label {
            width: parent.width
            //% "Password:"
            text: qsTrId("sailfish_browser-la-password")
        }

        TextField {
            id: password

            width: parent.width
            focus: passwordOnly
            echoMode: TextInput.Password
            //% "Enter password"
            placeholderText: qsTrId("sailfish_browser-la-enter_password")
        }
    }
}