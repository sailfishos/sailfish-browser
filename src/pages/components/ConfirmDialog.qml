/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/


import QtQuick 1.1
import Sailfish.Silica 1.0

Dialog {
    property alias text: label.text

    DialogHeader {
        //: Text on the Accept dialog button that accepts browser's confirm() messages
        //% "Ok"
        acceptText: qsTrId("sailfish_browser-he-accept_confirm")
    }

    Label {
        id: label

        anchors.centerIn: parent
        width: parent.width - (2 * theme.paddingMedium)
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
    }
}