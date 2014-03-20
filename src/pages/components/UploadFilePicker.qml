/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0

ContentPickerPage {

    property int winid
    property QtObject webView

    //% "Upload file"
    title: qsTrId("sailfish_browser-he-upload_file")

    Component.onDestruction: {
        var scheme = "file://"
        var filePath = selectedContent.toString()

        if (filePath.indexOf(scheme) === 0) {
            filePath = filePath.slice(scheme.length, filePath.length)
        }

        webView.sendAsyncMessage("filepickerresponse",
                                 {
                                     "winid": winid,
                                     "accepted": filePath ? true : false,
                                     "items": [filePath]
                                 })
    }
}
