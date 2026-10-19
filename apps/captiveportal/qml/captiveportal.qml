/****************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC.
** Copyright (c) 2021 Jolla Ltd.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import "pages"
import "shared"

BrowserWindow {
    id: window

    cover: Component {
        CoverBackground {
            id: portalCover

            Column {
                anchors.centerIn: parent
                width: parent.width - 2 * Theme.paddingLarge
                spacing: Theme.paddingLarge

                Image {
                    id: coverIcon

                    anchors.horizontalCenter: parent.horizontalCenter
                    source: "image://theme/icon-launcher-browser"
                    width: parent.width / 2
                    height: width
                    fillMode: Image.PreserveAspectFit
                }

                Label {
                    width: parent.width
                    text: window.rootPage && window.rootPage.signInHeading.length
                          ? window.rootPage.signInHeading
                          : WebUtils.sparse
                            ? (window.webView ? WebUtils.displayableUrl(String(window.webView.url)) : "")
                            //% "Sign in"
                            : qsTrId("sailfish_captiveportal-la-sign_in")
                    height: Math.min(implicitHeight, Math.max(0, portalCover.height
                                     - coverIcon.height - 3 * Theme.paddingLarge))
                    wrapMode: Text.Wrap
                    textFormat: Text.PlainText
                    horizontalAlignment: Text.AlignHCenter
                    color: palette.secondaryColor
                    font.pixelSize: Theme.fontSizeMedium
                    truncationMode: TruncationMode.Elide
                }
            }
        }
    }

    //% "Captive portal"
    activityDisabledByMdm: qsTrId("sailfish_captiveportal-la-captive_portal")
    function closeNetwork() {
        if (rootPage && rootPage.flowController) rootPage.flowController.closeNetwork()
        else close()
    }
    initialPage: Component {
        CaptivePortalPage {
            id: browserPage

            Component.onCompleted: {
                window.webView = webView
                window.rootPage = browserPage
            }

            Component.onDestruction: {
                window.webView = null
            }
        }
    }
}
