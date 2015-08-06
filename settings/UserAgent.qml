/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0
import org.nemomobile.configuration 1.0
import org.sailfishos.browser.settings 1.0

Page {

    readonly property string defaultUA: settings.defaultUA
    readonly property string firefoxUA: "Mozilla/5.0 (Android; Mobile; rv:31.0) Gecko/31.0 Firefox/31.0"
    readonly property string chromeUA: "Mozilla/5.0 (Linux; Android 4.0.4; Galaxy Nexus Build/IMM76B) AppleWebKit/535.19 (KHTML, like Gecko) Chrome/18.0.1025.133 Mobile Safari/535.19 "
    readonly property string safariUA: "Mozilla/5.0 (iPad; CPU OS 6_0 like Mac OS X) AppleWebKit/536.26 (KHTML, like Gecko) Version/6.0 Mobile/10A5355d Safari/8536.25"

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: contentColumn.height

        Column {
            id: contentColumn
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                //: User Agent settings page header
                //% "User Agent overrides"
                title: qsTrId("settings_browser-ph-useragent")
            }

            TextSwitch {
                id: enableOverride

                //% "Override User Agent"
                text: qsTrId("settings_browser-la-enable_user_agent_override")
                checked: userAgentOverrideConfig.value

                //: This will enable UserAgent string override by user
                //% "Enables User Agent overrides"
                description: qsTrId("settings_browser-la-enables_ua_override_desc")

                onCheckedChanged: {
                    if (!checked) {
                        overrideText.text = ""
                    }
                }
            }

            TextField {
                id: overrideText

                width: parent.width
                //% "User Agent string"
                label: qsTrId("settings_browser-la-user_agent_override")
                visible: enableOverride.checked
                placeholderText: defaultUA
                text: userAgentOverrideConfig.value

                onTextChanged: {
                    userAgentOverrideConfig.value = (text === defaultUA) ? "" : text
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                //: Button for resetting UA to Sailfish Browser's default
                //% "Reset to default"
                text: qsTrId("settings_browser-bt-reset_default")
                visible: enableOverride.checked
                onClicked: overrideText.text = defaultUA
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                //: Button for resetting UA to Firefox
                //% "Reset to Firefox"
                text: qsTrId("settings_browser-bt-reset_firefox")
                visible: enableOverride.checked
                onClicked: overrideText.text = firefoxUA
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                //: Button for resetting UA to Chrome
                //% "Reset to Chrome"
                text: qsTrId("settings_browser-bt-reset_chrome")
                visible: enableOverride.checked
                onClicked: overrideText.text = chromeUA
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                //: Button for resetting UA to Safari
                //% "Reset to Safari"
                text: qsTrId("settings_browser-bt-reset_safari")
                visible: enableOverride.checked
                onClicked: overrideText.text = safariUA
            }
        }
    }

    ConfigurationValue {
        id: userAgentOverrideConfig

        key: "/apps/sailfish-browser/settings/user_agent_override"
        defaultValue: ""
    }


    BrowserSettings {
        id: settings
    }
}
