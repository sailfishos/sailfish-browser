/****************************************************************************
**
** Copyright (c) 2013 - 2019 Jolla Ltd.
** Copyright (c) 2019 Open Mobile Platform LLC.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Nemo.KeepAlive 1.2
import Sailfish.WebEngine 1.0
import Nemo.DBus 2.0
import MeeGo.Connman 0.2
import org.nemomobile.policy 1.0

// QtObject cannot have children
Item {
    property QtObject webPage
    property bool videoActive
    property bool audioActive
    readonly property alias displayOff: screenBlanked.blanked
    property bool background

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

    function resumeView() {
        if (webPage) {
            webPage.resumeView()
        }
    }

    function suspendView() {
        if (webPage) {
            webPage.suspendView()
        }
    }

    onAudioActiveChanged: {
        if (!audioActive && screenBlanked.blanked) {
            delayedSuspend.suspendIntention = true
        }
    }

    // This is behind 1000ms timer
    onBackgroundChanged: {
        if (!audioActive && background) {
            suspendView()
        } else if (!background) {
            resumeView()
        }
    }

    Connections {
        target: WebEngine

        onInitialized: networkManager.notifyOfflineStatus()
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

    NetworkManager {
        id: networkManager
        readonly property bool online: state == "online" || state == "ready"

        function notifyOfflineStatus() {
            if (WebEngine.isInitialized) {
                WebEngine.notifyObservers("embed-network-link-status",
                                          {
                                              "offline": !networkManager.online
                                          })
            }
        }

        onOnlineChanged: notifyOfflineStatus()
    }

    DBusInterface {
        id: screenBlanked

        property bool blanked

        function display_status_ind(state) {
            blanked = (state === "off")
        }

        bus: DBus.SystemBus
        service: 'com.nokia.mce'
        path: '/com/nokia/mce/signal'
        iface: 'com.nokia.mce.signal'
        signalsEnabled: true

        onBlankedChanged: {
            if (blanked && !audioActive) {
                delayedSuspend.suspendIntention = true
            } else {
                // Return immediately from suspend.
                resumeView()
                // We want also reset suspendIntention to false.
                delayedSuspend.suspendIntention = false
            }
        }
    }

    DisplayBlanking {
        // This is stopping screen blank timer.
        preventBlanking: videoActive
    }

    Timer {
        id: delayedSuspend
        property bool suspendIntention

        // 1000ms was not enough to always allow buffering start of next song via 3G
        interval: 5000
        onTriggered: {
            if (!audioActive && suspendIntention) {
                suspendView()
            } else {
                resumeView()
            }
            // Reset suspend intention.
            suspendIntention = false
        }

        onSuspendIntentionChanged: {
            if (suspendIntention) {
                start()
            }
        }
    }

    Permissions {
        enabled: audioActive || videoActive
        applicationClass: "player"
        autoRelease: true

        Resource {
            type: Resource.AudioPlayback
        }

        Resource {
            type: Resource.VideoPlayback
        }
    }
}
