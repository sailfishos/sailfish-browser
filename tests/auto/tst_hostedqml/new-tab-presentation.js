/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

var chromeHostView
var virtualKeyboardObserver
var _foregroundNewTabPending
var _foregroundNewTabSelected
var _foregroundNewTabKeyboardSettled
var _foregroundNewTabFrameBaseline
var _foregroundNewTabDispatchPending
var _foregroundNewTabUrl
var _foregroundNewTabFromExternal
var foregroundNewTabDispatchTimer
var webView
var dismissInputMethod

(function() {
    var view = { "platformFrameGeneration": 4 }
    var dispatchRestartCount = 0
    var dismissCount = 0
    var createdTabs = []
    chromeHostView = view
    virtualKeyboardObserver = { "opened": true, "panelSize": 300 }
    foregroundNewTabDispatchTimer = {
        "restart": function() { ++dispatchRestartCount }
    }
    webView = {
        "tabModel": {
            "newTab": function(url, fromExternal) {
                createdTabs.push({ "url": url, "fromExternal": fromExternal })
                return 17
            }
        }
    }
    dismissInputMethod = function() { ++dismissCount }

    equal(newTab("https://example.com/", true), 0)
    check(_foregroundNewTabPending)
    check(_foregroundNewTabDispatchPending)
    equal(dismissCount, 1)
    equal(createdTabs.length, 0,
          "Tab creation must wait for the full viewport")

    virtualKeyboardObserver.panelSize = 0
    scheduleForegroundNewTabDispatch()
    equal(dispatchRestartCount, 0,
          "A zero panel alone must not dispatch while the keyboard is open")

    virtualKeyboardObserver.opened = false
    scheduleForegroundNewTabDispatch()
    equal(dispatchRestartCount, 1)
    equal(dispatchForegroundNewTab(), 17)
    equal(createdTabs.length, 1)
    equal(createdTabs[0].url, "https://example.com/")
    check(createdTabs[0].fromExternal)
    check(!_foregroundNewTabDispatchPending)

    finishForegroundNewTabWait()
    virtualKeyboardObserver.opened = true
    virtualKeyboardObserver.panelSize = 300

    beginForegroundNewTabWait(view)
    check(_foregroundNewTabPending)
    check(!_foregroundNewTabSelected)
    check(!_foregroundNewTabKeyboardSettled)
    equal(_foregroundNewTabFrameBaseline, 4)

    view.platformFrameGeneration = 5
    noteForegroundNewTabFrame(view)
    check(_foregroundNewTabPending,
          "A frame before tab selection must not reveal the old viewport")

    noteForegroundNewTabSelection(view)
    check(_foregroundNewTabSelected)
    equal(_foregroundNewTabFrameBaseline, 5)

    view.platformFrameGeneration = 6
    noteForegroundNewTabFrame(view)
    check(_foregroundNewTabPending,
          "A keyboard-sized frame must remain covered")

    virtualKeyboardObserver.opened = false
    virtualKeyboardObserver.panelSize = 0
    noteForegroundNewTabKeyboardSettled()
    check(_foregroundNewTabKeyboardSettled)
    equal(_foregroundNewTabFrameBaseline, 6)
    noteForegroundNewTabFrame(view)
    check(_foregroundNewTabPending,
          "Keyboard closure must wait for a newer full-size frame")

    view.platformFrameGeneration = 7
    noteForegroundNewTabFrame(view)
    check(!_foregroundNewTabPending)

    virtualKeyboardObserver.opened = false
    virtualKeyboardObserver.panelSize = 0
    view.platformFrameGeneration = 8
    beginForegroundNewTabWait(view)
    noteForegroundNewTabSelection(view)
    view.platformFrameGeneration = 9
    noteForegroundNewTabFrame(view)
    check(!_foregroundNewTabPending,
          "A new tab without a keyboard must reveal its next frame")

    return true
})()
