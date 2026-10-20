/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */
import QtQuick 2.6

Item {
    id: controller

    property var service
    property bool browserEnabled
    property var webView
    property var contentItem
    property var flows: ({})
    property var snapshot: []
    readonly property string activeFlowId: flowForTab(contentItem ? contentItem.selectedTabId : "")
    readonly property bool activeFlow: activeFlowId.length > 0
    readonly property string heading: activeFlow ? flows[activeFlowId].heading : ""
    signal empty
    signal present

    // Reassign so bindings update when entries are added or removed.
    function touch() {
        var copy = {}
        for (var id in flows) copy[id] = flows[id]
        flows = copy
    }

    function flowForTab(tabId) {
        var current = String(tabId)
        var seen = {}
        while (current && !seen[current]) {
            seen[current] = true
            for (var id in flows) {
                if (String(flows[id].runtimeId) === current) return id
            }
            var parent = ""
            for (var i = 0; i < snapshot.length; ++i) {
                if (String(snapshot[i].tabId) === current) parent = String(snapshot[i].openerId || "")
            }
            current = parent
        }
        return ""
    }

    function open(id, url, purpose, heading) {
        if (!browserEnabled) {
            service.failed(id, "load")
            service.closed(id, "failed")
            return
        }
        webView.reopen()
        var tabId = webView.tabModel.newTab(url, false)
        if (tabId <= 0) {
            service.failed(id, "load")
            service.closed(id, "failed")
            return
        }
        flows[id] = { "persistentId": String(tabId), "runtimeId": "", "purpose": purpose, "heading": heading || "",
                      "closing": false, "seen": false, "url": "", "title": "" }
        touch()
        present()
    }

    function close(id, reason) {
        var flow = flows[id]
        if (!flow || flow.closing) return
        flow.closing = true
        flow.reason = reason || "completed"
        if (!flow.runtimeId.length) {
            // Creation may already be in Gecko's command queue. Retain the
            // ownership until its snapshot arrives, then close that exact tab.
            return
        }
        var ids = []
        for (var i = snapshot.length - 1; i >= 0; --i) {
            if (flowForTab(String(snapshot[i].tabId)) === id) ids.push(Number(snapshot[i].persistentId))
        }
        for (i = 0; i < ids.length; ++i) webView.closeTab(ids[i])
    }

    function finish(id, reason) {
        if (!flows[id]) return
        delete flows[id]
        touch()
        service.closed(id, reason)
        if (!snapshot.length && !Object.keys(flows).length) empty()
    }

    function applySnapshot(tabs) {
        snapshot = tabs
        // Resolve roots first: a popup and its opener can be acknowledged in
        // the same snapshot, before either runtime id was known to the UI.
        for (var id in flows) {
            for (var i = 0; i < tabs.length; ++i) {
                if (String(tabs[i].persistentId) === flows[id].persistentId) {
                    flows[id].runtimeId = String(tabs[i].tabId)
                }
            }
        }
        // These callers previously disabled window.open. Keep that policy
        // local to owned flows without changing network portal preferences.
        for (var child = 0; child < tabs.length; ++child) {
            var owner = flowForTab(String(tabs[child].tabId))
            if (owner && String(tabs[child].persistentId) !== flows[owner].persistentId) {
                webView.closeTab(Number(tabs[child].persistentId))
            }
        }
        for (id in flows) {
            var flow = flows[id]
            var tab = null
            for (i = 0; i < tabs.length; ++i) {
                if (String(tabs[i].persistentId) === flow.persistentId) tab = tabs[i]
            }
            if (!tab) {
                if (flow.seen) {
                    var children = []
                    for (var j = 0; j < tabs.length; ++j) {
                        if (flowForTab(String(tabs[j].tabId)) === id) children.push(Number(tabs[j].persistentId))
                    }
                    for (j = 0; j < children.length; ++j) webView.closeTab(children[j])
                    if (!children.length) finish(id, flow.reason || "cancelled")
                }
                continue
            }
            flow.seen = true
            flow.runtimeId = String(tab.tabId)
            if (flow.closing) {
                // A close may arrive while Gecko is acknowledging creation.
                webView.closeTab(Number(flow.persistentId))
            } else if (flow.url !== String(tab.location) || flow.title !== String(tab.title)) {
                flow.url = String(tab.location)
                flow.title = String(tab.title)
                service.updated(id, flow.url, flow.title)
            }
        }
        touch()
    }

    function interceptCallback(tabId, url) {
        var id = flowForTab(tabId)
        if (!id || !service.callbackMatches(id, String(url))) return false
        var flow = flows[id]
        if (!flow.closing) service.updated(id, String(url), "")
        return true
    }

    function link(url) {
        if (!activeFlow) return false
        if (interceptCallback(contentItem.selectedTabId, url)) return true
        if (service.webUrl(String(url))) {
            contentItem.load(String(url), false)
            return true
        }
        return false
    }

    function loadFinished() {
        if (!activeFlow || flows[activeFlowId].closing || contentItem.loading || !contentItem.loaded) return
        if (String(contentItem.url).indexOf("about:neterror") === 0
                || String(contentItem.url).indexOf("about:certerror") === 0) {
            service.failed(activeFlowId, String(contentItem.url).indexOf("about:certerror") === 0 ? "tls" : "load")
        } else {
            service.loaded(activeFlowId, !!contentItem.security && contentItem.security.allGood)
        }
    }

    function closeNetwork() {
        for (var i = snapshot.length - 1; i >= 0; --i) {
            if (!flowForTab(String(snapshot[i].tabId))) webView.closeTab(Number(snapshot[i].persistentId))
        }
        if (!Object.keys(flows).length) empty()
    }

    function closingTab(tabId) {
        var id = flowForTab(tabId)
        return !!id && (flows[id].closing || String(tabId) !== flows[id].runtimeId)
    }

    function cancelAll(waitForRemoval) {
        for (var id in flows) {
            // Downloads can retain the hidden portal process. Keep ownership
            // until those tabs close so they cannot survive into its next use.
            if (waitForRemoval) close(id, "cancelled")
            else service.closed(id, "cancelled")
        }
        // Otherwise the engine is shutting down with the window and destroys
        // all tabs; report cancellation before the bus service disappears.
        if (!waitForRemoval) flows = ({})
    }

    Connections {
        target: controller.service
        onOpenRequested: controller.open(id, url, purpose, heading)
        onCloseRequested: controller.close(id, "completed")
    }
    Connections {
        target: controller.webView ? controller.webView.tabModel : null
        onRuntimeTabReservationRejected: {
            for (var id in controller.flows) {
                if (controller.flows[id].persistentId === String(persistentId)) {
                    var flow = controller.flows[id]
                    if (!flow.closing) controller.service.failed(id, "load")
                    controller.finish(id, flow.reason || "failed")
                    break
                }
            }
        }
    }
    Connections {
        target: controller.webView
        onCompletedChanged: if (controller.service && controller.webView.completed) controller.service.setReady()
    }
    Component.onCompleted: if (service && webView && webView.completed) service.setReady()
}
