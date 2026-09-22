/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

var _tabSwipeBusy = false
var _hostedThumbnailCaptureBlocked = false
var _hostedThumbnailCapturePending = false
var _hostedThumbnailCaptureScheduled = false
var _hostedThumbnailCaptureSuspended = false
var _hostedThumbnailRequiredGeneration = 1
var _privateCoverPending = false
var _privateCaptureGeneration = 0
var _privateTabGrabs = {}
var _privateCoverTab = ""
var privateCoverGrab
var width = 100
var height = 200
var captures = 0
var callbacks = []
var timerStarts = 0
var timerRunning = false
var hostedThumbnailCaptureTimer = {
    "stop": function() { timerRunning = false },
    "restart": function() { timerRunning = true; ++timerStarts }
}
var hostedThumbnailGrabber = {
    "grab": function() { return ++captures },
    "invalidateAll": function() {}
}
var chromeHostView = {
    "active": true, "visible": true, "privateMode": false,
    "selectedTabId": "23", "platformFrameGeneration": 1, "loading": false,
    "width": 100, "height": 200,
    "tabModel": { "snapshot": function() {
        return [{ "tabId": "23", "location": "https://forum.example/",
                  "locationRevision": "1" }]
    } },
    "grabNativeImage": function(callback) {
        ++captures
        callbacks.push(callback)
        return true
    },
    "grabToImage": function(callback) { return this.grabNativeImage(callback) }
}
var webView = {
    "privateMode": false, "foreground": true, "nativeWindow": {},
    "privateTabModel": { "updateThumbnailPath": function() {} }
}
var browserPage = { "active": true }
var Qt = { "size": function(w, h) { return { "width": w, "height": h } } }
function selectedPersistentId() { return "17" }

(function() {
    requestHostedThumbnail()
    check(timerRunning, "An idle page schedules its requested capture")

    // A drag starts while the capture timer is pending. Repeated load/frame
    // notifications must not start captures during the following fling.
    _hostedThumbnailCaptureBlocked = true
    updateHostedThumbnailScrollState()
    check(!timerRunning)
    for (var i = 0; i < 5; ++i) {
        ++chromeHostView.platformFrameGeneration
        requestHostedThumbnail()
        continueHostedThumbnailCapture(chromeHostView)
    }
    equal(timerStarts, 1)
    capturePendingHostedThumbnail() // Even an already queued timeout is guarded.
    check(!captureHostedThumbnail(true), "The tab grid cannot bypass scrolling")
    captureTabSwipeFrame(chromeHostView, "23", 1)
    equal(captures, 0)
    check(_hostedThumbnailCapturePending)

    _hostedThumbnailCaptureBlocked = false
    updateHostedThumbnailScrollState()
    check(timerRunning, "Fling completion retries without requiring another frame")
    equal(timerStarts, 2)
    capturePendingHostedThumbnail()
    equal(captures, 1)
    check(!_hostedThumbnailCapturePending)
    equal(_hostedThumbnailRequiredGeneration, 7)

    // Tab/navigation resets must retain the accepted-frame requirement.
    resetHostedThumbnailCapture(chromeHostView)
    _hostedThumbnailCaptureBlocked = true
    updateHostedThumbnailScrollState()
    requestHostedThumbnail()
    _hostedThumbnailCaptureBlocked = false
    updateHostedThumbnailScrollState()
    check(!timerRunning)
    ++chromeHostView.platformFrameGeneration
    continueHostedThumbnailCapture(chromeHostView)
    check(timerRunning)
    capturePendingHostedThumbnail()
    equal(captures, 2)

    // Private covers use the same deferral for native and Qt Quick grabs.
    for (var nativeIndex = 0; nativeIndex < 2; ++nativeIndex) {
        webView.nativeWindow = nativeIndex ? {} : null
        webView.privateMode = chromeHostView.privateMode = true
        ++chromeHostView.platformFrameGeneration
        _hostedThumbnailCaptureBlocked = true
        updateHostedThumbnailScrollState()
        requestHostedThumbnail()
        check(!requestPrivateCover(true), "Explicit private captures also wait")
        capturePendingHostedThumbnail()
        equal(captures, 2 + nativeIndex)
        check(!_privateCoverPending)
        _hostedThumbnailCaptureBlocked = false
        updateHostedThumbnailScrollState()
        check(timerRunning)
        capturePendingHostedThumbnail()
        equal(captures, 3 + nativeIndex)
        check(!_hostedThumbnailCapturePending)
        check(_privateCoverPending)
        callbacks.shift()(null)
    }

    _hostedThumbnailCaptureBlocked = true
    updateHostedThumbnailScrollState()
    cancelHostedThumbnailCaptureForBackground()
    _hostedThumbnailCaptureBlocked = false
    updateHostedThumbnailScrollState()
    check(!timerRunning, "Scroll settling must not revive a background capture")
    return true
})()
