/* SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0 */
import QtQml 2.2

QtObject {
    property bool available: true
    property bool privateBrowsing
    property string playbackState: "Playing"
    property string trackId: "/org/mpris/MediaPlayer2/track/t1"
    property var metadata: ({title: "Test", artist: "Artist", album: "Album"})
    property bool hasPosition: true
    property real duration: 120
    property real position: 10
    property bool canPlay: true
    property bool canPause: true
    property bool canGoNext: true
    property bool canGoPrevious: true
    property bool canSeek: true
    signal seeked(real seconds)
}
