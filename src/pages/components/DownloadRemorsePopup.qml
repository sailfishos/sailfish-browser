/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/


import QtQuick 2.0
import Sailfish.Silica 1.0

RemorsePopup {
    id: popup

    property int _currentDownloadId

    onCanceled: {
        MozContext.sendObserve("embedui:download",
                               {
                                   "msg": "cancelDownload",
                                   "id": _currentDownloadId
                               })
    }

    Connections {
        target: MozContext

        onRecvObserve: {
            if (message === "embed:download") {
                switch (data.msg) {
                    case "dl-start": {
                        popup._currentDownloadId = data.id
                        //% "Downloading %1"
                        popup.execute(qsTrId("sailfish_browser-no-downloading").arg(data.displayName))
                        break
                    }
                    case "dl-fail":
                    case "dl-done": {
                        if (popup.visible && popup._currentDownloadId === data.id) {
                            popup.visible = false
                        }
                        break
                    }
                }
            }
        }
    }
}

