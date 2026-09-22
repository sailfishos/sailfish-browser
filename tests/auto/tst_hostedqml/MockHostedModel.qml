/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

import QtQuick 2.6

QtObject {
    property bool loaded: true

    signal runtimeNewTabRequested(string url, string persistentId, bool fromExternal)
    signal runtimeTabActivationRequested(string persistentId, bool reload)
    signal runtimeTabCloseRequested(string persistentId)
    signal runtimeTabNavigationRequested(string persistentId, string url, bool fromExternal)
    signal runtimeTabsClearRequested
    signal runtimeTabAdopted(string runtimeId, string persistentId)
    signal runtimeTabReservationRejected(string persistentId)

    function runtimeRestoreBatch() {
        return { "tabs": [], "selectedIndex": -1 }
    }

    function runtimeIdForPersistentId(persistentId) {
        return persistentId === "17" ? "23" : ""
    }

    function takePendingRuntimeNewTabs() {
        return []
    }

    function applyRuntimeSnapshot(snapshot, selectedTabId) {
    }

    function cancelRuntimeTabReservation(persistentId) {
    }
}
