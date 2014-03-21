/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Browser 1.0
import "WebViewTabCache.js" as TabCache

TabModel {
    // TODO: Check could this be merged with activateTab(tabId)
    // TabModel could keep also TabCache internally.
    function activateView(tabId, force) {
        if (!TabCache.initialized) {
            TabCache.init({"container": webView}, webPageComponent, webView)
        }

        if ((loaded || force) && tabId > 0) {
            var activationObject = TabCache.getTab(tabId, _newTabParentId)
            webView.contentItem = activationObject.view
            webView.contentItem.chrome = true
            webView.loadProgress = webView.contentItem.loadProgress
            return activationObject.activated
        }
        return false
    }

    function releaseTab(tabId, virtualize) {
        TabCache.releaseTab(tabId, virtualize)
        if (count == 0) {
            webView.contentItem = null
        }
        resetNewTabData()
    }

    // Returns parent view of the given tabId
    function parentTabId(tabId) {
        return TabCache.parentTabId(tabId)
    }

    function _manageMaxTabCount() {
        if (TabCache.count > 5) {
            releaseTab(lastTabId(), true)
        }
    }

    // arguments of the signal handler: int tabId
    onActiveTabChanged: {
        if (hasNewTabData) {
            webView.currentTabChanged()
            return
        }

        // When all tabs are closed tabId is invalid and view will not get activated.
        if (activateView(tabId, true) && webView.currentTab.valid && webView._readyToLoad
                && (webView.contentItem.tabId !== tabId || webView.currentTab.url != webView.contentItem.url)) {
            webView.load(webView.currentTab.url, webView.currentTab.title)
        }

        webView.currentTabChanged()
        _manageMaxTabCount()
    }

    // arguments of the signal handler: int tabId
    onTabAdded: _manageMaxTabCount()

    // arguments of the signal handler: int tabId
    onTabClosed: releaseTab(tabId)

    onLoadedChanged: {
        // Load placeholder for the case where no tabs exist. If a tab exists,
        // it gets initialized by onActiveTabChanged.
        if (loaded && !webView.contentItem) {
            activateView(nextTabId, true)
        }
    }
}
