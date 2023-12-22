/****************************************************************************
**
** Copyright (c) 2020 - 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.WebView.Controls 1.0
import Sailfish.WebEngine 1.0

Page {
    id: page

    property string title
    property string url
    property PermissionModel permissionModel

    function _getPopupCapability() {
        if (WebEngineSettings.popupEnabled) {
            return PermissionManager.Allow
        } else {
            return PermissionManager.Deny
        }
    }

    function _getCookieCapability() {
        switch (WebEngineSettings.cookieBehavior) {
        case WebEngineSettings.AcceptAll:
            return PermissionManager.Allow
        case WebEngineSettings.BlockAll:
            return PermissionManager.Deny
        default:
            return PermissionManager.Unknown
        }
    }

    property var permissions: {
        "geolocation": PermissionManager.Prompt,
        "popup": _getPopupCapability(),
        "cookie": _getCookieCapability(),
        "camera": PermissionManager.Prompt,
        "microphone": PermissionManager.Prompt
    }

    function setPermissionTypesModel(permissions) {
        permissionTypesModel.append({
                   //% "Location"
                   title: qsTrId("sailfish_browser-ti-location"),
                   type: "geolocation",
                   capability: permissions["geolocation"]
               })
        permissionTypesModel.append({
                   //% "Popup"
                   title: qsTrId("sailfish_browser-ti-popup"),
                   type: "popup",
                   capability: permissions["popup"]
               })
        permissionTypesModel.append({
                   //% "Cookies"
                   title: qsTrId("sailfish_browser-ti-cookies"),
                   type: "cookie",
                   capability: permissions["cookie"]
               })
        permissionTypesModel.append({
                   //% "Camera"
                   title: qsTrId("sailfish_browser-ti-camera"),
                   type: "camera",
                   capability: permissions["camera"]
               })
        permissionTypesModel.append({
                   //% "Microphone"
                   title: qsTrId("sailfish_browser-ti-microphone"),
                   type: "microphone",
                   capability: permissions["microphone"]
               })
    }

    PermissionFilterProxyModel {
        id: permissionFilterProxyModel
        sourceModel: permissionModel
        onlyPermanent: true

        Component.onCompleted: setPermissionTypesModel(permissions)
    }

    Repeater {
        model: permissionFilterProxyModel
        delegate: Item {
            Component.onCompleted: permissions[model.type] = model.capability
        }
    }

    ListModel {
        id: permissionTypesModel
    }

    SilicaListView {
        anchors.fill: parent
        header: PageHeader {
            title: page.title
            description: page.url
        }

        model: permissionTypesModel

        delegate: ComboBox {
            label: model.title

            currentIndex: {
                switch(model.capability) {
                case PermissionManager.Allow:
                    return 0 // index for "Allow" menu
                case PermissionManager.Deny:
                    return 1 // index for "Deny" menu
                default:
                    return 2 // index for "Always ask" menu
                }
            }

            menu: ContextMenu {
                MenuItem {
                    //: Shown for context menu allow permission
                    //% "Allow"
                    text: qsTrId("sailfish_browser-me-allow")
                    onClicked: PermissionManager.add(url, model.type, PermissionManager.Allow)
                }
                MenuItem {
                    //: Shown for context menu block permission
                    //% "Block"
                    text: qsTrId("sailfish_browser-me-block")
                    onClicked: PermissionManager.add(url, model.type, PermissionManager.Deny)
                }
                MenuItem {
                    //: Shown for context menu always ask permission
                    //% "Always ask"
                    text: qsTrId("sailfish_browser-me-always_ask")
                    onClicked: PermissionManager.add(url, model.type, PermissionManager.Prompt)
                    visible: model.type !== "popup" && model.type !== "cookie"
                }
            }
        }
    }
}
