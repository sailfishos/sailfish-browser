/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 2.0
import Sailfish.Media 1.0
import org.freedesktop.contextkit 1.0

// QtObject cannot have children
Item {
    property Item webView
    property bool firstFrameRendered

    property bool videoActive
    property bool audioActive
    property bool background

    property bool _suspendable
    property string _mediaState: "pause"
    property string _lastStateOwner
    property string _lastMetaOwner
    property bool _isAudioStream
    property bool _isVideoStream

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

    onAudioActiveChanged: {
        if (!audioActive && screenBlanked.value) {
            _suspendable = true
        }
    }

    onBackgroundChanged: {
        if (!audioActive && !videoActive && background) {
            _suspendable = true
        } else {
            _suspendable = false
        }
    }

    on_SuspendableChanged: {
        if (_suspendable) {
            webView.suspendView()
        } else {
            webView.resumeView()
        }
    }

    Connections {
        target: MozContext
        onRecvObserve: {
            if (message === "embedlite-before-first-paint") {
                firstFrameRendered = true
            } else if (message === "media-decoder-info") {
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
                _suspendable = true
            } else {
                _suspendable = false
            }
        }
    }

    ScreenBlank {
        suspend: videoActive
    }
}
