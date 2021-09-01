/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Policy 1.0
import org.nemomobile.configuration 1.0

Dialog {
    id: dialog

    property ConfigurationValue homePageConfig
    canAccept: textField.text !== ""
    onAccepted: {
        homePageConfig.value = textField.text.trim() || "about:blank"
    }

    DialogHeader {
        id: header
        //: Accept button text for adding a home page adress
        //% "OK"
        acceptText: qsTrId("sailfish_browser-he-ok")
    }

    TextField {
        id: textField
        anchors.top: header.bottom
        text: homePageConfig.value === "about:blank" ? "" : homePageConfig.value
        //% "Shown when the browser is opened with no tabs to load."
        description: qsTrId("sailfish_browser-he-home_page_web_page-address")
        //% "Web page address"
        placeholderText: qsTrId("settings_browser-la-web_page_address")
        label: placeholderText
        inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly
        EnterKey.onClicked: dialog.accept()
    }
}
