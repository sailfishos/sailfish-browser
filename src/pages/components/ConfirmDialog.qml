/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/


import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0

UserPrompt {
    property alias text: label.text

    //: Text on the Accept dialog button that accepts browser's confirm() messages
    //% "Ok"
    acceptText: qsTrId("sailfish_browser-he-accept_confirm")

    Label {
        id: label

        anchors.centerIn: parent
        width: parent.width - (2 * Theme.paddingLarge)
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
        color: Theme.highlightColor
    }
}
