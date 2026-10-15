/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */
import QtQuick 2.6
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width
            DialogHeader {
                //% "Clear sign-in data?"
                title: qsTrId("sailfish_captiveportal-he-clear_sign_in_data")
                //% "Clear"
                acceptText: qsTrId("sailfish_captiveportal-he-clear")
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                wrapMode: Text.Wrap
                //% "Cookies, site data and saved passwords for sign-in windows will be removed."
                text: qsTrId("sailfish_captiveportal-la-clear_sign_in_data")
            }
        }
    }
    onAccepted: {
        if (WebUtils.sparse) {
            Settings.clearCookiesAndSiteData()
            Settings.clearPasswords()
        }
    }
}
