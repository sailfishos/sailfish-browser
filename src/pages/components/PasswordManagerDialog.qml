/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: passwordManagerDialog

    // As QML is not very closure friendly we'd better keep webView and requestId as properties of the dialog
    property QtObject webView
    property string requestId
    property string notificationType
    property variant formData

    onAccepted: {
        webView.sendAsyncMessage("embedui:login",
                                   {
                                       "buttonidx": 0, // "Yes" button
                                       "id": requestId
                                   })
    }

    onRejected: {
        webView.sendAsyncMessage("embedui:login",
                                   {
                                       "buttonidx": 1, // "No" button
                                       "id": requestId
                                   })
    }

    Item {
        id: contentItem
        anchors.fill: parent

        DialogHeader {
            //: Accept browser's request to save entered password
            //% "Save"
            acceptText: qsTrId("sailfish_browser-he-accept_password_mgr_request")
            _glassOnly: true
        }

        Label {
            id: label

            anchors.centerIn: parent
            width: parent.width - 2 * Theme.paddingLarge
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            font {
                family: Theme.fontFamilyHeading
                pixelSize: Theme.fontSizeExtraLarge
            }
            color: Theme.highlightColor
            opacity: 0.6

            text: {
                switch (notificationType) {
                case "password-save": {
                    if (formData["displayUser"]) {
                        //% "Would you like to save password for user %1 on %2?"
                        return qsTrId("sailfish_browser-la-save_password").arg(formData["displayUser"]).arg(formData["displayHost"])
                    } else {
                        //% "Would you like to save password on %1?"
                        return qsTrId("sailfish_browser-ls-save_password_no_user").arg(formData["displayHost"])
                    }
                    break
                }
                case "password-change": {
                    if (formData["displayUser"]) {
                        //% "Would you like to update password for user %1?"
                        return qsTrId("sailfish_browser-la-update_password").arg(formData["displayUser"])
                    } else {
                        //% "Would you like to update password?"
                        return qsTrId("sailfish_browser-la-update_password_no_user")
                    }
                    break
                }
                case "password-update-multiuser": {
                    // TODO: currently embedlite component for login manager promter heavily relies
                    //       on gecko's localization service for UI strings.
                    //       See LoginManagerPrompter.promtToChangePasswordWithUsernames() for details.
                    //       We need to reimplement it in order to use Qt l10n for password updates where
                    //       we don't know which existing login is being updated.
                    //       Though this task is quite a corner case and thus of very low priority.
                    console.log("TODO: this notification type hasn't been implemented yet")
                    break
                }
                }
            }
        }
    }
}
