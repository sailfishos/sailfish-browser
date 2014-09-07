/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2

Item {
    id: animator

    property Item overlay
    property Item webView
    property bool portrait
    property bool atTop
    property bool atMiddle
    property bool atBottom: true
    property int transitionDuration: !_immediate ? 400 : 0

    property bool active
    readonly property bool allowContentUse: state === "chromeVisible" || state === "fullscreenWebPage" || state === "doubleToolBar"
    readonly property bool dragging: state === "draggingOverlay"
    readonly property bool secondaryTools: state === "doubleToolBar"

    property bool _immediate

    function showSecondaryTools() {
        state = "doubleToolBar"
    }

    function showChrome() {
        state = "chromeVisible"
    }

    function showOverlay(immediate) {
        _immediate = immediate || false
        state = "fullscreenOverlay"
    }

    function drag() {
        state = "draggingOverlay"
    }

    function hide() {
        state = "noOverlay"
    }

    state: "chromeVisible"

    // TODO: Fix real cover. Once that is fixed, we should remove this block.
    onActiveChanged: {
        if (active && state !== "fullscreenOverlay") {
            state = "chromeVisible"
        } else if (!active) {
            state = "fullscreenWebPage"
        }
    }

    onStateChanged: {
        console.log("State:", state)
        console.trace()
        // Animation end changes to true state. Hence not like atTop = state !== "fullscreenOverlay"
        if (state !== "fullscreenOverlay") {
            atTop = false
        } if (state !== "chromeVisible" && state !== "fullscreenWebPage" && state !== "doubleToolBar") {
            atBottom = false
        } if (state !== "loadProgressOverlay") {
            atMiddle = false
        }
    }

    Connections {
        target: webView
        ignoreUnknownSignals: true

        onFullscreenModeChanged: {
            if (!webView.fullscreenMode) {
                animator.state = "chromeVisible"
            } else if (webView.fullscreenMode) {
                animator.state = "fullscreenWebPage"
            }
        }
    }

    states: [
        State {
            name: "fullscreenWebPage"
            changes: [
                PropertyChanges {
                    target: webView
                    // TODO: once we get rid of bad rendering loop, check if we could use here browserPage.height
                    // instead of webView.fullscreenHeight. Currently with browserPage.height binding we skip
                    // frames when returning back from tab page so that virtual keyboard was open.
                    height: overlay.y
                },
                PropertyChanges {
                    target: overlay
                    y: webView.fullscreenHeight
                }
            ]
        },
        State {
            name: "noOverlay"
            extend: "fullscreenWebPage"
        },
        State {
            name: "chromeVisible"
            changes: [
                PropertyChanges {
                    target: webView
                    height: overlay.y
                },
                PropertyChanges {
                    target: overlay
                    y: webView.fullscreenHeight - overlay.toolBar.toolsHeight
                }
            ]
        },
        State {
            name: "draggingOverlay"
            changes: [
                PropertyChanges {
                    target: webView
                    height: overlay.y
                },
                PropertyChanges {
                    target: overlay
                    y: overlay.y
                },
                PropertyChanges {
                    target: overlay.toolBar
                    secondaryToolsHeight: 0
                }
            ]
        },

        State {
            name: "fullscreenOverlay"
            changes: [
                PropertyChanges {
                    target: webView
                    // was floor
                    //height: Math.ceil(overlay.y) ? Math.ceil(overlay.y) : 0
                    height: overlay.y
                },
                PropertyChanges {
                    target: overlay
                    y: portrait ? overlay.toolBar.toolsHeight : -overlay.toolBar.toolsHeight
                }
            ]
        },

        State {
            name: "doubleToolBar"
            changes: [
                PropertyChanges {
                    target: webView
                    height: overlay.y
                },
                PropertyChanges {
                    target: overlay
                    y: webView.fullscreenHeight - overlay.toolBar.toolsHeight * 2
                },
                PropertyChanges {
                    target: overlay.toolBar
                    secondaryToolsHeight: overlay.toolBar.toolsHeight
                }
            ]
        }
    ]

    transitions: [
        Transition {
            id: overlayTransition
            to: "fullscreenWebPage,chromeVisible,loadProgressOverlay,fullscreenOverlay,noOverlay,doubleToolBar"

            SequentialAnimation {
                NumberAnimation { target: webView; property: "height"; duration: transitionDuration; easing.type: Easing.InOutQuad }
                ScriptAction {
                    script: {
                        if (animator.state === "chromeVisible" || animator.state === "fullscreenWebPage" || animator.state === "doubleToolBar") {
                            atBottom = true
                        } else if (animator.state === "fullscreenOverlay") {
                            atTop = true
                        }

                        if (webView.contentItem) {
                            webView.contentItem.chrome = animator.state !== "fullscreenWebPage"
                        }
                        _immediate = false
                    }
                }
            }
            NumberAnimation { target: overlay; property: "y"; duration: transitionDuration; easing.type: Easing.InOutQuad }
            NumberAnimation { target: overlay.toolBar; property: "secondaryToolsHeight"; duration: transitionDuration; easing.type: Easing.InOutQuad }
        }
        ,
        Transition {
            to: "draggingOverlay"
            NumberAnimation { target: overlay.toolBar; property: "secondaryToolsHeight"; duration: transitionDuration; easing.type: Easing.InOutQuad }
        }
    ]
}
