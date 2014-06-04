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
        for (var i = 0; i < selectedContent.count; i++) {
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
            case FileUploadFilter.FilterImages:
                pageStack.push(multiImagePickerComponent)
                break
            case FileUploadFilter.FilterAudio:
                pageStack.push(multiMusicPickerComponent)
                break
            case FileUploadFilter.FilterVideo:
                pageStack.push(multiVideoPickerComponent)
                break
            default:
                pageStack.push(multiContentPickerComponent)
            }
        } else {
            switch (filter) {
            case FileUploadFilter.FilterImages:
                pageStack.push(imagePickerComponent)
                break
            case FileUploadFilter.FilterAudio:
                pageStack.push(musicPickerComponent)
                break
            case FileUploadFilter.FilterVideo:
                pageStack.push(videoPickerComponent)
                break
            default:
                pageStack.push(contentPickerComponent)
            }
        }
    }


    Component  {
        id: imagePickerComponent
        ImagePickerPage {
            //% "Upload image"
            title: qsTrId("sailfish_browser-he-upload_image")
            Component.onDestruction: sendResponse(selectedContent)
        }
    }

    Component  {
        id: multiImagePickerComponent
        MultiImagePickerDialog {
            //% "Upload images"
            title: qsTrId("sailfish_browser-he-upload_images")
            Component.onDestruction: sendResponseList(selectedContent)
        }
    }


    Component  {
        id: contentPickerComponent
        ContentPickerPage {
            //% "Upload file"
            title: qsTrId("sailfish_browser-he-upload_file")
            Component.onDestruction: sendResponse(selectedContent)
        }
    }

    Component  {
        id: multiContentPickerComponent
        MultiContentPickerDialog {
            //% "Upload files"
            title: qsTrId("sailfish_browser-he-upload_files")
            Component.onDestruction: sendResponseList(selectedContent)
        }
    }

    Component  {
        id: videoPickerComponent
        VideoPickerPage {
            //% "Upload video"
            title: qsTrId("sailfish_browser-he-upload_video")
            Component.onDestruction: sendResponse(selectedContent)
        }
    }

    Component  {
        id: multiVideoPickerComponent
        MultiVideoPickerDialog {
            //% "Upload videos"
            title: qsTrId("sailfish_browser-he-upload_videos")
            Component.onDestruction: sendResponseList(selectedContent)
        }
    }

    Component  {
        id: musicPickerComponent
        MusicPickerPage {
            //% "Upload audio file"
            title: qsTrId("sailfish_browser-he-upload_audio")
            Component.onDestruction: sendResponse(selectedContent)
        }
    }

    Component  {
        id: multiMusicPickerComponent
        MultiMusicPickerDialog {
            //% "Upload audio files"
            title: qsTrId("sailfish_browser-he-upload_audio_files")
            Component.onDestruction: sendResponseList(selectedContent)
        }
    }
}
