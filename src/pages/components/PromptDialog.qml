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

    property alias text: label.text
    property alias value: input.text

    DialogHeader {
        //: Text on the Accept dialog button that accepts browser's prompt() messages
        //% "Ok"
        acceptText: qsTrId("sailfish_browser-he-accept_prompt")
    }

    Column {
        anchors.centerIn: parent
        width: parent.width - (2 * theme.paddingMedium)
        spacing: theme.paddingSmall

        Label {
            id: label

            width: parent.width
            wrapMode: Text.Wrap
        }

        TextField {
            id: input

            focus: true
            width: parent.width
        }
    }
}