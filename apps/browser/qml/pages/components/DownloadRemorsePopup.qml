/****************************************************************************
**
** Copyright (c) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0

RemorsePopup {
    id: popup

    property int currentDownloadId

    //% "Tap to cancel"
    cancelText: qsTrId("sailfish_browser-la-tap_to_cancel")
    onCanceled: DownloadManager.cancel(currentDownloadId)

    Connections {
        target: DownloadManager

        // Arguments downloadId, status, info
        onDownloadStatusChanged: {
            switch (status) {
            case DownloadStatus.Started:
                popup.currentDownloadId = downloadId
                //% "Downloading %1"
                popup.execute(qsTrId("sailfish_browser-no-downloading").arg(info.displayName))
                break
            case DownloadStatus.Failed:
            case DownloadStatus.Done:
                if (popup.visible && popup.currentDownloadId === downloadId) {
                    popup.visible = false
                }
                break
            }
        }
    }
}

