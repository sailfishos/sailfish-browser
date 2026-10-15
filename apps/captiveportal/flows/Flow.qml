/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */
import QtQuick 2.6
import Nemo.DBus 2.0

Item {
    id: flow

    readonly property bool active: _requestId.length > 0
    property string _requestId
    property bool _accepted
    property bool _available
    property bool _closing
    property string _closeReason
    property string heading
    property var returnWindow
    property bool destroyOnClose

    signal updated(string url, string title)
    signal loaded(bool secure)
    signal failed(string reason)
    signal closed(string reason)

    function open(url, purpose, callbackUrl) {
        if (active) return false
        _requestId = "flow-" + Date.now() + "-" + Math.random().toString(16).slice(2)
        _accepted = false
        _available = portal.status === DBusInterface.Available
        _closing = false
        _closeReason = ""
        var id = _requestId
        portal.call("openFlowWithHeading", [id, String(url), purpose, String(callbackUrl || ""), heading], function() {
            if (_requestId === id) {
                _accepted = true
                waitForEvent(id)
            }
        }, function() {
            if (_requestId === id) {
                failed("unavailable")
                finish("failed")
            }
        })
        return true
    }

    function waitForEvent(id) {
        portal.call("waitForEvent", [id], function(event) {
            if (_requestId !== id) return
            if (event.event === "closed") {
                finish(_closeReason || event.reason)
                return
            }
            // Keep a wait outstanding before handlers can close or destroy us.
            waitForEvent(id)
            if (_closing) return
            if (event.event === "updated") updated(event.url, event.title)
            else if (event.event === "loaded") loaded(event.secure)
            else if (event.event === "failed") failed(event.reason)
        }, function() {
            if (_requestId !== id) return
            failed("unavailable")
            close("failed")
            finish("failed")
        })
    }

    function close(reason) {
        if (!active || _closing) return
        _closing = true
        _closeReason = reason || "completed"
        var id = _requestId
        portal.call("closeFlow", [id], function() {}, function() {
            if (_requestId === id) {
                failed("unavailable")
                finish("failed")
            }
        })
    }

    function finish(reason) {
        if (!active) return
        _requestId = ""
        _accepted = false
        _closing = false
        if (returnWindow) returnWindow.activate()
        closed(reason)
        if (destroyOnClose) destroy()
    }

    DBusInterface {
        id: portal

        service: "org.sailfishos.captiveportal"
        path: "/flows"
        iface: "org.sailfishos.captiveportal.Flow1"
        signalsEnabled: false
        watchServiceStatus: true
        onStatusChanged: {
            if (status === DBusInterface.Available) flow._available = true
            if (status === DBusInterface.Unavailable && flow.active
                    && (flow._accepted || flow._available)) {
                flow.failed("unavailable")
                flow.finish("failed")
            }
        }
    }

    Component.onDestruction: {
        if (active && !_closing) portal.call("closeFlow", [_requestId])
    }
}
