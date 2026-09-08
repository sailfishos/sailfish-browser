/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

import QtQuick 2.6

Item {
    id: session

    property var model
    property var view
    property bool privateMode
    signal runtimeTabsClearFinished
    signal snapshotApplied(var snapshot)
    signal historyChanged
    signal empty
    signal locationInvalidated(string persistentId)

    property bool _runtimeRestoreSent
    property bool _runtimeSnapshotInitialized
    property var _runtimeAppliedState: ({})
    property var _pendingRuntimeTitles: ({})
    property var _pendingRuntimeCommands: []
    property var _pendingRuntimeCloseCommands: []
    property var _runtimeCloseInFlight: null
    property bool _runtimeTabsClearInProgress
    property var _pendingRuntimeNavigation: null

    function selectedPersistentId(hostView) {
        var snapshot = hostView.tabModel.snapshot()
        for (var i = 0; i < snapshot.length; ++i) {
            if (String(snapshot[i].tabId) === hostView.selectedTabId) return String(snapshot[i].persistentId)
        }
        return ""
    }

    function restoreRuntimeTabs(hostView) {
        if (!hostView || _runtimeRestoreSent
                || !model.loaded) {
            return
        }

        var restoreBatch = model.runtimeRestoreBatch()
        _runtimeRestoreSent = true
        if (!hostView.restoreTabs(restoreBatch.tabs, restoreBatch.selectedIndex)) {
            _runtimeRestoreSent = false
            console.warn("Failed to restore Gecko tab session")
        }
    }

    function queueRuntimeCommand(command) {
        _pendingRuntimeCommands.push(command)
    }

    function drainRuntimeNewTabs() {
        var pendingTabs = model.takePendingRuntimeNewTabs()
        for (var index = 0; index < pendingTabs.length; ++index) {
            var pendingTab = pendingTabs[index]
            var persistentId = String(pendingTab.persistentId)
            var alreadyQueued = false
            for (var commandIndex = 0;
                 commandIndex < _pendingRuntimeCommands.length;
                 ++commandIndex) {
                var queuedCommand = _pendingRuntimeCommands[commandIndex]
                if (queuedCommand.type === "new"
                        && queuedCommand.persistentId === persistentId) {
                    alreadyQueued = true
                    break
                }
            }
            if (!alreadyQueued) {
                dispatchRuntimeCommand({
                    "type": "new",
                    "url": String(pendingTab.url),
                    "persistentId": persistentId,
                    "fromExternal": !!pendingTab.fromExternal
                })
            }
        }
    }

    function removeQueuedRuntimeNewTab(persistentId) {
        var rejectedPersistentId = String(persistentId)
        var remainingCommands = []
        for (var index = 0; index < _pendingRuntimeCommands.length; ++index) {
            var command = _pendingRuntimeCommands[index]
            if (command.type !== "new"
                    || String(command.persistentId) !== rejectedPersistentId) {
                remainingCommands.push(command)
            }
        }
        _pendingRuntimeCommands = remainingCommands
    }

    function cancelPendingRuntimeNavigation() {
        _pendingRuntimeNavigation = null
        runtimeNavigationTimer.stop()
    }

    function queueRuntimeClose(persistentId) {
        var id = String(persistentId)
        if (!id.length || (_runtimeCloseInFlight
                           && _runtimeCloseInFlight.persistentId === id)) {
            return
        }
        for (var index = 0; index < _pendingRuntimeCloseCommands.length; ++index) {
            if (_pendingRuntimeCloseCommands[index] === id) {
                return
            }
        }
        _pendingRuntimeCloseCommands.push(id)
    }

    function finishRuntimeTabsClear() {
        if (_runtimeTabsClearInProgress) {
            _runtimeTabsClearInProgress = false
            runtimeTabsClearFinished()
        }
    }

    function startNextRuntimeClose(runtimeHostView) {
        var hostView = runtimeHostView || view
        if (!hostView || _runtimeCloseInFlight) {
            return
        }
        if (!_pendingRuntimeCloseCommands.length) {
            finishRuntimeTabsClear()
            return
        }

        var persistentId = _pendingRuntimeCloseCommands.shift()
        var runtimeId = model.runtimeIdForPersistentId(persistentId)
        if (!runtimeId.length) {
            startNextRuntimeClose(hostView)
            return
        }

        cancelPendingRuntimeNavigation()
        _runtimeCloseInFlight = {
            "persistentId": persistentId,
            "runtimeId": runtimeId,
            "revision": String(hostView.tabModel.revision)
        }
        if (!hostView.closeTab(runtimeId)) {
            _runtimeCloseInFlight = null
            _pendingRuntimeCloseCommands = []
            finishRuntimeTabsClear()
        }
    }

    function resolveRuntimeCloseAfterSnapshot(runtimeHostView) {
        var hostView = runtimeHostView || view
        if (!hostView || !_runtimeCloseInFlight
                || _runtimeCloseInFlight.revision === String(hostView.tabModel.revision)) {
            return
        }

        var close = _runtimeCloseInFlight
        var runtimeTab = hostedRuntimeTabByRuntimeId(hostView, close.runtimeId)
        if (runtimeTab) {
            // Snapshots can precede close processing or report the pending
            // PermitUnload state. A rejected close has its own result signal.
            return
        }
        _runtimeCloseInFlight = null
        startNextRuntimeClose(hostView)
    }

    function runtimeTabCloseResult(runtimeId, closed) {
        if (!_runtimeCloseInFlight
                || String(_runtimeCloseInFlight.runtimeId) !== String(runtimeId)) {
            return
        }

        if (!closed) {
            // A beforeunload prompt was declined. Do not issue the next
            // close-all request until a later user action starts a new batch.
            _runtimeCloseInFlight = null
            _pendingRuntimeCloseCommands = []
            finishRuntimeTabsClear()
        }
        // A successful close is still committed only by the next complete
        // runtime snapshot, which retains persistence as the authority.
    }

    function dispatchRuntimeCommand(command, runtimeHostView) {
        var hostView = runtimeHostView || view
        if (!hostView || !_runtimeRestoreSent || !_runtimeSnapshotInitialized) {
            queueRuntimeCommand(command)
            return
        }

        if (command.type === "new") {
            cancelPendingRuntimeNavigation()
            if (!hostView.newTab(command.url, command.persistentId,
                                 command.fromExternal, false)) {
                model.cancelRuntimeTabReservation(
                            command.persistentId)
            }
        } else if (command.type === "activate") {
            cancelPendingRuntimeNavigation()
            var runtimeId = model.runtimeIdForPersistentId(
                        command.persistentId)
            if (runtimeId.length && hostView.selectTab(runtimeId) && command.reload) {
                hostView.reload()
            }
        } else if (command.type === "close") {
            queueRuntimeClose(command.persistentId)
            startNextRuntimeClose(hostView)
        } else if (command.type === "navigate") {
            cancelPendingRuntimeNavigation()
            var navigateRuntimeId = model.runtimeIdForPersistentId(
                        command.persistentId)
            if (navigateRuntimeId.length) {
                if (hostView.selectedTabId === navigateRuntimeId) {
                    hostView.load(command.url, command.fromExternal)
                } else if (hostView.selectTab(navigateRuntimeId)) {
                    _pendingRuntimeNavigation = command
                    runtimeNavigationTimer.restart()
                } else {
                    cancelPendingRuntimeNavigation()
                }
            }
        } else if (command.type === "clear") {
            _runtimeTabsClearInProgress = true
            var tabs = hostView.tabModel.snapshot()
            for (var index = tabs.length - 1; index >= 0; --index) {
                queueRuntimeClose(String(tabs[index].persistentId))
            }
            startNextRuntimeClose(hostView)
        }
    }

    function flushSelectedRuntimeNavigation(runtimeHostView) {
        var hostView = runtimeHostView || view
        var persistentId = selectedPersistentId(hostView)
        var command = _pendingRuntimeNavigation
        if (!hostView || !command || !persistentId.length) {
            return
        }

        if (persistentId === command.persistentId) {
            cancelPendingRuntimeNavigation()
            hostView.load(command.url, command.fromExternal)
            return
        }

        if (!model.runtimeIdForPersistentId(
                    command.persistentId).length) {
            cancelPendingRuntimeNavigation()
        }
    }

    function flushRuntimeCommands(runtimeHostView) {
        var hostView = runtimeHostView || view
        if (!_runtimeSnapshotInitialized || !hostView) {
            return
        }
        var commands = _pendingRuntimeCommands
        _pendingRuntimeCommands = []
        for (var index = 0; index < commands.length; ++index) {
            dispatchRuntimeCommand(commands[index], hostView)
        }
    }

    function hasPendingRuntimeTitles() {
        for (var runtimeId in _pendingRuntimeTitles) {
            return true
        }
        return false
    }

    function pairedRuntimeSnapshot(snapshot, acceptDeferredTitles) {
        var pairedSnapshot = []
        var nextPendingTitles = {}
        var nextState = {}
        for (var index = 0; index < snapshot.length; ++index) {
            var tab = snapshot[index]
            var runtimeId = String(tab.tabId)
            var revision = String(tab.locationRevision)
            var location = String(tab.location)
            var runtimeTitle = String(tab.title)
            var previous = _runtimeAppliedState[runtimeId]
            var pending = _pendingRuntimeTitles[runtimeId]
            var locationChanged = previous
                    && (previous.revision !== revision
                        || previous.location !== location)
            var pairedTitle = runtimeTitle

            if (locationChanged) {
                // A grab can complete after Gecko has committed a new
                // location. Its persistent-id/revision guard will reject the
                // result too; invalidate it here so it is not written at all.
                if (!privateMode) locationInvalidated(String(tab.persistentId))
            }

            if (_runtimeSnapshotInitialized) {
                var pendingMatches = pending
                        && pending.revision === revision
                        && pending.location === location
                        && pending.title === runtimeTitle
                if (acceptDeferredTitles && pendingMatches) {
                    pairedTitle = runtimeTitle
                } else if (locationChanged
                           || (previous && previous.title !== runtimeTitle)) {
                    pairedTitle = locationChanged ? "" : previous.title
                    if (runtimeTitle.length) {
                        nextPendingTitles[runtimeId] = {
                            "revision": revision,
                            "location": location,
                            "title": runtimeTitle
                        }
                    }
                }
            }

            var pairedTab = {}
            for (var key in tab) {
                pairedTab[key] = tab[key]
            }
            pairedTab.title = pairedTitle
            pairedSnapshot.push(pairedTab)
            nextState[runtimeId] = {
                "revision": revision,
                "location": location,
                "title": pairedTitle
            }
        }
        _pendingRuntimeTitles = nextPendingTitles
        _runtimeAppliedState = nextState
        if (hasPendingRuntimeTitles()) {
            runtimeTitlePairingTimer.restart()
        } else {
            runtimeTitlePairingTimer.stop()
        }
        return pairedSnapshot
    }

    function runtimeSnapshotChanged(snapshot) {
        if (!_runtimeSnapshotInitialized) {
            return false
        }
        for (var index = 0; index < snapshot.length; ++index) {
            var tab = snapshot[index]
            var previous = _runtimeAppliedState[String(tab.tabId)]
            if (!previous
                    || previous.revision !== String(tab.locationRevision)
                    || previous.location !== String(tab.location)
                    || previous.title !== String(tab.title)) {
                return true
            }
        }
        var appliedCount = 0
        for (var runtimeId in _runtimeAppliedState) {
            ++appliedCount
        }
        return appliedCount !== snapshot.length
    }

    function applyRuntimeSnapshot(acceptDeferredTitles, runtimeHostView) {
        var hostView = runtimeHostView || view
        if (!hostView || !_runtimeRestoreSent || !hostView.tabModel.revision.length) {
            return
        }

        var runtimeSnapshot = hostView.tabModel.snapshot()
        var historyChanged = runtimeSnapshotChanged(runtimeSnapshot)
        var snapshot = pairedRuntimeSnapshot(runtimeSnapshot,
                                             !!acceptDeferredTitles)
        // Recover requests queued before the Connections object existed.
        // While the first snapshot is still initializing, dispatching here
        // appends them to the command queue. Applying the snapshot can then
        // reject an expired reservation before that queue is flushed.
        drainRuntimeNewTabs()
        model.applyRuntimeSnapshot(
                    snapshot, hostView.selectedTabId)
        if (historyChanged && !privateMode) {
            // PersistentTabModel is the sole writer. Queueing a search after
            // its snapshot update refreshes the live History UI without
            // incrementing the visit count a second time.
            session.historyChanged()
        }
        _runtimeSnapshotInitialized = true
        resolveRuntimeCloseAfterSnapshot(hostView)
        flushRuntimeCommands(hostView)
        flushSelectedRuntimeNavigation(hostView)

        snapshotApplied(runtimeSnapshot)
        if (snapshot.length === 0) empty()
    }

    Timer {
        id: runtimeTitlePairingTimer

        interval: 120
        onTriggered: session.applyRuntimeSnapshot(true)
    }

    Timer {
        id: runtimeNavigationTimer

        interval: 1000
        onTriggered: session.cancelPendingRuntimeNavigation()
    }

    Timer {
        id: runtimeNewTabDrainTimer

        interval: 0
        onTriggered: session.drainRuntimeNewTabs()
    }

    Connections {
        target: model
        // The persistent model records the recoverable command in another
        // handler for this signal. Drain on the next event-loop turn so that
        // state is visible regardless of connection ordering.
        onRuntimeNewTabRequested: runtimeNewTabDrainTimer.restart()
        onRuntimeTabActivationRequested: session.dispatchRuntimeCommand({
            "type": "activate",
            "persistentId": persistentId,
            "reload": reload
        })
        onRuntimeTabCloseRequested: session.dispatchRuntimeCommand({
            "type": "close",
            "persistentId": persistentId
        })
        onRuntimeTabNavigationRequested: session.dispatchRuntimeCommand({
            "type": "navigate",
            "persistentId": persistentId,
            "url": url,
            "fromExternal": fromExternal
        })
        onRuntimeTabsClearRequested: session.dispatchRuntimeCommand({
            "type": "clear"
        })
        onRuntimeTabAdopted: {
            if (view) {
                view.associateTab(runtimeId, persistentId)
            }
        }
        onRuntimeTabReservationRejected: {
            session.removeQueuedRuntimeNewTab(persistentId)
            if (session._pendingRuntimeNavigation
                    && session._pendingRuntimeNavigation.persistentId
                    === persistentId) {
                session.cancelPendingRuntimeNavigation()
            }
        }
    }
}
