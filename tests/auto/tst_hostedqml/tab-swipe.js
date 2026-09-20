/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

var _tabClosePendingId = ""
var _tabSwipePhase = TabTransition.Phase.Idle
var _tabSwipeOperation = TabTransition.Operation.None
Object.defineProperty(this, "_tabSwipeBusy", { get: function() { return _tabSwipePhase !== TabTransition.Phase.Idle } })
Object.defineProperty(this, "_tabSwipeActive", { get: function() { return _tabSwipeBusy && _tabSwipePhase !== TabTransition.Phase.AwaitingClose } })
var _tabSwipeCurrentTab
var _tabSwipePreviousTab
var _tabSwipeNextTab
var _tabSwipeTargetTab
var _tabSwipeDirection
var _tabSwipeCaptureSerial = 0
var _tabSwipeOffset
var _tabSwipeOpacity
var _tabSwipeFrameBaseline
var _tabSwipeCurrentGrab
var _tabSwipeCurrentUrl
var _tabSwipeTargetUrl
var _privateTabGrabs = {}
var _hostedViewportFitStates = {}
var _hostedCutoutTop = 24
var _hostedCutoutRight = 0
var _hostedCutoutBottom = 0
var _hostedCutoutLeft = 0
var _hostedCutoutInsetUsage = 1
var _hostedSurfaceColor = "blue"
var contentFullscreen = false
var browserPage
var chromeHostView
var tabSwipeModel
var tabSwipeSettleMotion
var tabSwipeSettleAnimation
var tabSwipeWaitTimer
var tabSwipeFadeAnimation
var requestHostedThumbnail
var hostedThumbnailGrabber
var webView
var width = 400
var height = 800
var Qt = { rect: function(x, y, width, height) { return { x: x, y: y, width: width, height: height } },
           size: function(width, height) { return { width: width, height: height } } }

