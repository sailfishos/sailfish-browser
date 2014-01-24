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

var activeWebView
var popups
var pageStack
var auxTimer
var contextMenuComponent
var componentParent
var resourceController
var tabModel
// TODO: WebUtils context property. Should be singleton.
var WebUtils

// TODO: Handle these per activeWebView (map of webviews + accepted/rejectedGeolocationUrl)
var acceptedGeolocationUrl = ""
var rejectedGeolocationUrl = ""

// Singleton
var _contextMenu

// As QML can't disconnect closure from a signal (but methods only)
// let's keep auth data in this auxilary attribute whose sole purpose is to
// pass arguments to openAuthDialog().
var _authData = null

function _hideVirtualKeyboard() {
    if (Qt.inputMethod.visible) {
        componentParent.focus = true
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
            auxTimer.triggered.disconnect(activeWebView.openAuthDialog)
            activeWebView._authData = null
        }

        var dialog = pageStack.push(popups.authenticationComponentUrl,
                                    {
                                        "hostname": data.text,
                                        "realm": data.title,
                                        "username": data.defaultValue,
                                        "passwordOnly": data.passwordOnly
                                    })
        dialog.accepted.connect(function () {
            activeWebView.sendAsyncMessage("authresponse",
                                           {
                                               "winid": winid,
                                               "accepted": true,
                                               "username": dialog.username,
                                               "password": dialog.password
                                           })
        })
        dialog.rejected.connect(function() {
            activeWebView.sendAsyncMessage("authresponse",
                                           {"winid": winid, "accepted": false})
        })
    }
}

function openSelectDialog(data) {
    var dialog = pageStack.push(popups.selectComponentUrl,
                                {
                                    "options": data.options,
                                    "multiple": data.multiple,
                                    "webview": activeWebView
                                })
}

function openPasswordManagerDialog(data) {
    pageStack.push(popups.passwordManagerComponentUrl,
                   {
                       "webView": activeWebView,
                       "requestId": data.id,
                       "notificationType": data.name,
                       "formData": data.formdata
                   })
}

function openContextMenu(data) {
    activeWebView.contextMenuRequested(data)
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
            contextMenuComponent = Qt.createComponent(popups.contextMenuComponentUrl)
            if (contextMenuComponent.status !== QtQuick.Component.Error) {
                _contextMenu = contextMenuComponent.createObject(componentParent,
                                                        {
                                                            "linkHref": linkHref,
                                                            "imageSrc": imageSrc,
                                                            "linkTitle": linkTitle.trim(),
                                                            "contentType": contentType,
                                                            "tabModel": tabModel,
                                                            "viewId": activeWebView.uniqueID()
                                                        })
                _hideVirtualKeyboard()

                popups.active = Qt.binding(function() { return (_contextMenu.active) })
                _contextMenu.show()
            } else {
                console.log("Can't load BrowserContextMenu.qml")
            }
        }
    }
}

function openLocationDialog(data) {
    // Ask for location permission
    var url = activeWebView.url
    if (isAcceptedGeolocationUrl(url)) {
        activeWebView.sendAsyncMessage("embedui:premissions", {
                             allow: true,
                             checkedDontAsk: false,
                             id: data.id })
    } else if (isRejectedGeolocationUrl(url)) {
        activeWebView.sendAsyncMessage("embedui:premissions", {
                             allow: false,
                             checkedDontAsk: false,
                             id: data.id })
    } else {
        var dialog = pageStack.push(popups.locationComponentUrl, {})
        dialog.accepted.connect(function() {
            activeWebView.sendAsyncMessage("embedui:premissions", {
                                               allow: true,
                                               checkedDontAsk: false,
                                               id: data.id })
            acceptedGeolocationUrl = WebUtils.displayableUrl(url)
            rejectedGeolocationUrl = ""
        })
        dialog.rejected.connect(function() {
            activeWebView.sendAsyncMessage("embedui:premissions", {
                                               allow: false,
                                               checkedDontAsk: false,
                                               id: data.id })
            rejectedGeolocationUrl = WebUtils.displayableUrl(url)
            acceptedGeolocationUrl = ""
        })
    }
}
