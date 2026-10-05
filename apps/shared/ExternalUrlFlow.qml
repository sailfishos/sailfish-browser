/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import Sailfish.WebView.Popups 1.0 as Popups

Item {
    id: flow

    property var contentItem
    property var webView
    property var pageStack
    property bool automatic
    property var requests: ({})
    property var promptQueue: []
    property var prompt
    property var promptRequest

    function current(request) {
        if (!contentItem || !request) return false
        var tabs = contentItem.tabModel.snapshot()
        for (var i = 0; i < tabs.length; ++i) {
            if (String(tabs[i].tabId) === String(request.tabId)
                    && String(tabs[i].locationRevision) === String(request.locationRevision)) return true
        }
        return false
    }

    function respond(request, outcome) {
        if (!requests[String(request.id)]) return
        var remembered = !!requests[String(request.id)].remember
        delete requests[String(request.id)]
        handler.cancel(String(request.id))
        if (Object.keys(requests).length === 0) expiryTimer.stop()
        contentItem.sendAsyncMessageToTab(String(request.tabId), "externalurlresponse", {
            "winId": request.winId, "id": request.id, "outcome": outcome, "remember": remembered,
            "fallback": outcome === "fallback" ? request.fallback : ""
        })
    }

    function closeFlow(request) {
        var tabs = contentItem.tabModel.snapshot()
        var parents = {}
        for (var i = 0; i < tabs.length; ++i) parents[String(tabs[i].tabId)] = String(tabs[i].openerId || "")
        var root = String(request.tabId)
        var seen = {}
        while (parents[root] && !seen[root]) { seen[root] = true; root = parents[root] }
        var ids = []
        for (i = 0; i < tabs.length; ++i) {
            var ancestor = String(tabs[i].tabId)
            seen = {}
            while (parents[ancestor] && ancestor !== root && !seen[ancestor]) {
                seen[ancestor] = true
                ancestor = parents[ancestor]
            }
            if (ancestor === root) ids.push(Number(tabs[i].persistentId))
        }
        for (i = ids.length - 1; i >= 0; --i) webView.closeTab(ids[i])
    }

    function reject(request) {
        if (current(request)) respond(request, request.fallback ? "fallback" : "declined")
        else respond(request, "cancelled")
    }

    function cancelAll() {
        for (var id in requests) respond(requests[id], "cancelled")
        handler.cancelAll()
    }

    function nextPrompt() {
        if (automatic || pageStack.busy) return
        if (prompt) {
            if ((!current(promptRequest) || !requests[String(promptRequest.id)])
                    && prompt.status === PageStatus.Active) prompt.reject()
            return
        }
        while (promptQueue.length) {
            var request = promptQueue.shift()
            if (!current(request) || !requests[String(request.id)]) {
                respond(request, "cancelled")
                continue
            }
            promptRequest = request
            prompt = pageStack.animatorPush(promptComponent, {"site": request.host || "", "target": request.url, "canRemember": request.canRemember === true})
            prompt.pageCompleted.connect(function(dialog) {
                prompt = dialog
                dialog.accepted.connect(function() {
                    if (current(request) && requests[String(request.id)]) {
                        requests[String(request.id)].remember = dialog.remember
                        handler.open(String(request.id))
                    }
                    else respond(request, "cancelled")
                })
                dialog.rejected.connect(function() { reject(request) })
                dialog.statusChanged.connect(function() {
                    if (dialog.status === PageStatus.Inactive) {
                        prompt = null
                        promptRequest = null
                        nextPrompt()
                    }
                })
            })
            return
        }
    }

    ExternalUrlHandler {
        id: handler

        onChecked: {
            if (!requests[String(request.id)]) return
            if (!current(request)) { respond(request, "cancelled"); return }
            requests[String(request.id)] = request
            if (!querySucceeded || !supported) {
                if (request.fallback) respond(request, "fallback")
                else {
                    respond(request, "unsupported")
                    pageStack.push(unsupportedComponent)
                }
            } else if (automatic || request.permissionAllowed) handler.open(String(request.id))
            else { promptQueue.push(request); nextPrompt() }
        }
        onDispatched: {
            if (!current(request)) { respond(request, "cancelled"); return }
            respond(request, delivered ? "dispatched" : "unsupported")
            if (automatic && delivered) {
                closeFlow(request)
            }
        }
    }
    Connections {
        target: contentItem
        onRecvAsyncMessageFromTab: {
            if (message === "embed:externalurl") {
                for (var id in requests) {
                    if (String(requests[id].tabId) === String(data.tabId)) respond(requests[id], "cancelled")
                }
                requests[String(data.id)] = data
                data.expiresAt = Date.now() + 110000
                expiryTimer.start()
                handler.check(data)
            }
        }
    }
    Connections {
        target: contentItem ? contentItem.tabModel : null
        onRevisionChanged: {
            for (var id in requests) {
                var request = requests[id]
                if (!current(request)) respond(request, "cancelled")
            }
            if (prompt && !current(promptRequest) && !pageStack.busy && prompt.status === PageStatus.Active) prompt.reject()
        }
    }
    Connections {
        target: flow.pageStack
        onBusyChanged: nextPrompt()
    }
    Timer {
        id: expiryTimer

        interval: 1000
        repeat: true
        onTriggered: {
            for (var id in requests) {
                if (Date.now() >= requests[id].expiresAt) respond(requests[id], "cancelled")
            }
            if (prompt && !requests[String(promptRequest.id)] && !pageStack.busy
                    && prompt.status === PageStatus.Active) prompt.reject()
        }
    }
    Component {
        id: promptComponent

        Dialog {
            property string site
            property string target
            property bool canRemember
            property alias remember: rememberPermission.checked

            SilicaFlickable {
                anchors.fill: parent
                contentHeight: column.height
                Column {
                    id: column

                    width: parent.width
                    DialogHeader {
                        //% "Open external application?"
                        title: qsTrId("sailfish_browser-he-external_application")
                        //% "Open"
                        acceptText: qsTrId("sailfish_browser-he-open_external_application")
                    }
                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * x
                        wrapMode: Text.Wrap
                        //% "%1 wants to open %2 in another application."
                        text: qsTrId("sailfish_browser-la-open_external_application").arg(site).arg(target)
                    }
                    TextSwitch {
                        id: rememberPermission

                        visible: canRemember
                        //% "Always allow this site to open this application"
                        text: qsTrId("sailfish_browser-la-remember_external_application")
                    }
                }
            }
        }
    }
    Component {
        id: unsupportedComponent

        Popups.AlertDialog {
            //% "Cannot open link"
            title: qsTrId("sailfish_browser-he-unsupported_link")
            //% "No application is available to open this link."
            text: qsTrId("sailfish_browser-la-unsupported_link")
        }
    }
    Component.onCompleted: contentItem.addMessageListener("embed:externalurl")
    Component.onDestruction: handler.cancelAll()
}
