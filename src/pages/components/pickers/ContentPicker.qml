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

ContentPickerPage {
    property var creator

    //: For choosing any file (document/image/video/audio) to send to the website from the device
    //% "Upload file"
    title: qsTrId("sailfish_browser-he-upload_file")
    Component.onDestruction: creator.sendResponse(selectedContent)
}
