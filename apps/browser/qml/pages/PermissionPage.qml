/****************************************************************************
**
** Copyright (c) 2020 - 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.WebView.Controls 1.0
import Sailfish.WebEngine 1.0
import org.nemomobile.configuration 1.0
import "components"

Page {
    property PermissionModel permissionExceptionsModel: PermissionModel {}
    property ListModel permissionTypesModel: ListModel {}

    function setGlobalPermission(permission, type) {
        if (type === "popup") {
            if (permission === PermissionManager.Allow) {
                WebEngineSettings.popupEnabled = true
            }
            if (permission === PermissionManager.Deny) {
                WebEngineSettings.popupEnabled = false
            }
        } else if (type === "cookie") {
            if (permission === PermissionManager.Allow) {
                WebEngineSettings.cookieBehavior = WebEngineSettings.AcceptAll
            } else if (permission === PermissionManager.Deny) {
                WebEngineSettings.cookieBehavior = WebEngineSettings.BlockAll
            }
        } else if (type === "geolocation") {
            if (permission === PermissionManager.Deny) {
                locationRequestsBlocked.value = true
            } else if (permission === PermissionManager.Prompt) {
                locationRequestsBlocked.value = false
            }
        } else if (type === "camera") {
            if (permission === PermissionManager.Deny) {
                cameraRequestsBlocked.value = true
            } else if (permission === PermissionManager.Prompt) {
                cameraRequestsBlocked.value = false
            }
        } else if (type === "microphone") {
            if (permission === PermissionManager.Deny) {
                microphoneRequestsBlocked.value = true
            } else if (permission === PermissionManager.Prompt) {
                microphoneRequestsBlocked.value = false
            }
        }
    }

    function _getSensitiveDataCapability(capabilityBlocked) {
        if (capabilityBlocked.value) {
            return PermissionManager.Deny
        } else {
            return PermissionManager.Prompt
        }
    }

    function _getPopupCapability() {
        if (WebEngineSettings.popupEnabled) {
            return PermissionManager.Allow
        } else {
            return PermissionManager.Deny
        }
    }

    function _getCookieCapability() {
        switch(WebEngineSettings.cookieBehavior) {
        case WebEngineSettings.AcceptAll:
            return PermissionManager.Allow
        case WebEngineSettings.BlockAll:
            return PermissionManager.Deny
        default:
            return PermissionManager.Unknown
        }
    }

    function initPermissionTypesModel() {
        permissionTypesModel.append({
                                        //% "Location"
                                        title: qsTrId("sailfish_browser-ti-geolocation"),
                                        type: "geolocation",
                                        capability: _getSensitiveDataCapability(locationRequestsBlocked),
                                        iconSource: "image://theme/icon-m-browser-location",
                                        sensitiveData: true
                                    })

        permissionTypesModel.append({
                                        //% "Popup"
                                        title: qsTrId("sailfish_browser-ti-popup"),
                                        type: "popup",
                                        capability: _getPopupCapability(),
                                        iconSource: "image://theme/icon-m-browser-popup",
                                        sensitiveData: false
                                    })

        permissionTypesModel.append({
                                        //% "Cookies"
                                        title: qsTrId("sailfish_browser-ti-cookies"),
                                        type: "cookie",
                                        capability: _getCookieCapability(),
                                        iconSource: "image://theme/icon-m-browser-cookies",
                                        sensitiveData: false
                                    })

        permissionTypesModel.append({
                                        //% "Camera"
                                        title: qsTrId("sailfish_browser-ti-camera"),
                                        type: "camera",
                                        capability: _getSensitiveDataCapability(cameraRequestsBlocked),
                                        iconSource: "image://theme/icon-m-browser-camera",
                                        sensitiveData: true
                                    })

        permissionTypesModel.append({
                                        //% "Microphone"
                                        title: qsTrId("sailfish_browser-ti-microphone"),
                                        type: "microphone",
                                        capability: _getSensitiveDataCapability(microphoneRequestsBlocked),
                                        iconSource: "image://theme/icon-m-browser-microphone",
                                        sensitiveData: true
                                    })

    }

    Component.onCompleted: initPermissionTypesModel()

    SilicaListView {
        anchors.fill: parent
        header: PageHeader {
            //% "Permissions"
            title: qsTrId("sailfish_browser-ti-permissions")
        }
        model: permissionTypesModel

        delegate: BrowserListItem {
            label: model.title
            value: {
                switch(model.capability) {
                case PermissionManager.Allow:
                    //% "Allow"
                    return qsTrId("sailfish_browser-me-allow")
                case PermissionManager.Deny:
                    //% "Block"
                    return qsTrId("sailfish_browser-me-block")
                case PermissionManager.Prompt:
                    //% "Ask"
                    return qsTrId("sailfish_browser-me-ask")
                }
            }
            iconSource: model.iconSource

            menu: ContextMenu {
                MenuItem {
                    //% "Allow"
                    text: qsTrId("sailfish_browser-me-allow")
                    visible: !model.sensitiveData
                    onClicked: {
                        model.capability = PermissionManager.Allow
                        setGlobalPermission(PermissionManager.Allow, model.type)
                    }
                }
                MenuItem {
                    //% "Block"
                    text: qsTrId("sailfish_browser-me-block")
                    onClicked: {
                        model.capability = PermissionManager.Deny
                        setGlobalPermission(PermissionManager.Deny, model.type)
                    }
                }
                MenuItem {
                    //% "Ask"
                    text: qsTrId("sailfish_browser-me-ask")
                    visible: model.sensitiveData
                    onClicked: {
                        model.capability = PermissionManager.Prompt
                        setGlobalPermission(PermissionManager.Prompt, model.type)
                    }
                }
                MenuItem {
                    //% "Show exceptions"
                    text: qsTrId("sailfish_browser-me-show-exceptions")
                    onClicked: {
                        pageStack.push("PermissionExceptionsPage.qml",
                                       {
                                           model: permissionExceptionsModel,
                                           permissionType: model.type,
                                           title: model.title,
                                           iconSource: model.iconSource
                                       })
                    }
                }
            }
        }
    }

    ConfigurationValue {
        id: locationRequestsBlocked

        key: "/apps/sailfish-browser/settings/location_req_blocked"
    }

    ConfigurationValue {
        id: cameraRequestsBlocked

        key: "/apps/sailfish-browser/settings/camera_req_blocked"
    }

    ConfigurationValue {
        id: microphoneRequestsBlocked

        key: "/apps/sailfish-browser/settings/microphone_req_blocked"
    }
}
