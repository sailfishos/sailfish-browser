/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

var _pendingHostedModalRequests
var _activeHostedModalTarget

(function() {
    var host = {}
    var otherHost = {}
    function request(overrides) {
        var value = {
            "message": "embed:selectasync",
            "hostView": host,
            "tabId": "1",
            "persistentId": "10",
            "data": { "id": "123:1" }
        }
        for (var name in overrides) value[name] = overrides[name]
        return value
    }
    var matching = request({})
    var preserved = [
        request({ "hostView": otherHost }),
        request({ "tabId": "2" }),
        request({ "persistentId": "11" }),
        request({ "data": { "id": "124:1" } }),
        request({ "message": "embed:confirm" })
    ]
    var activeRequests = [matching].concat(preserved)
    for (var activeIndex = 0; activeIndex < activeRequests.length; ++activeIndex) {
        var active = activeRequests[activeIndex]
        var calls = []
        _pendingHostedModalRequests = [matching].concat(preserved)
        _activeHostedModalTarget = {
            "modalRequest": active,
            "opener": {
                "message": function(name, data) { calls.push([name, data]) }
            }
        }
        cancelHostedSelect(host, "1", "10", { "id": "123:1" })
        equal(_pendingHostedModalRequests.length, preserved.length)
        for (var index = 0; index < preserved.length; ++index) {
            equal(_pendingHostedModalRequests[index], preserved[index])
        }
        equal(calls.length, active === matching ? 1 : 0)
        if (calls.length) equal(calls[0][0], "embed:selectabort")
    }
    return true
})()
