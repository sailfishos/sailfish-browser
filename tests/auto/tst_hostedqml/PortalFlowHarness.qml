/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */
import QtQuick 2.6

Item {
    id: harness

    property var events: []
    property var closedTabs: []
    property int nextTabId: 100
    property bool ready

    QtObject {
        id: mockService

        signal openRequested(string id, string url, string purpose, string heading)
        signal closeRequested(string id)
        function setReady() { harness.ready = true }
        function updated(id, url, title) { harness.events.push(["updated", id, url, title]) }
        function loaded(id, secure) { harness.events.push(["loaded", id, secure]) }
        function failed(id, reason) { harness.events.push(["failed", id, reason]) }
        function closed(id, reason) { harness.events.push(["closed", id, reason]) }
        function callbackMatches(id, url) { return id === "auth" && url.indexOf("account:/done?") === 0 }
        function webUrl(url) { return url.indexOf("https://") === 0 }
    }
    QtObject {
        id: model

        signal runtimeTabReservationRejected(string persistentId)
        function newTab(url, external) { return ++harness.nextTabId }
    }
    QtObject {
        id: mockWebView

        property var tabModel: model
        property bool completed: true
        function reopen() {}
        function closeTab(id) { harness.closedTabs.push(id) }
    }
    QtObject {
        id: content

        property string selectedTabId: "10"
        property bool loading
        property bool loaded: true
        property url url: "https://provider.example/auth"
        property var security: ({"allGood": true})
        function load(url, external) { harness.events.push(["navigate", url]) }
    }
    PortalFlowController {
        id: controller

        browserEnabled: true
        service: mockService
        webView: mockWebView
        contentItem: content
    }

    function check(condition, message) { if (!condition) throw new Error(message) }
    function runTest() {
        try {
            check(ready, "cold-start service readiness")
            controller.browserEnabled = false
            mockService.openRequested("blocked", "https://provider.example/auth", "authentication", "Sign in")
            check(nextTabId === 100 && !controller.flows.blocked, "policy blocks tab creation")
            check(events.length === 2 && events[0][0] === "failed"
                  && events[1][0] === "closed" && events[1][2] === "failed",
                  "policy failure reaches caller and settles flow")
            controller.browserEnabled = true
            mockService.openRequested("auth", "https://provider.example/auth", "authentication", "Sign in")
            mockService.openRequested("terms", "https://example.com/terms", "terms", "")
            var network = {"tabId": "9", "persistentId": "90", "location": "http://network.login", "title": "Network"}
            var auth = {"tabId": "10", "persistentId": "101", "location": "https://provider.example/auth", "title": "Sign in"}
            var terms = {"tabId": "11", "persistentId": "102", "location": "https://example.com/terms", "title": "Terms"}
            controller.applySnapshot([network, auth, terms])
            check(controller.activeFlowId === "auth", "selected flow ownership")
            check(controller.heading === "Sign in", "caller heading survives snapshot")
            content.selectedTabId = "11"
            check(controller.heading === "", "unheaded flow hides heading")
            content.selectedTabId = "10"
            check(controller.interceptCallback("10", "account:/done?code=secret"), "owned custom callback captured")
            check(!controller.interceptCallback("9", "account:/done?code=secret"), "network tab cannot impersonate auth")
            check(controller.link("https://other-provider.example/consent"), "cross-origin provider link remains internal")
            controller.loadFinished()
            check(events[events.length - 1][0] === "loaded", "TLS state reported")
            mockService.closeRequested("auth")
            check(closedTabs.length === 1 && closedTabs[0] === 101, "close only the initiating account")
            controller.applySnapshot([network, terms])
            check(!controller.flows.auth && !!controller.flows.terms, "another account survives completion")
            check(events[events.length - 1][0] === "closed" && events[events.length - 1][2] === "completed", "completion after actual removal")
            controller.closeNetwork()
            check(closedTabs[closedTabs.length - 1] === 90, "legacy network close excludes owned terms")
            mockService.openRequested("early", "https://example.org", "terms", "")
            mockService.closeRequested("early")
            check(!!controller.flows.early, "retain ownership while creation is in flight")
            var early = {"tabId": "12", "persistentId": "103", "location": "https://example.org", "title": ""}
            controller.applySnapshot([network, terms, early])
            check(closedTabs[closedTabs.length - 1] === 103, "late creation is closed")
            controller.applySnapshot([network, terms])
            check(!controller.flows.early, "late creation completion settles once")
            mockService.openRequested("popup-owner", "https://example.org", "authentication", "")
            var root = {"tabId": "13", "persistentId": "104", "location": "https://example.org", "title": ""}
            var popup = {"tabId": "14", "persistentId": "105", "openerId": "13", "location": "https://example.org/popup", "title": ""}
            controller.applySnapshot([network, terms, root, popup])
            check(closedTabs[closedTabs.length - 1] === 105, "owned popup policy does not affect network tabs")
            check(controller.closingTab("14"), "owned popup cannot retain the flow with beforeunload")
            controller.applySnapshot([network, terms, popup])
            check(!!controller.flows["popup-owner"], "root removal retains ownership until child removal")
            controller.applySnapshot([network, terms])
            check(!controller.flows["popup-owner"], "popup tree fully removed before cancellation")
            mockService.openRequested("rejected", "https://example.org", "terms", "")
            mockService.closeRequested("rejected")
            model.runtimeTabReservationRejected("105")
            check(!controller.flows.rejected, "cancelled creation rejection settles ownership")
            check(events[events.length - 1][2] === "completed", "cancelled rejection does not become a load failure")
            var eventCount = events.length
            controller.cancelAll(true)
            check(!!controller.flows.terms && controller.flows.terms.closing,
                  "download-retained portal closes owned tabs before dropping ownership")
            check(closedTabs[closedTabs.length - 1] === 102 && events.length === eventCount,
                  "caller cancellation waits for retained tab removal")
            controller.applySnapshot([network])
            check(!controller.flows.terms && events[events.length - 1][2] === "cancelled",
                  "window cancellation settles after retained tab removal")
            return ""
        } catch (error) {
            return String(error)
        }
    }
}
