/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

var selectedPersistentId
var chromeHostView
var webView

(function() {
    var directions = ["Back", "Forward"]
    for (var directionIndex = 0; directionIndex < directions.length; ++directionIndex) {
        var direction = directions[directionIndex]
        var method = direction === "Back" ? goBack : goForward
        var persistentIds = ["17", ""]
        for (var idIndex = 0; idIndex < persistentIds.length; ++idIndex) {
            var persistentId = persistentIds[idIndex]
            for (var acceptedIndex = 0; acceptedIndex < 2; ++acceptedIndex) {
                var accepted = acceptedIndex !== 0
                var calls = []
                selectedPersistentId = function() { return persistentId }
                chromeHostView = {}
                chromeHostView["go" + direction] = function() { calls.push("native") }
                webView = { "tabModel": {} }
                webView.tabModel["runtimeGo" + direction] = function(id) {
                    equal(id, persistentId)
                    calls.push("bookkeeping")
                    return accepted
                }
                method()
                deepEqual(calls, persistentId
                          ? ["bookkeeping", "native"] : ["native"])
            }
        }

        var legacyCalls = 0
        chromeHostView = null
        webView = {}
        webView["go" + direction] = function() { ++legacyCalls }
        method()
        equal(legacyCalls, 0,
              "Do not use a native fallback while the QML view initializes")
    }
    return true
})()
