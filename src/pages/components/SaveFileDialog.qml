/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/


import QtQuick 1.1
import Sailfish.Silica 1.0

Dialog {
    property string fileName
    property bool alreadyExists: false

    DialogHeader {
        //: Text on the Accept dialog button that accepts browser alert messages
        //% "Download"
        acceptText: qsTrId("sailfish_browser-he-accept_download")
    }

    Column {
        anchors.centerIn: parent
        width: parent.width - (2 * theme.paddingMedium)
        spacing: theme.paddingMedium

        Label {
            width: parent.width
            wrapMode: Text.Wrap
            visible: alreadyExists
            horizontalAlignment: Text.AlignHCenter
            //% "The file exists already. Do you want to override?"
            text: qsTrId("sailfish_browser-la-override_download")
        }

        Label {
            width: parent.width
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            //% "Please accept if you want to download %1"
            text: qsTrId("sailfish_browser-la-accept_download").arg(fileName)
        }
    }
}