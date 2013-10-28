/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

import QtQuick 2.0
import org.freedesktop.contextkit 1.0

Item {

    property bool suspendable
    property bool videoActive
    property bool audioActive

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
            suspendable = true
        }
    }

    ContextProperty {
        id: screenBlanked

        key: "Screen.Blanked"
        value: 0

        onValueChanged: {
            if (value && !audioActive) {
                suspendable = true
            } else {
                suspendable = false
            }
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
}
