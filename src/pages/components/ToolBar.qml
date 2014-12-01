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
    property string findText
    property real secondaryToolsHeight
    property bool secondaryToolsActive
    property bool findInPageActive
    readonly property bool showFindButtons: webView.findInPageHasResult && findInPageActive
    property alias bookmarked: secondaryBar.bookmarked
    readonly property alias toolsHeight: toolsRow.height

    signal showTabs
    signal showOverlay
    signal showSecondaryTools
    signal showChrome
    signal closeActiveTab

    // Used from SecondaryBar
    signal enterNewTabUrl
    signal findInPage
    signal shareActivePage
    signal bookmarkActivePage
    signal removeActivePageFromBookmarks

    function resetFind() {
        webView.sendAsyncMessage("embedui:find", { text: "", backwards: false, again: false })
        if (webView.contentItem) {
            webView.contentItem.forceChrome(false)
        }

        findInPageActive = false
    }

    width: parent.width

    onFindInPageActiveChanged: {
        // Down allow hiding of toolbar when finding text from the page.
        if (findInPageActive && webView.contentItem) {
            webView.contentItem.forceChrome(true)
        }
    }

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
        // Height of toolbar should be such that viewport height is
        // even number both chrome and fullscreen modes. For instance
        // height of 110px for toolbar would result 1px rounding
        // error in chrome mode as viewport height would be 850px. This would
        // result in CSS pixels viewport height of 566.66..px -> rounded to 566px.
        // So, we make sure below that (device height - toolbar height) / pixel ratio is even number.
        // target values when Theme.pixelratio == 1 are:
        // portrait: 108px
        // landcape: 78px
        property int scaledPortraitHeight: Screen.height -
                                           Math.floor((Screen.height - Settings.toolbarLarge * Theme.pixelRatio) /
                                                      WebUtils.cssPixelRatio) * WebUtils.cssPixelRatio
        property int scaledLandscapeHeight: Screen.width -
                                            Math.floor((Screen.width - Settings.toolbarSmall * Theme.pixelRatio) /
                                                      WebUtils.cssPixelRatio) * WebUtils.cssPixelRatio

        width: parent.width
        height: isPortrait ? scaledPortraitHeight : scaledLandscapeHeight

        // Container item for cross fading tabs, close, find in page button (and keep Row's width still).
        Item {
            id: tabButton

            width: toolsRow.iconWidth + toolsRow.horizontalOffset
            height: parent.height

            Browser.TabButton {
                id: tabs

                opacity: secondaryToolsActive || findInPageActive ? 0.0 : 1.0
                horizontalOffset: toolsRow.horizontalOffset
                label.text: webView.tabModel.count
                onTapped: toolBarRow.showTabs()

                RotationAnimator {
                    id: rotationAnimator
                    target: tabs.icon
                    duration: 1500
                    easing.type: Easing.InOutQuad
                    alwaysRunToEnd: true
                }

                Connections {
                    target: webView.tabModel
                    onNewTabRequested: {
                        // New tab request triggers 90 degrees clockwise rotation
                        // for the tab icon.
                        rotationAnimator.from = rotationAnimator.to
                        rotationAnimator.to = rotationAnimator.from + 90
                        rotationAnimator.restart()
                    }

                    onTabClosed: {
                        // Counter closewise when closing.
                        rotationAnimator.from = rotationAnimator.to
                        rotationAnimator.to = rotationAnimator.from - 90
                        rotationAnimator.restart()
                    }
                }
            }

            Browser.IconButton {
                opacity: secondaryToolsActive && !findInPageActive ? 1.0 : 0.0
                icon.source: "image://theme/icon-m-tab-close"
                icon.anchors.horizontalCenterOffset: toolsRow.horizontalOffset
                active: webView.tabModel.count > 0
                onTapped: closeActiveTab()
            }

            Browser.IconButton {
                opacity: !secondaryToolsActive && findInPageActive ? 1.0 : 0.0
                icon.source: "image://theme/icon-m-search"
                icon.anchors.horizontalCenterOffset: toolsRow.horizontalOffset
                onTapped: {
                    findInPageActive = true
                    findInPage()
                }
            }
        }

        Browser.ExpandingButton {
            id: backIcon
            expandedWidth: toolsRow.iconWidth
            icon.source: "image://theme/icon-m-back"
            active: webView.canGoBack && !toolBarRow.secondaryToolsActive && !findInPageActive
            onTapped: webView.goBack()
        }

        MouseArea {
            id: touchArea

            readonly property bool down: pressed && containsMouse

            height: parent.height
            width: toolBarRow.width - (tabButton.width + reloadButton.width + backIcon.width + menuButton.width)
            enabled: !showFindButtons

            onClicked: {
                if (findInPageActive) {
                    findInPage()
                } else {
                    toolBarRow.showOverlay()
                }
            }

            Label {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width + Theme.paddingMedium
                color: touchArea.down ? Theme.highlightColor : Theme.primaryColor

                text: {
                    if (findInPageActive) {
                        //: No text search results were found from the page.
                        //% "No results"
                        return qsTrId("sailfish_browser-la-no_results")
                    } else if (url == "about:blank" || (webView.completed && webView.tabModel.count === 0 && !webView.tabModel.waitingForNewTab)) {
                        //: Placeholder text for url typing and searching
                        //% "Type URL or search"
                        return qsTrId("sailfish_browser-ph-type_url_or_search")
                    } else if (url) {
                        return parseDisplayableUrl(url)
                    } else {
                        //: Loading text that is visible when url is not yet resolved.
                        //% "Loading"
                        return qsTrId("sailfish_browser-la-loading")
                    }
                }

                truncationMode: TruncationMode.Fade

                opacity: showFindButtons ? 0.0 : 1.0
                Behavior on opacity { FadeAnimation {} }

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

            Browser.ExpandingButton {
                id: previousFindResult
                active: showFindButtons
                expandedWidth: (toolBarRow.width - menuButton.width - tabButton.width) / 2
                icon {
                    source: "image://theme/icon-m-left"
                    anchors.horizontalCenterOffset: Theme.paddingLarge
                }

                onTapped: {
                    webView.sendAsyncMessage("embedui:find", { text: findText, backwards: true, again: true })
                }
            }

            Browser.ExpandingButton {
                active: showFindButtons
                expandedWidth: previousFindResult.width
                anchors.left: previousFindResult.right
                icon {
                    source: "image://theme/icon-m-right"
                    anchors.horizontalCenterOffset: -Theme.paddingLarge
                }

                onTapped: {
                    webView.sendAsyncMessage("embedui:find", { text: findText, backwards: false, again: true })
                }
            }
        }

        Browser.ExpandingButton {
            id: reloadButton
            expandedWidth: toolsRow.iconWidth
            icon.source: webView.loading ? "image://theme/icon-m-reset" : "image://theme/icon-m-refresh"
            active: webView.contentItem && !toolBarRow.secondaryToolsActive && !findInPageActive
            onTapped: webView.loading ? webView.stop() : webView.reload()
        }

        Item {
            id: menuButton

            width: toolsRow.iconWidth + toolsRow.horizontalOffset
            height: parent.height

            Browser.IconButton {
                icon.source: "image://theme/icon-m-menu"
                icon.anchors.horizontalCenterOffset: -toolsRow.horizontalOffset
                width: parent.width
                opacity: secondaryToolsActive || findInPageActive ? 0.0 : 1.0
                onTapped: showSecondaryTools()
            }

            Browser.IconButton {
                icon.source: "image://theme/icon-m-menu"
                icon.anchors.horizontalCenterOffset: toolsRow.horizontalOffset
                width: parent.width
                rotation: 180
                opacity: secondaryToolsActive && !findInPageActive ? 1.0 : 0.0
                onTapped: showChrome()
            }

            Browser.IconButton {
                icon.source: "image://theme/icon-m-reset"
                icon.anchors.horizontalCenterOffset: -toolsRow.horizontalOffset
                width: parent.width
                opacity: !secondaryToolsActive && findInPageActive ? 1.0 : 0.0
                onTapped: {
                    resetFind()
                    showChrome()
                }
            }
        }
    }
}
