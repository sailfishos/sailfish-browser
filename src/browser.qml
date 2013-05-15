/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 1.1
import Sailfish.Silica 1.0
import Sailfish.TransferEngine 1.0
import "pages"

ApplicationWindow {
    id: window

    // TODO: unlike Gecko downloads and Sailfish transfers these mappings
    //       are not persistent -> after user has browser closed transfers can't be
    //       restarted.
    property variant download2transferMap: {}
    property variant transfer2downloadMap: {}

    initialPage: Component {BrowserPage {}}
    cover: undefined

    function handleDownloadMessage(data) {
        var dMap = download2transferMap
        var tMap = transfer2downloadMap
        var transferId

        switch (data.msg) {
            case "dl-start": {
                transferId = transferInterface.createDownloadEvent(data.displayName,
                                                                   "image://theme/icon-launcher-browser",
                                                                   "image://theme/icon-launcher-browser",
                                                                   data.sourceUrl,
                                                                   data.mimeType, data.size,
                                                                   ["org.sailfishos.browser", "/", "org.sailfishos.browser"], "cancelTransfer", "restartTransfer")

                dMap[data.id] = transferId
                download2transferMap = dMap
                tMap[transferId] = data.id
                transfer2downloadMap = tMap

                transferInterface.startTransfer(transferId)
                break
            }
            case "dl-progress": {
                var progress = data.percent / 100
                transferInterface.updateTransferProgress(download2transferMap[data.id], progress)
                break
            }
            case "dl-done": {
                transferInterface.finishTransfer(download2transferMap[data.id],
                                                 SailfishTransferInterface.TransferFinished, "")
                break
            }
            case "dl-fail": {
                transferInterface.finishTransfer(download2transferMap[data.id],
                                                 SailfishTransferInterface.TransferInterrupted,
                                                 //% "Browser failed to download this file"
                                                 qsTrId("sailfish_browser-la-failed_to_download"))
                break
            }
            case "dl-cancel": {
                transferInterface.finishTransfer(download2transferMap[data.id],
                                                 SailfishTransferInterface.TransferCanceled,
                                                 //% "Download canceled"
                                                 qsTrId("sailfish_browser-la-download_canceled"))
                break
            }
        }
    }

    // Handlers for application-wide events
    Connections {
        target: MozContext

        onRecvObserve: {
            switch (message) {
                case "embed:download": {
                    handleDownloadMessage(data)
                    break
                }
            }
        }
    }

    Connections {
        target: WebUtils

        onCancelTransferRequested: {
            if (transfer2downloadMap[transferId] !== undefined) {
                MozContext.sendObserve("embedui:download",
                                       {
                                           "msg": "cancelDownload",
                                           "id": transfer2downloadMap[transferId]
                                       })
            }
        }

        onRestartTransferRequested: {
            if (transfer2downloadMap[transferId] !== undefined) {
                MozContext.sendObserve("embedui:download",
                                       {
                                           "msg": "retryDownload",
                                           "id": transfer2downloadMap[transferId]
                                       })
            } else {
                 console.log("TODO: remove unknown transfer from TransferEngine")
            }
        }
    }

    SailfishTransferInterface {
        id: transferInterface
    }
}

