/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

.pragma library
.import QtQuick 2.1 as QtQuick
.import Sailfish.Browser 1.0 as Browser

var webView
var popups
var pageStack
var auxTimer
var contextMenuComponent
var resourceController
var tabModel
// TODO: WebUtils context property. Should be singleton.
var WebUtils
var pickerCreator

// TODO: Handle these per QmlMozView (map of webviews + accepted/rejectedGeolocationUrl)
var acceptedGeolocationUrl = ""
var rejectedGeolocationUrl = ""

var _authenticationComponentUrl = Qt.resolvedUrl("AuthDialog.qml")
var _passwordManagerComponentUrl = Qt.resolvedUrl("PasswordManagerDialog.qml")
var _contextMenuComponentUrl = Qt.resolvedUrl("BrowserContextMenu.qml")
var _selectComponentUrl = Qt.resolvedUrl("SelectDialog.qml")
var _locationComponentUrl = Qt.resolvedUrl("LocationDialog.qml")
var _alertComponentUrl = Qt.resolvedUrl("AlertDialog.qml")
var _confirmComponentUrl = Qt.resolvedUrl("ConfirmDialog.qml")
var _queryComponentUrl = Qt.resolvedUrl("PromptDialog.qml")
var _uploadFilePickerComponentUrl = Qt.resolvedUrl("UploadFilePicker.qml")


// Singleton
var _contextMenu

// As QML can't disconnect closure from a signal (but methods only)
// let's keep auth data in this auxilary attribute whose sole purpose is to
// pass arguments to openAuthDialog().
var _authData = null

function _hideVirtualKeyboard() {
    if (Qt.inputMethod.visible) {
        webView.parent.focus = true
    }
}

function isAcceptedGeolocationUrl(url) {
    var tmpUrl = WebUtils.displayableUrl(url)
    return  acceptedGeolocationUrl === tmpUrl
}

function isRejectedGeolocationUrl(url) {
    var tmpUrl = WebUtils.displayableUrl(url)
    return  rejectedGeolocationUrl === tmpUrl
}

function openAuthDialog(input) {
    if (pageStack.busy) {
        // User has just entered wrong credentials and webView wants
        // user's input again immediately even thogh the accepted
        // dialog is still deactivating.
        _authData = input
        // A better solution would be to connect to browserPage.statusChanged,
        // but QML Page transitions keep corrupting even
        // after browserPage.status === PageStatus.Active thus auxTimer.
        auxTimer.triggered.connect(openAuthDialog)
        auxTimer.start()
    } else {
        var data = input !== undefined ? input : _authData
        var winid = data.winid

        if (_authData !== null) {
            auxTimer.triggered.disconnect(openAuthDialog)
            _authData = null
        }

        var dialog = pageStack.push(_authenticationComponentUrl,
                                    {
                                        "hostname": data.text,
                                        "realm": data.title,
                                        "username": data.storedUsername,
                                        "password": data.storedPassword,
                                        "passwordOnly": data.passwordOnly
                                    })
        dialog.accepted.connect(function () {
            webView.sendAsyncMessage("authresponse",
                                           {
                                               "winid": winid,
                                               "accepted": true,
                                               "username": dialog.username,
                                               "password": dialog.password,
                                               "dontsave": dialog.dontsave
                                           })
        })
        dialog.rejected.connect(function() {
            webView.sendAsyncMessage("authresponse",
                                           {"winid": winid, "accepted": false})
        })
    }
}

function openSelectDialog(data) {
    var dialog = pageStack.push(_selectComponentUrl,
                                {
                                    "options": data.options,
                                    "multiple": data.multiple,
                                    "webview": webView.contentItem
                                })
}

function openPasswordManagerDialog(data) {
    pageStack.push(_passwordManagerComponentUrl,
                   {
                       "webView": webView.contentItem,
                       "requestId": data.id,
                       "notificationType": data.name,
                       "formData": data.formdata
                   })
}

