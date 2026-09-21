/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

var _tabSwipeBusy = false
var _hostedThumbnailCaptureBlocked = false
var _tabClosePendingId = ""
var chromeHostView
var width
var height
var _privateCoverPending
var _privateCaptureGeneration
var _privateTabGrabs
var privateCoverGrab
var _privateCoverTab
var webView
var Qt
var browserPage
var selectedPersistentId
var _hostedTabViewPending
var _hostedTabViewPersistentId
var _hostedTabViewGeneration
var hostedTabViewCaptureTimeout
var pageStack
var tabView

(function() {
    for (var nativeIndex = 0; nativeIndex < 2; ++nativeIndex) {
        var nativePresentation = nativeIndex !== 0
        var callbacks = []
        var capturePaths = []
        var updated = []
        var completed = []
        var view = {
            "privateMode": true,
            "selectedTabId": "23",
            "grabNativeImage": function(callback) {
                capturePaths.push(true)
                callbacks.push(callback)
                return true
            },
            "grabToImage": function(callback) {
                capturePaths.push(false)
                callbacks.push(callback)
                return true
            }
        }
        chromeHostView = view
        width = 100
        height = 200
        _privateCoverPending = false
        _privateCaptureGeneration = 0
        _privateTabGrabs = {}
        privateCoverGrab = null
        _privateCoverTab = ""
        webView = {
            "privateMode": true,
            "foreground": true,
            "nativeWindow": nativePresentation ? {} : null,
            "privateTabModel": {
                "updateThumbnailPath": function(id, path) {
                    updated.push([id, path])
                }
            }
        }
        Qt = { "size": function(w, h) { return { "width": w, "height": h } } }
        browserPage = {
            "hostedThumbnailGrabbed": function(id, location, revision, generation) {
                completed.push([id, location, revision, generation])
            }
        }
        selectedPersistentId = function() { return "17" }

        var capture = beginHostedTabViewThumbnailCapture()
        check(capture, "Private tab grid must wait for a capture")
        equal(completed.length, 0)
        var result = { "url": "itemgrabber:private-preview" }
        callbacks.shift()(result)
        deepEqual(updated, [[17, result.url]])
        equal(_privateTabGrabs["17"], result)
        equal(completed[0][3], capture.generation)

        beginHostedTabViewThumbnailCapture()
        view.selectedTabId = "24"
        callbacks.shift()({ "url": "itemgrabber:stale" })
        equal(updated.length, 1)

        view.selectedTabId = "23"
        var timedOutCapture = beginHostedTabViewThumbnailCapture()
        check(_privateCoverPending)
        cancelPrivateCoverCapture(timedOutCapture.generation)
        check(!_privateCoverPending)
        callbacks.shift()({ "url": "itemgrabber:timed-out" })
        equal(updated.length, 1)

        requestPrivateCover()
        var newestCapture = beginHostedTabViewThumbnailCapture()
        callbacks.shift()({ "url": "itemgrabber:older" })
        check(_privateCoverPending)
        var newestResult = { "url": "itemgrabber:newest" }
        callbacks.shift()(newestResult)
        check(!_privateCoverPending)
        deepEqual(updated[1], [17, newestResult.url])
        equal(completed[1][3], newestCapture.generation)

        prunePrivateTabGrabs([])
        equal(Object.keys(_privateTabGrabs).length, 0)
        equal(privateCoverGrab, null)
        deepEqual(capturePaths, [nativePresentation, nativePresentation,
                                nativePresentation, nativePresentation,
                                nativePresentation])
    }

    var timeoutCalls = []
    browserPage = {
        "cancelPrivateCoverCapture": function(generation) {
            timeoutCalls.push(["private", generation])
        },
        "cancelHostedThumbnailCapture": function(id, generation) {
            timeoutCalls.push(["hosted", id, generation])
        }
    }
    hostedTabViewCaptureTimeout = { "stop": function() {} }
    pageStack = { "animatorPush": function() {} }
    tabView = {}
    function finishTimedOutCapture(generation) {
        _hostedTabViewPending = true
        _hostedTabViewPersistentId = "17"
        _hostedTabViewGeneration = generation
        finishHostedTabViewCapture("17", generation, true)
    }
    finishTimedOutCapture(-7)
    finishTimedOutCapture(7)
    deepEqual(timeoutCalls, [["private", -7], ["hosted", "17", 7]])
    return true
})()