;(function() {
    var captures = 0
    var saves = 0
    var closes = 0
    var animations = 0
    var fades = 0
    var finishes = 0
    var activatedId = 0
    var ids = [1, 2, 3, 4]
    var runtimeTabs = ids.map(function(id) {
        return { "tabId": String(id * 10), "persistentId": String(id),
                 "location": "https://tab" + id + ".example/", "locationRevision": "1" }
    })
    captureTabSwipeFrame = function() { ++captures }
    hostedRuntimeTabByRuntimeId = function(view, id) {
        for (var i = 0; i < runtimeTabs.length; ++i) {
            if (runtimeTabs[i].tabId === id) return runtimeTabs[i]
        }
        return null
    }
    browserPage = { "url": "https://tab2.example/" }
    chromeHostView = {
        "selectedTabId": "20", "platformFrameGeneration": 7,
        "width": 400, "height": 776,
        "mapToItem": function() { return { "x": 0, "y": 24 } },
        "tabModel": { "selectedTabOpenerId": "10" }
    }
    tabSwipeModel = { "activeTabIndex": 1, "count": 4,
        "get": function(index) {
            var id = ids[index]
            return id ? { "tabId": id, "url": "https://tab" + id + ".example/",
                          "thumbnailPath": "preview" + id } : {}
        }
    }
    tabSwipeSettleMotion = {}
    tabSwipeSettleAnimation = {
        "restart": function() { ++animations },
        "stop": function() { finishTabSwipeSettle() }
    }
    tabSwipeFadeAnimation = {
        "restart": function() { ++fades },
        "stop": function() { check(_tabSwipePhase === TabTransition.Phase.Idle, "retire state before stopping animations") }
    }
    requestHostedThumbnail = function() { ++finishes }
    tabSwipeWaitTimer = { "restart": function() {}, "stop": function() {} }
    webView = {
        "privateMode": false,
        "cutoutGuardConfig": { "value": "strict" }, "_defaultThemeColor": "white",
        "tabModel": {
            "runtimeIdForPersistentId": function(id) { return ids.indexOf(Number(id)) < 0 ? "" : String(Number(id) * 10) },
            "activateTabById": function(id) {
                if (ids.indexOf(id) < 0) return false
                activatedId = id
                return true
            },
            "closeActiveTab": function() { ++closes }
        }
    }
    hostedThumbnailGrabber = { "saveGrab": function(grab, id, location, revision, size) {
        equal(grab, _tabSwipeCurrentGrab)
        equal(id, "2")
        equal(location, "https://tab2.example/")
        equal(revision, "1")
        equal(size.height, 776)
        ++saves
        return 1
    } }
    equal(beginTabSwipe(), true)
    equal(captures, 1)
    equal(_tabSwipeCurrentTab.tabId, 2)
    equal(_tabSwipeCurrentTab.rect.y, 24, "keep live content below the notch")
    equal(_tabSwipeCurrentTab.rect.height, 776)
    equal(_tabSwipeNextTab.rect.y, 24, "guard the incoming preview too")
    equal(tabSwipeTargetForDistance(-1).tabId, 3)
    equal(tabSwipeTargetForDistance(1).tabId, 1)
    equal(tabSwipeTargetForDistance(0), null)
    updateTabSwipe(-120)
    equal(_tabSwipeDirection, 1)
    equal(_tabSwipeOffset, -120)
    equal(_tabSwipeTargetUrl, "https://tab3.example/")
    updateTabSwipe(80)
    equal(_tabSwipeDirection, -1)
    equal(_tabSwipeTargetTab.tabId, 1)
    _tabSwipePreviousTab = null
    updateTabSwipe(100)
    equal(_tabSwipeOffset, 25, "resist the edge")

    // Removing an earlier row must not change the destination after release.
    endTabSwipe(-120, true)
    equal(tabSwipeSettleMotion.to, -400)
    ids.shift()
    _tabSwipeCurrentGrab = { "url": "image://qmoznative/1" }
    finishTabSwipeSettle()
    equal(activatedId, 3)
    equal(captures, 1, "reuse the existing capture")
    equal(saves, 1)
    finishTabSwipeSettle()
    equal(saves, 1, "duplicate settle completion cannot save or select again")

    // Outgoing frames before selection must never satisfy readiness.
    chromeHostView.platformFrameGeneration = 8
    noteTabSwipeFrame(chromeHostView)
    equal(fades, 0)
    chromeHostView.selectedTabId = "30"
    noteTabSwipeSelection(chromeHostView)
    equal(_tabSwipeFrameBaseline, 8)
    equal(fades, 0)
    noteTabSwipeFrame(chromeHostView)
    equal(fades, 0)
    chromeHostView.platformFrameGeneration = 9
    noteTabSwipeFrame(chromeHostView)
    equal(fades, 1)
    chromeHostView.selectedTabId = "40"
    noteTabSwipeFrame(chromeHostView)
    equal(fades, 1, "a different selected tab cannot complete the transition")
    noteTabSwipeSelection(chromeHostView)
    equal(_tabSwipeActive, false)

    // A removed destination cancels rather than selecting another row.
    ids = [1, 2, 3, 4]
    chromeHostView.selectedTabId = "20"
    beginTabSwipe()
    endTabSwipe(-100, true)
    ids.splice(2, 1)
    activatedId = 0
    finishTabSwipeSettle()
    equal(activatedId, 0)
    equal(_tabSwipeActive, false)

    ids = [1, 2, 3, 4]
    beginTabSwipe()
    endTabSwipe(30, false)
    finishTabSwipeSettle()
    equal(_tabSwipeActive, false, "cancelled gesture returns to live content")

    // Teardown clears the transaction before animation stop callbacks run.
    beginTabSwipe()
    endTabSwipe(-100, true)
    finishTabSwipe()
    equal(_tabSwipePhase, TabTransition.Phase.Idle)
    equal(_tabSwipeOperation, TabTransition.Operation.None)
    equal(_tabSwipeTargetTab, null)
    equal(_tabSwipeCurrentGrab, null)
    equal(_tabSwipeFrameBaseline, -1)
    activatedId = 0
    finishTabSwipeSettle()
    equal(activatedId, 0, "late settle callback cannot select after cancellation")
    startTabSwipeFade()
    equal(_tabSwipePhase, TabTransition.Phase.Idle, "late timeout cannot restart a cancelled fade")

    // A timeout can reveal the live page, and another gesture starts cleanly.
    beginTabSwipe()
    endTabSwipe(-100, true)
    finishTabSwipeSettle()
    equal(_tabSwipePhase, TabTransition.Phase.WaitingSelection)
    startTabSwipeFade()
    equal(_tabSwipePhase, TabTransition.Phase.Fading)
    var fadeCount = fades
    startTabSwipeFade()
    equal(fades, fadeCount, "a fade starts only once")
    finishTabSwipe()
    equal(beginTabSwipe(), true)
    equal(_tabSwipePhase, TabTransition.Phase.Dragging)
    equal(_tabSwipeFrameBaseline, -1)
    finishTabSwipe()

    // Freeze destination guard policy and reject stale viewport-fit metadata.
    _hostedViewportFitStates["30"] = {
        "persistentId": "3", "locationRevision": "1", "viewportFit": "cover",
        "safeAreaInsetUsage": 0, "themeColor": "red"
    }
    equal(tabSwipeTabAt(2).rect.y, 0)
    webView.cutoutGuardConfig.value = "top_guard"
    equal(tabSwipeTabAt(2).rect.y, 24)
    _hostedViewportFitStates["30"].safeAreaInsetUsage = 1
    equal(tabSwipeTabAt(2).rect.y, 0)
    _hostedViewportFitStates["30"].locationRevision = "old"
    equal(tabSwipeTabAt(2).rect.y, 24)
    _hostedCutoutTop = 0
    _hostedCutoutLeft = 24
    equal(tabSwipeTabAt(2).rect.x, 24)
    equal(tabSwipeTabAt(2).rect.width, 376)

    // Back-and-close keeps prompts visible, then follows confirmed selection.
    closeTabWithTransition()
    equal(closes, 1)
    equal(_tabClosePendingId, "20")
    equal(_tabSwipeActive, false)
    closeTabWithTransition()
    equal(closes, 1)
    noteTabCloseResult(chromeHostView, "99", false)
    equal(_tabClosePendingId, "20")
    noteTabCloseResult(chromeHostView, "20", false)
    equal(_tabSwipeActive, false)
    closeTabWithTransition()
    chromeHostView.selectedTabId = "10"
    noteTabSwipeSelection(chromeHostView)
    equal(_tabSwipeFrameBaseline, chromeHostView.platformFrameGeneration)
    var baseline = _tabSwipeFrameBaseline
    noteTabCloseResult(chromeHostView, "20", true)
    equal(_tabSwipeActive, true)
    equal(tabSwipeSettleMotion.to, 400)
    var priorFades = fades
    finishTabSwipeSettle()
    equal(fades, priorFades)
    chromeHostView.platformFrameGeneration = baseline + 1
    noteTabSwipeFrame(chromeHostView)
    equal(fades, priorFades + 1)
    finishTabSwipe()
    chromeHostView.selectedTabId = "20"
    closeTabWithTransition()
    chromeHostView.selectedTabId = "40"
    var priorAnimations = animations
    noteTabCloseResult(chromeHostView, "20", true)
    equal(animations, priorAnimations, "opener disappeared during close")

    chromeHostView.selectedTabId = "20"
    chromeHostView.tabModel.selectedTabOpenerId = "missing"
    closeTabWithTransition()
    equal(closes, 4)
    equal(_tabSwipeActive, false)
    tabSwipeModel.count = 1
    equal(beginTabSwipe(), false)
    return true
})()
