/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Media 1.0
import org.freedesktop.contextkit 1.0

// QtObject cannot have children
Item {
    property Item webView
    property bool videoActive
    property bool audioActive
    property bool background

    property string _mediaState: "pause"
    property string _lastStateOwner
    property string _lastMetaOwner
    property bool _isAudioStream
    property bool _isVideoStream

    signal webViewSuspended

    function calculateStatus() {
        var video = false
        var audio = false

        if (_mediaState === "play" && _lastStateOwner === _lastMetaOwner) {
            if (_isVideoStream) {
                video = true
            }
            if (_isAudioStream) {
                audio = true
            }
        }

        if (videoActive !== video) {
            videoActive = video
        }
        if (audioActive !== audio) {
            audioActive = audio
        }
    }

    function resumeView() {
        if (webView) {
            webView.resumeView()
        }
    }

    function suspendView() {
        if (webView) {
            webView.suspendView()
        }
        webViewSuspended()
    }

    onAudioActiveChanged: {
        if (!audioActive && screenBlanked.value) {
            delayedSuspend.suspendIntention = true
        }
    }

    // This is behind 1000ms timer
    onBackgroundChanged: {
        if (!audioActive && !videoActive && background) {
            suspendView()
        } else {
            resumeView()
        }
    }

    Connections {
        target: MozContext
        onRecvObserve: {
            if (message === "media-decoder-info") {
                if (data.state === "meta") {
                    _isAudioStream = data.a
                    _isVideoStream = data.v
                    _lastMetaOwner = data.owner
                } else {
                    _mediaState = data.state
                    _lastStateOwner = data.owner
                }
                calculateStatus()
            }
        }
    }

    ContextProperty {
        id: screenBlanked
        key: "Screen.Blanked"
        value: 0

        onValueChanged: {
            if (value && !audioActive) {
                delayedSuspend.suspendIntention = true
            } else {
                // Return immediately from suspend.
                resumeView()
                // We want also reset suspendIntention to false.
                delayedSuspend.suspendIntention = false
            }
        }
    }

    ScreenBlank {
        // This is stopping ScreenBlank timer.
        suspend: videoActive
    }

    Timer {
        id: delayedSuspend
        property bool suspendIntention

        // 1000ms was not enough to always allow buffering start of next song via 3G
        interval: 5000
        onTriggered: {
            if (!videoActive && !audioActive && suspendIntention) {
                suspendView()
            } else {
                resumeView()
            }
        }

        onSuspendIntentionChanged: {
            if (suspendIntention) {
                start()
            }
        }
    }
}
