/*
 * Copyright (c) 2014 - 2019 Jolla Ltd.
 * Copyright (c) 2019 - 2021 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.2
import Sailfish.Silica 1.0

Item {
    id: animator

    property Item overlay
    property QtObject webView
    property string direction
    property bool portrait
    property bool atTop
    property bool atBottom: true
    property int transitionDuration: !_immediate ? (state === _certOverlay ? proportionalDuration : 250) : 0
    readonly property bool allowContentUse: state === _chromeVisible || state === _fullscreenWebPage && state !== _doubleToolBar
    readonly property bool dragging: state === _draggingOverlay
    readonly property bool secondaryTools: state === _doubleToolBar
    readonly property bool certOverlay: state === _certOverlay

    property bool opened: isOpenedState()
    property bool _immediate
    property bool _midPos

    readonly property string _fullscreenOverlay: "fullscreenOverlay"
    readonly property string _doubleToolBar: "doubleToolBar"
    readonly property string _chromeVisible: "chromeVisible"
    readonly property string _fullscreenWebPage: "fullscreenWebPage"
    readonly property string _startPage: "startPage"
    readonly property string _draggingOverlay: "draggingOverlay"
    readonly property string _certOverlay: "certOverlay"
    readonly property string _noOverlay: "noOverlay"
    property var _previousYs
    property int proportionalDuration: 400

    function showSecondaryTools() {
        updateState(_doubleToolBar)
    }

    function showChrome(immediate) {
        updateState(_chromeVisible, immediate || false)
    }

    function showStartPage(immediate) {
        updateState(_startPage, immediate || false)
    }

    function showOverlay(immediate) {
        updateState(_fullscreenOverlay, immediate || false)
    }

    function showInfoOverlay(immediate) {
        updateState(_certOverlay, immediate || false)
    }

    function drag() {
        updateState(_draggingOverlay)
    }

    function hide() {
        updateState(_noOverlay)
    }

    function isOpenedState() {
        return state !== _fullscreenOverlay && state !== _fullscreenWebPage && state !== _startPage && state !== _noOverlay
    }

    // Wrapper from updating the state. Handy for debugging.
    function updateState(newState, immediate) {
        _immediate = immediate || false
        if (newState !== _fullscreenWebPage) {
            overlay.visible = true
        }
        if (state === _certOverlay) {
            _midPos = true
        }

        // Update the animation time to suit the type of overlay
        if ((newState !== _noOverlay) && (newState !== _chromeVisible)) {
            if (newState === _certOverlay) {
                proportionalDuration = 600 * (1.0 - (_infoHeight / webView.fullscreenHeight))
            } else {
                proportionalDuration = 400
            }
        }

        state = newState
    }

    state: _chromeVisible
    onStateChanged: {
        // Animation end changes to true state. Hence not like atTop = state !== _fullscreenOverlay
        var wasAtMiddle = (!atBottom && !atTop) || _midPos
        var goingUp = (atBottom || wasAtMiddle) && (state === _fullscreenOverlay || state === _startPage)
        var goingDown = (atTop || wasAtMiddle) && (state === _chromeVisible || state === _fullscreenWebPage || state === _doubleToolBar || state === _noOverlay || state === _draggingOverlay || state === _certOverlay || state === _startPage )

        if ((state !== _fullscreenOverlay && state !== _certOverlay && state !== _startPage) || _midPos) {
            atTop = false
        } else if (state == _fullscreenOverlay || state == _startPage) {
            atTop = true
        }
        if ((state !== _chromeVisible && state !== _fullscreenWebPage && state !== _doubleToolBar) || _midPos) {
            atBottom = false
        }
        if (!isOpenedState()) {
            opened = false
        }
        _midPos = false

        direction = goingUp ? "upwards" : (goingDown ? "downwards" : "")
    }

    Connections {
        target: overlay
        onYChanged: {
            if (!_previousYs)
                _previousYs = []

            if (_previousYs.length > 0) {
                var lastPos = _previousYs[_previousYs.length-1]
                // Filter out movement a bit, padding medium as a hysteresis
                var hasMoved = Math.abs(lastPos - overlay.y) > Theme.paddingMedium
                if (hasMoved) _previousYs.push(overlay.y)
            } else {
                _previousYs.push(overlay.y)
            }

            if (_previousYs.length > 5)
                _previousYs.shift()

            var tmpDirection = ""
            var directionChanged = false
            for (var i = 1; i < _previousYs.length && _previousYs.length > 2; ++i) {
                var dir = _previousYs[i-1] > _previousYs[i] ? "upwards" : "downwards"
                if (tmpDirection !== "" && dir !== tmpDirection) {
                    directionChanged = true
                    break
                } else {
                    tmpDirection = dir
                }
            }

            if (directionChanged) {
                _previousYs = []
            } else {
                direction = tmpDirection
            }
        }
    }

    Connections {
        target: webView
        ignoreUnknownSignals: true

        onFullscreenModeChanged: {
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
                    y: webView.fullscreenHeight - overlay.toolBar.rowHeight
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
                    y: _fullHeight
                }
            ]
        },

        State {
            name: _startPage
            changes: [
                PropertyChanges {
                    target: overlay
                    y: webView.privateMode ? _fullHeight : 0
                    height: webView.fullscreenHeight
                },
                PropertyChanges {
                    target: overlay.toolBar
                    secondaryToolsHeight: 0
                    visible: false
                }
            ]
        },

        State {
            name: _doubleToolBar
            changes: [
                PropertyChanges {
                    target: overlay
                    y: webView.fullscreenHeight - overlay.toolBar.rowHeight
                    enabled: false
                }
            ]
        },

        State {
            name: _certOverlay
            changes: [
                PropertyChanges {
                    target: overlay
                    y: _infoHeight
                }
            ]
        }
    ]

    transitions: [
        Transition {
            id: overlayTransition
            to: "fullscreenWebPage,chromeVisible,loadProgressOverlay,fullscreenOverlay,noOverlay,doubleToolBar,certOverlay,startPage"

            SequentialAnimation {
                NumberAnimation { target: webView; property: "height"; duration: transitionDuration; easing.type: Easing.InOutQuad }
                ScriptAction {
                    script: {
                        if (animator.state === _chromeVisible || animator.state === _fullscreenWebPage || animator.state === _doubleToolBar) {
                            atBottom = true
                        } else if (animator.state === _fullscreenOverlay || animator.state === _certOverlay || animator.state === _startPage) {
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
                            _previousYs = []
                        }
                        if (isOpenedState()) {
                            opened = true
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
