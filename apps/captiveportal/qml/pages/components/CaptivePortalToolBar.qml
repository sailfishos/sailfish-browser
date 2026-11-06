/*
 * Copyright (c) 2014 - 2021 Jolla Ltd.
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import Sailfish.WebEngine 1.0
import "." as Browser
import "../../shared" as Shared

Column {
    id: toolBarRow

    property string url
    property bool isPdf
    property bool finding
    property int pdfRequest
    readonly property var contentItem: webView.contentItem
    readonly property real rowHeight: height
    readonly property int maxRowCount: 1

    readonly property int horizontalOffset: largeScreen ? Theme.paddingLarge : Theme.paddingSmall
    readonly property int buttonPadding: largeScreen
                                         || orientation === Orientation.Landscape
                                         || orientation === Orientation.LandscapeInverted
                                         ? Theme.paddingMedium : Theme.paddingSmall
    readonly property int iconWidth: largeScreen ? (Theme.iconSizeLarge + 3 * buttonPadding)
                                                 : (Theme.iconSizeMedium + 2 * buttonPadding)
    readonly property int smallIconWidth: largeScreen ? (Theme.iconSizeMedium + 3 * buttonPadding)
                                                      : (Theme.iconSizeSmall + 2 * buttonPadding)
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

    signal showChrome

    function updatePdfState() {
        ++pdfRequest
        isPdf = false
        finding = false
        webView.findInPageHasResult = false
        pdfStateTimer.restart()
    }

    function find(backwards, again) {
        if (isPdf && contentItem) {
            contentItem.sendAsyncMessage("embedui:find", {
                "text": finding ? searchField.text : "",
                "backwards": backwards, "again": again
            })
        }
    }

    onContentItemChanged: updatePdfState()
    onFindingChanged: {
        if (finding) {
            searchField.forceActiveFocus()
            find(false, false)
        } else {
            searchField.focus = false
            find(false, false)
        }
        showChrome()
    }

    Timer {
        id: pdfStateTimer

        interval: 0
        onTriggered: {
            if (WebUtils.sparse && contentItem) {
                contentItem.addMessageListener("embed:pdfState")
                contentItem.addMessageListener("embed:find")
                contentItem.sendAsyncMessage("embedui:pdfState", {"request": pdfRequest})
            }
        }
    }

    Connections {
        target: contentItem
        onSelectedTabChanged: updatePdfState()
        onUrlChanged: updatePdfState()
        onLoadedChanged: updatePdfState()
        onRecvAsyncMessage: {
            if (message === "embed:pdfState" && data.request === pdfRequest) {
                isPdf = data.isPdf
            }
        }
    }

    width: parent.width

    Item {
        width: 1
        height: Theme.paddingMedium
    }

    Row {
        visible: finding
        width: parent.width
        height: visible ? searchField.height : 0

        TextField {
            id: searchField

            width: parent.width - 3 * toolBarRow.iconWidth
            //% "Find from page"
            placeholderText: qsTrId("sailfish_browser-ph-type_find_from_page")
            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
            onTextChanged: find(false, false)
            EnterKey.iconSource: "image://theme/icon-m-enter-next"
            EnterKey.onClicked: {
                find(false, true)
                focus = false
            }
        }
        IconButton {
            width: toolBarRow.iconWidth
            enabled: searchField.text.length > 0 && webView.findInPageHasResult
            icon.source: "image://theme/icon-m-left"
            onClicked: find(true, true)
        }
        IconButton {
            width: toolBarRow.iconWidth
            enabled: searchField.text.length > 0 && webView.findInPageHasResult
            icon.source: "image://theme/icon-m-right"
            onClicked: find(false, true)
        }
        IconButton {
            width: toolBarRow.iconWidth
            icon.source: "image://theme/icon-m-close"
            onClicked: finding = false
        }
    }

    Row {
        id: toolsRow

        width: parent.width
        height: browserPage.isPortrait ? scaledPortraitHeight : scaledLandscapeHeight

        Shared.ExpandingButton {
            id: backIcon

            height: parent.height
            expandedWidth: toolBarRow.iconWidth
            icon.source: "image://theme/icon-m-back"
            active: webView.canGoBack
            onTapped: webView.goBack()
        }

        Label {
            anchors.verticalCenter: parent.verticalCenter
            width: toolBarRow.width - (reloadButton.width + backIcon.width
                                      + closeButton.width + clearDataButton.width
                                      + findButton.width + saveButton.width) + Theme.paddingMedium
            color: Theme.highlightColor

            text: {
                if (url) {
                    return WebUtils.displayableUrl(url)
                } else {
                    // Keep the translation id insync with ToolBar.qml's
                    //% "Loading"
                    //: Loading text that is visible when url is not yet resolved.
                    return qsTrId("sailfish_captiveportal-la-loading")
                }
            }

            truncationMode: TruncationMode.Fade
        }

        Shared.ExpandingButton {
            id: closeButton

            visible: WebUtils.sparse || (browserPage.flowController && browserPage.flowController.activeFlow)
            width: visible ? toolBarRow.iconWidth : 0
            height: parent.height
            expandedWidth: toolBarRow.iconWidth
            icon.source: "image://theme/icon-m-close"
            active: true
            onTapped: {
                if (browserPage.flowController && browserPage.flowController.activeFlow) {
                    browserPage.flowController.close(browserPage.flowController.activeFlowId, "cancelled")
                } else {
                    webView.contentItem.flowController.closeFlow({"tabId": webView.contentItem.selectedTabId})
                }
            }
        }

        IconButton {
            id: clearDataButton

            anchors.verticalCenter: parent.verticalCenter
            visible: WebUtils.sparse && !isPdf
            width: visible ? toolBarRow.iconWidth : 0
            icon.source: "image://theme/icon-m-delete"
            onClicked: pageStack.push(Qt.resolvedUrl("ClearSignInDataDialog.qml"))
        }

        IconButton {
            id: findButton

            anchors.verticalCenter: parent.verticalCenter
            visible: isPdf
            width: visible ? toolBarRow.iconWidth : 0
            icon.source: "image://theme/icon-m-search"
            onClicked: finding = !finding
        }

        IconButton {
            id: saveButton

            anchors.verticalCenter: parent.verticalCenter
            visible: isPdf
            width: visible ? toolBarRow.iconWidth : 0
            icon.source: "image://theme/icon-m-file-download-as-pdf"
            onClicked: WebEngine.notifyObservers("embedui:download", {
                "msg": "saveAsPdf", "windowId": contentItem.uniqueId,
                "tabId": contentItem.selectedTabId
            })
        }

        Shared.ExpandingButton {
            id: reloadButton

            height: parent.height
            expandedWidth: toolBarRow.iconWidth
            icon.source: webView.loading ? "image://theme/icon-m-reset" : "image://theme/icon-m-refresh"
            active: webView.contentItem
            onTapped: {
                if (webView.loading) {
                    webView.stop()
                } else {
                    webView.reload()
                }
                toolBarRow.showChrome()
            }
        }
    }
}
