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
        updateState("doubleToolBar")
    }

    function showChrome(immediate) {
        updateState("chromeVisible", immediate || false)
    }

    function showOverlay(immediate) {
        updateState("fullscreenOverlay", immediate || false)
    }

    function drag() {
        updateState("draggingOverlay")
    }

    function hide() {
        updateState("noOverlay")
    }

    // Wrapper from updating the state. Handy for debugging.
    function updateState(newState, immediate) {
        _immediate = immediate || false
        // Verify that we return back to opacity 1.0
        // For instance, push to switcher from new-tab-creation overlay
        if (newState === "fullscreenWebPage" || newState === "chromeVisible") {
            if (webView && webView.contentItem) {
                webView.contentItem.visible = true
                webView.contentItem.opacity = 1.0
            }
        }

        if (newState !== "fullscreenWebPage") {
            overlay.visible = true
        }

        state = newState
    }

    state: "chromeVisible"

    // TODO: Fix real cover. Once that is fixed, we should remove this block.
    onActiveChanged: {
        // When activating and state already changed to something else than
        // "fullscreenWebPage" we should not alter the state.
        // For instance "new-tab" cover action triggers this state change.
        if (active && (state !== "fullscreenWebPage" || webView.contentItem && webView.contentItem.fullscreen)) {
            return
        }

        if (active) {
            if (webView.completed && !webView.tabModel.waitingForNewTab && webView.tabModel.count === 0) {
                updateState("fullscreenOverlay", true)
            } else {
                updateState("chromeVisible", true)
            }
        } else if (webView.tabModel.count > 0) {
            updateState("fullscreenWebPage", true)
        }
    }

    onStateChanged: {
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
            if (!active) {
                return
            }

            if (!webView.fullscreenMode) {
                updateState("chromeVisible")
            } else if (webView.fullscreenMode) {
                updateState("fullscreenWebPage")
            }
        }
    }

    Connections {
        target: webView.tabModel
        onCountChanged: {
            if (webView.completed && webView.tabModel.count === 0) {
                updateState("fullscreenOverlay")
            }

            window.setBrowserCover(webView.tabModel)
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
                    y: portrait ? overlay.toolBar.toolsHeight : 0
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
                        overlay.visible = animator.state !== "fullscreenWebPage"
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
