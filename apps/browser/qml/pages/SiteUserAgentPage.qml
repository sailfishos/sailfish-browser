/****************************************************************************
**
** Copyright (c) 2022 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import "useragenthelper.js" as UserAgentHelper

Page {
    id: page

    property UserAgentModel userAgentModel: UserAgentModel {
        currentHost: WebUtils.host(webView.url)
    }
    property string hostname: userAgentModel.currentHost
    property bool isKey: userAgentModel.currentHostUserAgent.isKey
    property string userAgent: userAgentModel.currentHostUserAgent.userAgent
    property WebPage webPage: null

    SilicaListView {
        anchors.fill: parent
        model: UserAgentHelper.model

        header: PageHeader {
            //% "User agent override"
            title: qsTrId("sailfish_browser-he-user_agent_override")
            description: page.hostname
        }

        delegate: BackgroundItem {
            id: delegateItem

            onClicked: {
                if ("custom" in modelData) {
                    pageStack.animatorPush(customUserAgentDialog,
                                           {
                                               userAgentModel: userAgentModel,
                                               userAgent: page.isKey ? "" : page.userAgent,
                                               hostname: page.hostname
                                           })
                } else {
                    userAgentModel.setUserAgentOverride(page.hostname, modelData.key, true)
                    pageStack.pop()
                }
            }

            Label {
                width: parent.width
                leftPadding: Theme.horizontalPageMargin
                rightPadding: Theme.horizontalPageMargin
                text: modelData.name + (!page.isKey && "custom" in modelData ? ": " + userAgent : "")
                elide: Text.ElideRight
                highlighted: page.isKey ? modelData.key === page.userAgent : "custom" in modelData
            }
        }

        VerticalScrollDecorator {}
    }

    Component {
        id: customUserAgentDialog

        Dialog {
            id: dialog

            property UserAgentModel userAgentModel
            property string userAgent
            property string hostname

            canAccept: textField.text !== ""
            onAccepted: {
                userAgentModel.setUserAgentOverride(hostname, textField.text.trim(), false)
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
                focus: true
                text: userAgent
                //% "User agent to use when loading %1."
                description: qsTrId("sailfish_browser-he-user_agent_for_site").arg(hostname)
                label: placeholderText
                inputMethodHints: Qt.ImhNoPredictiveText
                EnterKey.onClicked: dialog.accept()
            }
        }
    }
}

