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
        anchors.left: parent.left
        anchors.leftMargin: theme.paddingMedium
        y: dialog.height / 2
        spacing: theme.paddingSmall

        Label {
            id: label
        }

        TextField {
            id: input

            focus: true
        }
    }
}