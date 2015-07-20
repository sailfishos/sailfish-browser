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
    property QtObject webView
    property string direction
    property bool portrait
    property bool atTop
    property bool atBottom: true
    property int transitionDuration: !_immediate ? 400 : 0
    property real openYPosition: portrait ? overlay.toolBar.toolsHeight : 0

    property bool active
    readonly property bool allowContentUse: state === _chromeVisible || state === _fullscreenWebPage || state === _doubleToolBar
    readonly property bool dragging: state === _draggingOverlay
    readonly property bool secondaryTools: state === _doubleToolBar

    property bool _immediate

    readonly property string _fullscreenOverlay: "fullscreenOverlay"
    readonly property string _doubleToolBar: "doubleToolBar"
    readonly property string _chromeVisible: "chromeVisible"
    readonly property string _fullscreenWebPage: "fullscreenWebPage"
    readonly property string _draggingOverlay: "draggingOverlay"
    readonly property string _noOverlay: "noOverlay"

    function showSecondaryTools() {
        updateState(_doubleToolBar)
    }

    function showChrome(immediate) {
        updateState(_chromeVisible, immediate || false)
    }

    function showOverlay(immediate) {
        updateState(_fullscreenOverlay, immediate || false)
    }

    function drag() {
        updateState(_draggingOverlay)
    }

    function hide() {
        updateState(_noOverlay)
    }

    // Wrapper from updating the state. Handy for debugging.
    function updateState(newState, immediate) {
        _immediate = immediate || false
        if (newState !== _fullscreenWebPage) {
            overlay.visible = true
        }

        state = newState
    }

    state: _chromeVisible

    // TODO: Fix real cover. Once that is fixed, we should remove this block.
    onActiveChanged: {
        // When activating and state already changed to something else than
        // _fullscreenWebPage we should not alter the state.
        // For instance "new-tab" cover action triggers this state change.
        if (active && (state !== _fullscreenWebPage || webView.contentItem && webView.contentItem.fullscreen)) {
            return
        }

        if (active) {
            if (webView.completed && !webView.tabModel.waitingForNewTab && webView.tabModel.count === 0) {
                updateState(_fullscreenOverlay, true)
            } else {
                updateState(_chromeVisible, true)
            }
        } else if (webView.tabModel.count > 0) {
            updateState(_fullscreenWebPage, true)
        }
    }

    onStateChanged: {
        // Animation end changes to true state. Hence not like atTop = state !== _fullscreenOverlay
        var wasAtMiddle = !atBottom && !atTop
        var goingUp = (atBottom || wasAtMiddle) && state === _fullscreenOverlay
        var goingDown = (atTop || wasAtMiddle) && (state === _chromeVisible || state === _fullscreenWebPage || state === _doubleToolBar || state === _noOverlay || state == _draggingOverlay)

        if (state !== _fullscreenOverlay) {
            atTop = false
        } if (state !== _chromeVisible && state !== _fullscreenWebPage && state !== _doubleToolBar) {
            atBottom = false
        }

        direction = goingUp ? "upwards" : (goingDown ? "downwards" : "")
    }

    Connections {
        target: webView
        ignoreUnknownSignals: true

        onFullscreenModeChanged: {
            if (!active) {
                return
            }

            if (webView.fullscreenMode) {
                updateState(_fullscreenWebPage)
            } else {
                updateState(_chromeVisible)
            }
        }
    }

    states: [
        State {
            name: _fullscreenWebPage
            changes: [
                PropertyChanges {
                    target: overlay
                    y: webView.fullscreenHeight
                }
            ]
        },
        State {
            name: _noOverlay
            extend: _fullscreenWebPage
        },
        State {
            name: _chromeVisible
            changes: [
                PropertyChanges {
                    target: overlay
                    y: webView.fullscreenHeight - overlay.toolBar.toolsHeight
                }
            ]
        },
        State {
            name: _draggingOverlay
            changes: [
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
            name: _fullscreenOverlay
            changes: [
                PropertyChanges {
                    target: overlay
                    y: openYPosition
                }
            ]
        },

        State {
            name: _doubleToolBar
            changes: [
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
                        if (animator.state === _chromeVisible || animator.state === _fullscreenWebPage || animator.state === _doubleToolBar) {
                            atBottom = true
                        } else if (animator.state === _fullscreenOverlay) {
                            atTop = true
                        }

                        if (webView.contentItem && !webView.contentItem.fullscreen) {
                            webView.contentItem.chrome = animator.state !== _fullscreenWebPage
                        }
                        _immediate = false
                        overlay.visible = animator.state !== _fullscreenWebPage && animator.state !== _noOverlay

                        // Target reached, clear it.
                        if (atBottom || atTop) {
                            direction = ""
                        }
                    }
                }
            }
            NumberAnimation { target: overlay; property: "y"; duration: transitionDuration; easing.type: Easing.InOutQuad }
            NumberAnimation { target: overlay.toolBar; property: "secondaryToolsHeight"; duration: transitionDuration; easing.type: Easing.InOutQuad }
        }
        ,
        Transition {
            to: _draggingOverlay
            NumberAnimation { target: overlay.toolBar; property: "secondaryToolsHeight"; duration: transitionDuration; easing.type: Easing.InOutQuad }
        }
    ]
}
