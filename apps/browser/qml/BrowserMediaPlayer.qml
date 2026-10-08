/* Copyright (C) 2026 Jolla Mobile Ltd
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
import QtQuick 2.6
import Amber.Mpris 1.0
import Sailfish.WebEngine 1.0

// Owned by the application window, independent of BrowserPage visibility.
Loader {
    id: mediaPlayer

    property var browserWindow
    readonly property var controller: WebEngine.mediaController
    readonly property bool publishable: controller.available && !controller.privateBrowsing

    active: false

    function updatePlayer() {
        // Unregister synchronously before Loader defers the old object's deletion.
        if (!publishable && item) item.serviceName = ""
        active = publishable
    }

    onPublishableChanged: updatePlayer()
    Component.onCompleted: updatePlayer()
    sourceComponent: Component {
        MprisPlayer {
            id: player

            serviceName: "sailfish_browser"
            identity: "Browser"
            desktopEntry: "sailfish-browser"
            canControl: true
            canRaise: mediaPlayer.publishable
            canPlay: mediaPlayer.publishable && mediaPlayer.controller.canPlay
            canPause: mediaPlayer.publishable && mediaPlayer.controller.canPause
            canGoNext: mediaPlayer.publishable && mediaPlayer.controller.canGoNext
            canGoPrevious: mediaPlayer.publishable && mediaPlayer.controller.canGoPrevious
            canSeek: mediaPlayer.publishable && mediaPlayer.controller.canSeek
            hasTrackList: false
            hasLoopStatus: false
            hasShuffle: false
            playbackStatus: !mediaPlayer.publishable ? Mpris.Stopped
                            : mediaPlayer.controller.playbackState === "Playing" ? Mpris.Playing
                            : Mpris.Paused
            metaData.trackId: mediaPlayer.publishable ? mediaPlayer.controller.trackId : undefined
            metaData.title: mediaPlayer.publishable ? mediaPlayer.controller.metadata.title : undefined
            metaData.contributingArtist: mediaPlayer.publishable && mediaPlayer.controller.metadata.artist
                                        ? [mediaPlayer.controller.metadata.artist] : undefined
            metaData.albumTitle: mediaPlayer.publishable ? mediaPlayer.controller.metadata.album : undefined
            metaData.duration: mediaPlayer.publishable && mediaPlayer.controller.hasPosition
                               ? Math.round(mediaPlayer.controller.duration * 1000) : undefined

            onPlayRequested: if (mediaPlayer.publishable) mediaPlayer.controller.play()
            onPauseRequested: if (mediaPlayer.publishable) mediaPlayer.controller.pause()
            onPlayPauseRequested: if (mediaPlayer.publishable) mediaPlayer.controller.playPause()
            onStopRequested: if (mediaPlayer.publishable) mediaPlayer.controller.stop()
            onNextRequested: if (mediaPlayer.publishable) mediaPlayer.controller.next()
            onPreviousRequested: if (mediaPlayer.publishable) mediaPlayer.controller.previous()
            onPositionRequested: position = mediaPlayer.publishable
                                           ? Math.round(mediaPlayer.controller.position * 1000) : 0
            onSeekRequested: {
                if (!mediaPlayer.publishable || !mediaPlayer.controller.canSeek) return
                var seconds = Math.max(0, Math.min(mediaPlayer.controller.duration,
                           mediaPlayer.controller.position + offset / 1000))
                mediaPlayer.controller.seek(mediaPlayer.controller.trackId, seconds)
            }
            onSetPositionRequested: {
                if (mediaPlayer.publishable) mediaPlayer.controller.seek(trackId, position / 1000)
            }
            property Connections positionUpdates: Connections {
                target: mediaPlayer.controller
                onSeeked: {
                    if (mediaPlayer.publishable) {
                        player.position = Math.round(seconds * 1000)
                        player.seeked(player.position)
                    }
                }
            }

            onRaiseRequested: {
                if (mediaPlayer.publishable && mediaPlayer.browserWindow.rootPage) {
                    mediaPlayer.browserWindow.rootPage.raiseMediaController(mediaPlayer.controller)
                }
            }
        }
    }
}
