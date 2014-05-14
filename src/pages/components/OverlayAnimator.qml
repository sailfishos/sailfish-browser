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

Item {
    id: animator

    property Item overlay
    property Item webView
    property bool portrait
    property bool atTop
    property bool atMiddle
    property bool atBottom: true
    property bool dragHintVisible: false
    property bool loadedOnce
    property int transitionDuration: _previousState ? 400 : 0

    readonly property bool active: Qt.application.active
    readonly property bool allowHtmlInputFocus: state === "chromeVisible" ||
                                                state === "loadProgressOverlay"
                                                || dragHintVisible
                                                //|| state === "dragHint"
    readonly property bool allowContentUse: allowHtmlInputFocus || state === "fullscreenWebPage"

    property string _previousState
    property bool _allowProgressOverlay

    function hide() {
        state = "chromeVisible"
    }

    function show() {
        state = "fullscreenOverlay"
    }

    function allowProgressOverlay() {
        _allowProgressOverlay = true
    }

    onDragHintVisibleChanged: {
        if (dragHintVisible) {
            dragHintTimer.running = true
        }
    }

    onActiveChanged: {
        //console.log("state:", state, )
        if (active) {
            state = _previousState || "chromeVisible"
            _previousState = state
        } else {
            _previousState = state
            state = "fullscreenWebPage"
        }
    }

    onStateChanged: {
        console.log("State:", state, dragHintTimer.running, loadedOnce)
        // Animation end changes to true state. Hence not like atTop = state !== "fullscreenOverlay"
        if (state !== "fullscreenOverlay") {
            atTop = false
        } if (state !== "chromeVisible" && state !== "fullscreenWebPage") {
            atBottom = false
        } if (state !== "loadProgressOverlay") {
            atMiddle = false
        }

        // State after loaded triggered state change.
        if ((dragHintTimer.count > 0 || dragHintVisible) && state !== "fullscreenWebPage") {
            if (dragHintTimer.running) {
                dragHintTimer.stop()
                ++dragHintTimer.count
            }
            dragHintVisible = false
        } else if (state === "chromeVisible" && loadedOnce) {
            dragHintTimer.running = true
            dragHintVisible = true
        }

        if (state === "loadProgressOverlay") {
            loadingTimer.running = true
        }

//            overlayPeek.stop()
//            repeatTimer.stop()
//            repeatTimer.needRepeat = false
//            dragHintVisible = false
//        } else if (state === "dragHint") {
//            overlayPeek.start()
//        }
    }

    onAllowHtmlInputFocusChanged: console.log("onAllowHtmlInputFocusChanged:", allowHtmlInputFocus)

    Timer {
        id: loadingTimer
        interval: 4000
        onTriggered: {
            if (webView.loading) {
                loadedOnce = true
                animator.state = "chromeVisible"
                console.log("loadingTimer: ", loadedOnce, animator.state)
            }
            _allowProgressOverlay = false
        }
    }

    Timer {
        id: dragHintTimer

        property int count: 0

        interval: 3000
        onTriggered: {
            dragHintVisible = false
            ++count
        }
    }

    Connections {
        target: webView
        ignoreUnknownSignals: true
        onLoadingChanged: {
            console.log("LOADING:", webView.loading)
            if (webView.loading && _allowProgressOverlay) {
                animator.state = "loadProgressOverlay"
            } else if (!webView.loading) {
                loadedOnce = true
                if (animator.state === "loadProgressOverlay") {
                    animator.state = "chromeVisible"
                }
                console.log("onLoadingChanged: ", loadedOnce, animator.state)
            }
            _allowProgressOverlay = false
        }
        onFullscreenModeChanged: {
            console.log("onFullscreenModeChanged:", webView.fullscreenMode, atTop, overlayTransition.running)
            if (!webView.fullscreenMode /*&& !atTop*/) {
                animator.state = "chromeVisible"
            } else if (webView.fullscreenMode /*&& !atTop*/) {
                animator.state = "fullscreenWebPage"
            }
        }
        onContentItemChanged: {
            if (webView.contentItem) {
                _allowProgressOverlay = true
                loadingTimer.start()
            }
        }

    }

    Connections {
        target: webView.tabModel
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
                    height: webView.fullscreenHeight
                },
                PropertyChanges {
                    target: overlay
                    y: webView.fullscreenHeight
                }
            ]
        },
        State {
            name: "chromeVisible"
            changes: [
                StateChangeScript {
                    script: console.log("chrome visible y:", (webView.fullscreenHeight - overlay.progressBar.height))
                },
                PropertyChanges {
                    target: webView
                    height: overlay.y
                },
                PropertyChanges {
                    target: overlay
                    y: webView.fullscreenHeight - overlay.toolBar.height
                }
            ]
        },
        State {
            name: "draggingOverlay"
            PropertyChanges {
                target: webView
                // was floor
                height: Math.ceil(overlay.y) > 0 ? Math.ceil(overlay.y) : 0
            }
        },
        State {
            name: "loadProgressOverlay"
            changes: [
                StateChangeScript {
                    script: console.log("loadProgressOverlay y:", (webView.fullscreenHeight - overlay.progressBar.height))
                },
                PropertyChanges {
                    target: webView
                    height: overlay.y
                },
                PropertyChanges {
                    target: overlay
                    y: webView.fullscreenHeight - overlay.progressBar.height
                }
            ]
        },
        State {
            name: "fullscreenOverlay"
            changes: [
                PropertyChanges {
                    target: webView
                    // was floor
                    height: Math.ceil(overlay.y) ? Math.ceil(overlay.y) : 0
                },
                PropertyChanges {
                    target: overlay
                    y: portrait ? overlay.toolBar.height : -overlay.toolBar.height
                }
            ]
        }
    ]

    transitions: Transition {
        id: overlayTransition
        to: "fullscreenWebPage,chromeVisible,loadProgressOverlay,fullscreenOverlay"

        onRunningChanged: console.log("TRANSITION RUNNING:", running)

        SequentialAnimation {
            NumberAnimation { target: webView; property: "height"; duration: transitionDuration; easing.type: Easing.InOutQuad }
            ScriptAction {
                script: {
                    if (animator.state === "chromeVisible") {
                        atBottom = true
                    } else if (animator.state === "fullscreenOverlay") {
                        atTop = true
                    } else if (animator.state === "loadProgressOverlay") {
                        atMiddle = true
                    }

                    if (webView.contentItem) {
                        webView.contentItem.chrome = animator.state !== "fullscreenWebPage"
                    }
                }
            }
        }
        NumberAnimation { target: overlay; property: "y"; duration: transitionDuration; easing.type: Easing.InOutQuad }
    }
}
