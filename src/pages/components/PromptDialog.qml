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

    property alias text: input.placeholderText
    property alias value: input.text

    canAccept: input.text.length > 0
    //: Text on the Accept dialog button that accepts browser's prompt() messages
    //% "Ok"
    acceptText: qsTrId("sailfish_browser-he-accept_prompt")

    TextField {
        id: input

        anchors.centerIn: parent
        width: parent.width
        focus: true
        label: text.length > 0 ? dialog.text : ""
        inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
        EnterKey.enabled: text.length > 0
        EnterKey.onClicked: dialog.accept()
    }
}
