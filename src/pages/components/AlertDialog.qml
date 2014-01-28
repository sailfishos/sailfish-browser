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
    property alias text: label.text

    //: Text on the Accept dialog button that accepts browser alert messages
    //% "Ok"
    acceptText: qsTrId("sailfish_browser-he-accept_alert")

    PromptLabel {
        id: label
    }
}
