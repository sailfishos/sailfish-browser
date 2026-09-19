/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

var cancelCalls = []
var queueCalls = []
var sentMessages = []

function cancelHostedDate(hostView, tabId, persistentId, data) {
    cancelCalls.push([hostView, tabId, persistentId, data])
}

function cancelHostedSelect() {
    throw new Error("select cancellation must not handle date requests")
}

function queueHostedModalRequest(kind, hostView, tabId, persistentId, message, data) {
    queueCalls.push([kind, hostView, tabId, persistentId, message, data])
    return true
}

function sendHostedMessageToTab(hostView, tabId, persistentId, message, data) {
    sentMessages.push([hostView, tabId, persistentId, message, data])
}

(function() {
    var host = {}
    var data = { "winId": 42, "id": "42:1" }

    equal(openHostedPicker(host, "7", "11", "embed:datepicker", data), true)
    deepEqual(queueCalls, [["picker", host, "7", "11", "embed:datepicker", data]])

    equal(openHostedPicker(host, "7", "11", "embed:datepickerabort", data), true)
    deepEqual(cancelCalls, [[host, "7", "11", data]])

    rejectHostedModalRequest({
        "hostView": host,
        "tabId": "7",
        "persistentId": "11",
        "message": "embed:datepicker",
        "data": data
    })
    deepEqual(sentMessages, [[host, "7", "11", "embedui:datepickerresponse", {
        "winId": 42,
        "id": "42:1",
        "accepted": false,
        "year": 0,
        "month": 0,
        "day": 0
    }]])

    return true
})()
