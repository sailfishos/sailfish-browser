/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0
import "." as Browser

PanelBackground {
    id: overlay

    property Item webView
    property Item browserPage
    property Item overlayAnimator
    property alias historyModel: historyList.model
    property alias toolBar: toolBar
    property alias progressBar: progressBar

    function openTabPage(focus, newTab, operationType) {
        if (browserPage.status === PageStatus.Active) {
            webView.captureScreen()
            pageStack.push(browserPage.tabPageComponent ? browserPage.tabPageComponent :
                                                          Qt.resolvedUrl("TabPage.qml"),
                                                          {
                                                              "browserPage" : browserPage,
                                                              "initialSearchFocus": focus,
                                                              "newTab": newTab
                                                          }, operationType)
        }
    }

    y: webView.fullscreenHeight - toolBar.height
    width: parent.width
    height: toolBar.height + historyContainer.height

    gradient: Gradient {
        GradientStop { position: 0.0; color: Theme.rgba(Theme.highlightBackgroundColor, 0.3) }
        GradientStop { position: 1.0; color: Theme.rgba(Theme.highlightBackgroundColor, 0.0) }
    }

    Image {
        anchors.fill: parent
        source: "image://theme/graphic-gradient-edge"
    }

    Browser.ProgressBar {
        id: progressBar
        width: parent.width
        height: historyContainer.height - historyList.height + toolBar.height
        visible: !firstUseOverlay
        opacity: webView.loading && !overlayAnimator.dragHintVisible ? 1.0 : 0.0
        progress: webView.loadProgress / 100.0
    }

    Rectangle {
        anchors {
            top: parent.top
            topMargin: Theme.paddingMedium
            horizontalCenter: parent.horizontalCenter
        }
        color: Theme.primaryColor
        width: Theme.paddingLarge
        height: Theme.paddingSmall
        radius: 2

        opacity: overlayAnimator.dragHintVisible ? 1.0 : 0.0
        Behavior on opacity { FadeAnimation {} }
    }

    Text {
        id: hint
        font.pixelSize: Theme.fontSizeLarge
        text: "Pull up to enter address"
        anchors {
            top: parent.top
            topMargin: Theme.paddingLarge
            left: parent.left; leftMargin: Theme.paddingLarge
            right: parent.right; rightMargin: Theme.paddingLarge
        }
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        opacity: overlayAnimator.dragHintVisible ? 1.0 : 0.0
        color: Theme.highlightColor

        Behavior on opacity { FadeAnimation {} }
    }

    MouseArea {
        id: dragArea

        property int dragThreshold: state === "fullscreenOverlay" ? toolBar.height * 1.5 : (webView.fullscreenHeight - toolBar.height * 2)

        width: parent.width
        height: toolBar.height + historyContainer.height + Theme.paddingLarge * 2

        anchors {
            top: parent.top
            // Check history height
            topMargin: -Theme.paddingLarge * 2
        }

        opacity: !overlayAnimator.dragHintVisible ? 1.0 : 0.0
        enabled: !webView.fullscreenMode
        drag.target: overlay
        drag.filterChildren: true
        drag.axis: Drag.YAxis
        drag.minimumY: browserPage.isPortrait ? toolBar.height : -toolBar.height
        drag.maximumY: webView.fullscreenHeight - toolBar.height

        drag.onActiveChanged: {
            if (!drag.active) {
                if (overlay.y < dragThreshold) {
                    overlayAnimator.state = "fullscreenOverlay"
                } else {
                    overlayAnimator.state = "chromeVisible"
                }
            } else {
                // Store previous end state
                if (overlayAnimator.state !== "draggingOverlay") {
                    state = overlayAnimator.state
                }

                if (overlayAnimator.atTop && webView.inputPanelVisible) {
                    Qt.inputMethod.hide()
                }

                overlayAnimator.state = "draggingOverlay"
                console.log("STATE:", state, webView.inputPanelVisible)

            }
        }

        Behavior on opacity { FadeAnimation {} }

        // We don't want to fade in/out toolbar. Only rest of the content.
        Browser.ToolBar {
            id: toolBar
            anchors {
                top: parent.top
                topMargin: Theme.paddingLarge * 2
            }
        }

        Column {
            id: historyContainer
            width: parent.width
            opacity: overlayAnimator.atBottom ? 0.0 : 1.0
            visible: opacity > 0.0
            spacing: Theme.paddingSmall
            anchors {
                left: parent.left; right: parent.right
                top: toolBar.bottom
            }

            Behavior on opacity { FadeAnimation {} }

            Label {
                id: title
                x: Theme.paddingLarge
                text: browserPage.title
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeExtraSmall
                width: parent.width - 2 * Theme.paddingLarge
                truncationMode: TruncationMode.Fade
            }

            Loader {
                id: searchFieldLoader

                property string text: item ? item.text : ""

                asynchronous: true
                height: Theme.itemSizeMedium
                width: parent.width
                sourceComponent: Item {
                    property alias text: textField.text
                    property string url: overlay.webView.url
                    property bool atTop: overlayAnimator.atTop

                    onUrlChanged: text = url
                    onAtTopChanged: {
                        if (atTop) {
                            textField.forceActiveFocus()
                        } else {
                            textField.focus = false
                        }
                    }



                    height: Theme.itemSizeMedium
                    width: parent.width - Theme.paddingLarge

                    TextField {
                        id: textField
                        text: browserPage.url
                        width: parent.width - clearButton.width

                        onActiveFocusChanged: {
                            if (activeFocus) {
                                overlayAnimator.show()
                                if (text === webView.url) {
                                    selectAll()
                                }
                            }
                        }

                        label: {
                            if (text.length === 0) return ""

                            if (activeFocus || text !== browserPage.url) {
                                //% "Search"
                                return qsTrId("sailfish_browser-la-search")
                            }

                            //: Active browser tab.
                            //% "Active Tab"
                            var activeTab = qsTrId("sailfish_browser-la-active-tab")

                            if (text === browserPage.url && webView.loading) {
                                //: Current browser page loading.
                                //% "Loading"
                                return activeTab + " • " + qsTrId("sailfish_browser-la-loading")
                            } else {
                                //: Current browser page loaded.
                                //% "Done"
                                return activeTab + " • " + qsTrId("sailfish_browser-la-done")
                            }
                        }

                        EnterKey.iconSource: "image://theme/icon-m-enter-accept"
                        EnterKey.onClicked: {
                            // let gecko figure out how to handle malformed URLs
                            overlayAnimator.allowProgressOverlay()
                            webView.load(text)
                            webView.focus = true
                        }
                    }

                    Browser.IconButton {
                        id: clearButton
                        width: icon.width + Theme.paddingMedium * 2
                        height: textField.height
                        anchors.left: textField.right
                        icon.source: "image://theme/icon-m-clear"
                        icon.anchors.top: clearButton.top
                        icon.anchors.horizontalCenter: clearButton.horizontalCenter
                        icon.anchors.centerIn: undefined

                        opacity: textField.text.length > 0 ? 1 : 0
                        Behavior on opacity {
                            FadeAnimation {}
                        }

                        onTapped: {
                            textField.text = ""
                            textField.forceActiveFocus()
                        }
                    }
                }
            }

            Browser.HistoryList {
                id: historyList

                width: parent.width
                //height: browserPage.isPortrait ? browserPage.height - toolBar.height : browserPage.height
                height: browserPage.height - toolBar.height - title.height - searchFieldLoader.height - Theme.paddingSmall * 2 - dragArea.drag.minimumY
                header: SectionHeader {
                    //: Section header for history items
                    //% "History"
                    text: qsTrId("sailfish_browser-he-history")
                }
                search: searchFieldLoader.text

                onSearchChanged: if (search !== webView.url) historyModel.search(search)
                onLoad: {
                    if (searchFieldLoader.item) {
                        searchFieldLoader.item.text = url
                    }
                    overlayAnimator.allowProgressOverlay()
                    webView.load(url, title)
                    webView.focus = true
                }
            }
        }
    }
}
