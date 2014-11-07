/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Pickers 1.0
import Sailfish.Browser 1.0

Item {
    id: pickerCreator
    property int winid
    property QtObject webView
    property int filter: FileUploadFilter.FilterAll
    property int mode
    property Item pageStack

    function sendResponse(selectedContent) {
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
        pickerCreator.destroy()
    }

    function sendResponseList(selectedContent) {
        var scheme = "file://"
        var result = []
        for (var i = 0; selectedContent && i < selectedContent.count; i++) {
            var filePath = selectedContent.get(i).filePath
            if (filePath.indexOf(scheme) === 0) {
                filePath = filePath.slice(scheme.length, filePath.length)
            }
            result.push(filePath)
        }

        webView.sendAsyncMessage("filepickerresponse",
                                 {
                                     "winid": winid,
                                     "accepted": result.length > 0,
                                     "items": result
                                 })
        pickerCreator.destroy()
    }

    Component.onCompleted: {
        if (mode == FileUploadMode.OpenMultiple) {
            switch (filter) {
            case FileUploadFilter.Image:
                pageStack.push(Qt.resolvedUrl("pickers/MultiImagePicker.qml"), {"creator": pickerCreator})
                break
            case FileUploadFilter.Audio:
                pageStack.push(multiMusicPickerComponent)
                break
            case FileUploadFilter.Video:
                pageStack.push(multiVideoPickerComponent)
                break
            default:
                pageStack.push(Qt.resolvedUrl("pickers/MultiContentPicker.qml"), {"creator": pickerCreator})
            }
        } else {
            switch (filter) {
            case FileUploadFilter.Image:
                pageStack.push(Qt.resolvedUrl("pickers/ImagePicker.qml"), {"creator": pickerCreator})
                break
            case FileUploadFilter.Audio:
                pageStack.push(musicPickerComponent)
                break
            case FileUploadFilter.Video:
                pageStack.push(Qt.resolvedUrl("pickers/VideoPicker.qml"), {"creator": pickerCreator})
                break
            default:
                pageStack.push(Qt.resolvedUrl("pickers/ContentPicker.qml"), {"creator": pickerCreator})
            }
        }
    }

    Component  {
        id: multiVideoPickerComponent
        MultiVideoPickerDialog {
            //: For choosing videos to send to the website from the device
            //% "Upload videos"
            title: qsTrId("sailfish_browser-he-upload_videos")
            Component.onDestruction: sendResponseList(selectedContent)
        }
    }

    Component  {
        id: musicPickerComponent
        MusicPickerPage {
            //: For choosing audio file to send to the website from the device
            //% "Upload audio file"
            title: qsTrId("sailfish_browser-he-upload_audio")
            Component.onDestruction: sendResponse(selectedContent)
        }
    }

    Component  {
        id: multiMusicPickerComponent
        MultiMusicPickerDialog {
            //: For choosing audio files to send to the website from the device
            //% "Upload audio files"
            title: qsTrId("sailfish_browser-he-upload_audio_files")
            Component.onDestruction: sendResponseList(selectedContent)
        }
    }
}
