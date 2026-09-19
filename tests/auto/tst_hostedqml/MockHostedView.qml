/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

import QtQuick 2.6

QtObject {
    id: mockView

    property string name
    property string selectedTabId: "23"
    property var calls: []
    property QtObject tabModel: QtObject {
        property string revision: "1"

        function snapshot() {
            return [{
                "tabId": "23",
                "persistentId": "17",
                "location": "https://example.com/",
                "locationRevision": "1",
                "title": "Example"
            }]
        }
    }

    function restoreTabs(tabs, selectedIndex) {
        calls.push("restore:" + name)
        return true
    }

    function load(url, fromExternal) {
        calls.push("load:" + url)
    }

    function selectTab(runtimeId) {
        calls.push("select:" + runtimeId)
        selectedTabId = runtimeId
        return true
    }

    function reload() {
        calls.push("reload")
    }

    function newTab(url, persistentId, fromExternal, background) {
        calls.push("new:" + url + ":" + persistentId)
        return true
    }

    function closeTab(runtimeId) {
        calls.push("close:" + runtimeId)
        return true
    }

    function associateTab(runtimeId, persistentId) {
        calls.push("associate:" + runtimeId + ":" + persistentId)
    }
}
