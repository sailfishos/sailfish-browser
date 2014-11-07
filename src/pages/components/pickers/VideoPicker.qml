/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Pickers 1.0

VideoPickerPage {
    property var creator

    //: For choosing video to send to the website from the device
    //% "Upload video"
    title: qsTrId("sailfish_browser-he-upload_video")
    Component.onDestruction: creator.sendResponse(selectedContent)
}
