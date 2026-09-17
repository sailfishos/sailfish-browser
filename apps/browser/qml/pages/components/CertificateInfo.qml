/*
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Silica.private 1.0 as Private
import Sailfish.Browser 1.0
import Qt5Mozilla 1.0
import "." as Browser
import Sailfish.WebView.Controls 1.0

SilicaFlickable {
    id: root

    property QMozSecurity security
    readonly property bool _secure: security && security.allGood

    contentHeight: certInfoColumn.height
    clip: contentHeight > height

    signal showCertDetail
    signal closeCertInfo

    function openSiteSettings() {
        pageStack.push("../SitePermissionPage.qml",
                       {
                           title: webView.title,
                           url: toolBarRow.url,
                           permissionModel: permissionIndicationModel
                       })
    }

    onSecurityChanged: {
        // Jump back to the top
        contentY = originY
    }

    PermissionModel {
        id: permissionIndicationModel

        host: toolBarRow.url
    }

    VerticalScrollDecorator {}

    MouseArea {
        anchors.fill: parent
        onClicked: closeCertInfo()
    }

    Column {
        id: certInfoColumn

        width: parent.width
        spacing: Theme.paddingMedium
        topPadding: Theme.paddingLarge

        Image {
            source: _secure ? "image://theme/icon-m-device-lock" : "image://theme/icon-m-warning"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Label {
            width: parent.width - 2 * Theme.horizontalPageMargin
            x: Theme.horizontalPageMargin
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeLarge
            color: _secure ? Theme.primaryColor : Theme.errorColor
            text: {
                if (_secure) {
                    //: The SSL/TLS connection is good
                    //% "Connection is secure"
                    return qsTrId("sailfish_browser-la-cert_connection_secure")
                }
                //: Either no SSL/TLS is in use, or the SSL/TLS connection is broken in some way
                //% "Connection is not secure"
                return qsTrId("sailfish_browser-la-cert_connection_insecure")
            }
        }

        DetailItem {
            //: Label for the issuer field of a TLS certificate
            //% "Verified by"
            label: qsTrId("sailfish_browser-la-cert_issuer")
            value: security ? security.issuerDisplayName : ""
            visible: value && value.length > 0
        }

        DetailItem {
            //: Label for the cipher suite field of a TLS certificate
            //% "Cipher suite"
            label: qsTrId("sailfish_browser-la-cipher_suite")
            value: security ? security.cipherName : ""
            visible: value && value.length > 0
        }

        Label {
            width: parent.width - 2 * Theme.horizontalPageMargin
            x: Theme.horizontalPageMargin
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            color: Theme.secondaryHighlightColor
            visible: !_secure
            //% "Do not enter personal data, passwords, card details on this site"
            text: qsTrId("sailfish_browser-sh-do_not-enter_personal_data")
        }

        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            //: Details for certificate info
            //% "Details"
            text: qsTrId("sailfish_browser-sh-details")
            enabled: security && !security.certIsNull
            visible: enabled
            onClicked: showCertDetail()
        }


        Loader {
            height: Theme.fontSizeMedium + Theme.iconSizeMedium + Theme.paddingMedium
            width: permissionIndicationModel.count === 0 ? implicitWidth : parent.width
            anchors.horizontalCenter: parent.horizontalCenter
            sourceComponent: permissionIndicationModel.count === 0 ? permissionButtonComponent : permissionComponent
        }

        Component {
            id: permissionButtonComponent

            Button {
                //: Manage permission for current site
                //% "Site permissions"
                text: qsTrId("sailfish_browser-sh-site-permissions")
                onClicked: openSiteSettings()
            }
        }

        Component {
            id: permissionComponent

            MouseArea {
                id: permissionArea

                width: parent.width
                height: permissionColumn.height
                onClicked: openSiteSettings()

                Column {
                    id: permissionColumn

                    width: parent.width
                    spacing: Theme.paddingMedium
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: permissionIndicationRepeater.count > 0

                    Label {
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        x: Theme.horizontalPageMargin
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.Wrap
                        color: Theme.highlightColor
                        //% "Current permissions"
                        text: qsTrId("sailfish_browser-sh-current-permissions")
                    }

                    Grid {
                        rows: Math.max(1, Math.ceil(permissionIndicationModel.count / 5))
                        columns: Math.min(permissionIndicationModel.count, 5)
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: Theme.paddingLarge

                        Repeater {
                            id: permissionIndicationRepeater

                            model: permissionIndicationModel
                            delegate: Icon {
                                source: {
                                    // template variants have small hole to avoid overlapping transparent
                                    // icon content.
                                    // TODO: designed for deny graphics, the prompt case should either
                                    // have prompt icon adjusted to match the deny shape or there should be
                                    // yet another set of clipped permission graphics.
                                    var template = (model.capability === PermissionManager.Deny
                                                    || model.capability === PermissionManager.Prompt)
                                    if (model.type === "geolocation") {
                                        return "image://theme/icon-m-browser-location" + (template ? "-template" : "")
                                    } else if (model.type === "cookie") {
                                        return "image://theme/icon-m-browser-cookies" + (template ? "-template" : "")
                                    } else if (model.type === "popup") {
                                        return "image://theme/icon-m-browser-popup" + (template ? "-template" : "")
                                    } else if (model.type === "camera") {
                                        return "image://theme/icon-m-browser-camera" + (template ? "-template" : "")
                                    } else if (model.type === "microphone") {
                                        return "image://theme/icon-m-browser-microphone" + (template ? "-template" : "")
                                    } else {
                                        return ""
                                    }
                                }
                                highlighted: permissionArea.pressed

                                Icon {
                                    anchors {
                                        bottom: parent.bottom
                                        right: parent.right
                                    }
                                    visible: model.capability === PermissionManager.Deny
                                             || model.capability === PermissionManager.Prompt
                                    source: model.capability === PermissionManager.Deny
                                            ? "image://theme/icon-s-blocked"
                                            : "image://theme/icon-s-maybe"
                                    color: model.capability === PermissionManager.Deny
                                           ? Theme.errorColor : Theme.primaryColor
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
