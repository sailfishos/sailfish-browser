/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

import QtQuick 2.6

Item {
    id: root

    property string failure

    MockHostedModel { id: normalModel }
    MockHostedModel { id: privateModel }
    MockHostedView { id: normalView; name: "normal" }
    MockHostedView { id: privateView; name: "private" }

    HostedTabSession {
        id: normalSession

        model: normalModel
        view: normalView
    }

    HostedTabSession {
        id: privateSession

        model: privateModel
        view: privateView
        privateMode: true
    }

    function expect(condition, message) {
        if (!condition && !failure.length) failure = message
    }

    function runTest() {
        normalSession.dispatchRuntimeCommand({
            "type": "navigate",
            "persistentId": "17",
            "url": "normal"
        })
        privateSession.dispatchRuntimeCommand({
            "type": "navigate",
            "persistentId": "17",
            "url": "private"
        })
        expect(normalView.calls.length === 0, "Normal command was dispatched before restore")
        expect(privateView.calls.length === 0, "Private command was dispatched before restore")

        privateSession.restoreRuntimeTabs(privateView)
        privateSession.applyRuntimeSnapshot(false, privateView)
        expect(privateView.calls.join("|") === "restore:private|load:private",
               "Private session did not restore and drain its own command")
        expect(normalView.calls.length === 0, "Private restore drained the normal session")

        normalSession.restoreRuntimeTabs(normalView)
        normalSession.applyRuntimeSnapshot(false, normalView)
        expect(normalView.calls.join("|") === "restore:normal|load:normal",
               "Normal session did not restore and drain its own command")

        privateSession.dispatchRuntimeCommand({
            "type": "activate",
            "persistentId": "17",
            "reload": true
        })
        expect(privateView.calls.slice(-2).join("|") === "select:23|reload",
               "Private activation did not stay in its renderer")
        expect(normalView.calls.length === 2, "Private activation reached the normal renderer")
        return failure
    }
}