function openContextMenu(data) {
    // Possible path that leads to a new tab. Thus, capturing current
    // view before opening context menu.
    webView.captureScreen()
    webView.contentItem.contextMenuRequested(data)
    if (data.types.indexOf("image") !== -1 || data.types.indexOf("link") !== -1) {
        var linkHref = data.linkURL
        var imageSrc = data.mediaURL
        var linkTitle = data.linkTitle
        var contentType = data.contentType
        if (_contextMenu) {
            _contextMenu.linkHref = linkHref
            _contextMenu.linkTitle = linkTitle.trim()
            _contextMenu.imageSrc = imageSrc
            _hideVirtualKeyboard()
            _contextMenu.show()
        } else {
            contextMenuComponent = Qt.createComponent(_contextMenuComponentUrl)
            if (contextMenuComponent.status !== QtQuick.Component.Error) {
                _contextMenu = contextMenuComponent.createObject(webView.parent,
                                                        {
                                                            "linkHref": linkHref,
                                                            "imageSrc": imageSrc,
                                                            "linkTitle": linkTitle.trim(),
                                                            "contentType": contentType,
                                                            "tabModel": tabModel,
                                                            "viewId": webView.contentItem.uniqueID(),
                                                            "pageStack": pageStack
                                                        })
                _hideVirtualKeyboard()

                webView.popupActive = Qt.binding(function() { return (_contextMenu.active) })
                _contextMenu.show()
            } else {
                console.log("Can't load BrowserContextMenu.qml")
            }
        }
    }
}

function openLocationDialog(data) {
    // Ask for location permission
    var url = webView.contentItem.url
    if (isAcceptedGeolocationUrl(url)) {
        webView.sendAsyncMessage("embedui:premissions", {
                             allow: true,
                             checkedDontAsk: false,
                             id: data.id })
    } else if (isRejectedGeolocationUrl(url)) {
        webView.sendAsyncMessage("embedui:premissions", {
                             allow: false,
                             checkedDontAsk: false,
                             id: data.id })
    } else {
        var dialog = pageStack.push(_locationComponentUrl, {})
        dialog.accepted.connect(function() {
            webView.sendAsyncMessage("embedui:premissions", {
                                               allow: true,
                                               checkedDontAsk: false,
                                               id: data.id })
            acceptedGeolocationUrl = WebUtils.displayableUrl(url)
            rejectedGeolocationUrl = ""
        })
        dialog.rejected.connect(function() {
            webView.sendAsyncMessage("embedui:premissions", {
                                               allow: false,
                                               checkedDontAsk: false,
                                               id: data.id })
            rejectedGeolocationUrl = WebUtils.displayableUrl(url)
            acceptedGeolocationUrl = ""
        })
    }
}

function openAlert(data) {
    var winid = data.winid
    var dialog = pageStack.push(_alertComponentUrl,
                                {"text": data.text})
    // TODO: also the Async message must be sent when window gets closed
    dialog.done.connect(function() {
        webView.sendAsyncMessage("alertresponse", {"winid": winid})
    })
}

function openConfirm(data) {
    var winid = data.winid
    var dialog = pageStack.push(_confirmComponentUrl,
                                {"text": data.text})
    // TODO: also the Async message must be sent when window gets closed
    dialog.accepted.connect(function() {
        webView.sendAsyncMessage("confirmresponse",
                         {"winid": winid, "accepted": true})
    })
    dialog.rejected.connect(function() {
        webView.sendAsyncMessage("confirmresponse",
                         {"winid": winid, "accepted": false})
    })
}

function openPrompt(data) {
    var winid = data.winid
    var dialog = pageStack.push(_queryComponentUrl,
                                {"text": data.text, "value": data.defaultValue})
    // TODO: also the Async message must be sent when window gets closed
    dialog.accepted.connect(function() {
        webView.sendAsyncMessage("promptresponse",
                         {
                             "winid": winid,
                             "accepted": true,
                             "promptvalue": dialog.value
                         })
    })
    dialog.rejected.connect(function() {
        webView.sendAsyncMessage("promptresponse",
                         {"winid": winid, "accepted": false})
    })
}

function openFilePicker(data) {
    if (data.mode == Browser.FileUploadMode.Open || data.mode == Browser.FileUploadMode.OpenMultiple ) {
        pickerCreator.createObject(pageStack, {
                                       "pageStack": pageStack,
                                       "winid": data.winid,
                                       "webView": webView,
                                       "filter": data.filter,
                                       "mode": data.mode});
    } else {
        console.log("Gecko file picker requested unsupported mode" + data.mode)
    }
}
