/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0

UserPrompt {
    property alias text: label.text

    //: Text on the Accept dialog button that accepts browser alert messages
    //% "Ok"
    acceptText: qsTrId("sailfish_browser-he-accept_alert")

    PromptLabel {
        id: label
    }
}
