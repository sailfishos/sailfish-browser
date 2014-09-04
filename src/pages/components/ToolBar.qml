/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import "." as Browser

Column {
    id: toolBarRow

    property string url
    property real secondaryToolsHeight
    property bool secondaryToolsActive
    property alias bookmarked: secondaryBar.bookmarked
    readonly property alias toolsHeight: toolsRow.height

    signal showTabs
    signal showOverlay
    signal showSecondaryTools
    signal hideSecondaryTools
    signal closeActiveTab

    // Used from SecondaryBar
    signal enterNewTabUrl
    signal searchFromActivePage
    signal shareActivePage
    signal showDownloads
    signal bookmarkActivePage
    signal removeActivePageFromBookmarks

    width: parent.width

    SecondaryBar {
        id: secondaryBar
        visible: opacity > 0.0 || height > 0.0
        opacity: secondaryToolsActive ? 1.0 : 0.0
        height: secondaryToolsHeight
        horizontalOffset: toolsRow.horizontalOffset
        iconWidth: toolsRow.iconWidth

        Behavior on opacity { FadeAnimation {} }
    }

    Row {
        id: toolsRow
        readonly property int horizontalOffset: Theme.paddingSmall
        readonly property int iconWidth: Theme.iconSizeMedium + 2 * Theme.paddingMedium

        width: parent.width
        height: isPortrait ? Settings.toolbarLarge : Settings.toolbarSmall

        // Container item for cross fading tabs and close button (and keep Row's width still).
        Item {
            id: tabButton

            width: toolsRow.iconWidth + toolsRow.horizontalOffset
            height: parent.height

            Browser.TabButton {
                opacity: secondaryToolsActive ? 0.0 : 1.0
                horizontalOffset: toolsRow.horizontalOffset
                label.visible: webView.tabModel.count > 0
                label.text: webView.tabModel.count

                onTapped: {
                    if (firstUseOverlay) {
                        firstUseOverlay.visible = false
                        firstUseOverlay.destroy()
                    }
                    if (!WebUtils.firstUseDone) WebUtils.firstUseDone = true
                    toolBarRow.showTabs()
                }
                Behavior on opacity { FadeAnimation {} }
            }

            Browser.IconButton {
                opacity: secondaryToolsActive ? 1.0 : 0.0
                icon.source: "image://theme/icon-m-tab-close"
                icon.anchors.horizontalCenterOffset: toolsRow.horizontalOffset
                active: webView.tabModel.count > 0
                onTapped: closeActiveTab()

                // Don't pass touch events through in the middle FadeAnimation
                enabled: opacity === 1.0

                Behavior on opacity { FadeAnimation {} }
            }
        }

        Browser.NavigationButton {
            id: backIcon
            buttonWidth: toolsRow.iconWidth
            icon.source: "image://theme/icon-m-back"
            active: webView.canGoBack
            onTapped: webView.goBack()
        }

        MouseArea {
            id: touchArea
            readonly property bool down: pressed && containsMouse

            height: parent.height
            width: toolBarRow.width - (tabButton.width + reloadButton.width + backIcon.width + menuButton.width)

            Label {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width + Theme.paddingMedium
                color: touchArea.down ? Theme.highlightColor : Theme.primaryColor

                text: {
                    if (url) {
                        return parseDisplayableUrl(url)
                    } else if (webView.contentItem) {
                        //: Loading text that is visible when url is not yet resolved.
                        //% "Loading"
                        return qsTrId("sailfish_browser-la-loading")
                    } else {
                        //: All tabs have been closed.
                        //% "No tabs"
                        return qsTrId("sailfish_browser-la-no_tabs")
                    }
                }

                truncationMode: TruncationMode.Fade

                function parseDisplayableUrl(url) {
                    var returnUrl = WebUtils.displayableUrl(url)
                    returnUrl = returnUrl.substring(returnUrl.lastIndexOf("/") + 1) // Strip protocol
                    if (returnUrl.indexOf("www.") === 0) {
                        returnUrl = returnUrl.substring(4)
                    } else if (returnUrl.indexOf("m.") === 0 && returnUrl.length > 2) {
                        returnUrl = returnUrl.substring(2)
                    } else if (returnUrl.indexOf("mobile.") === 0 && returnUrl.length > 7) {
                        returnUrl = returnUrl.substring(7)
                    }

                    return returnUrl || url
                }
            }
            onClicked: toolBarRow.showOverlay()
        }

        Browser.IconButton {
            id: reloadButton
            width: toolsRow.iconWidth
            icon.source: webView.loading ? "image://theme/icon-m-reset" : "image://theme/icon-m-refresh"
            active: webView.contentItem
            onTapped: webView.loading ? webView.stop() : webView.reload()
        }

        Browser.IconButton {
            id: menuButton
            icon.source: "image://theme/icon-m-menu"
            icon.anchors.horizontalCenterOffset: -toolsRow.horizontalOffset
            width: toolsRow.iconWidth + toolsRow.horizontalOffset
            onTapped: {
                if (secondaryToolsActive) {
                    hideSecondaryTools()
                } else {
                    showSecondaryTools()
                }
            }
        }
    }
}
