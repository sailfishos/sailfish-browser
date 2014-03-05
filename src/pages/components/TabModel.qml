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
    property Component webViewComponent
    property Item webViewContainer

    readonly property bool hasNewTabData: _newTabData && _newTabData.url
    readonly property string newTabUrl: hasNewTabData ? _newTabData.url : ""
    readonly property string newTabTitle: hasNewTabData ? _newTabData.title : ""
    readonly property Item newTabPreviousView: hasNewTabData ? _newTabData.previousView : null

    // Load goes so that we first use engine load to resolve url
    // and then save that resolved url to the history. This way
    // urls that resolve to download urls won't get saved to the
    // history (as those won't trigger url change). By doing this way
    // a redirected url will be saved with the redirected url not with
    // the input url.
    property var _newTabData: null

    function newTab(url, title) {
        newTabData(url, title, webViewContainer.contentItem)
        webViewContainer.load(url, title)
    }

    function newTabData(url, title, contentItem) {
        // Url is not need in model._newTabData as we let engine to resolve
        // the url and use the resolved url.
        _newTabData = { "url": url, "title": title, "previousView": contentItem }
    }

    function resetNewTabData() {
        _newTabData = null
    }

    // TODO: Check could this be merged with activateTab(tabId)
    // TabModel could keep also TabCache internally.
    function activateView(tabId, force) {
        if (!TabCache.initialized) {
            TabCache.init({"tab": currentTab, "container": webViewContainer},
                          webViewComponent, webViewContainer)
        }

        if ((loaded || force) && tabId > 0) {
            var activationObject = TabCache.getView(tabId)
            webViewContainer.contentItem = activationObject.view
            return activationObject.activated
        }
        return false
    }

    function releaseView(tabId) {
        TabCache.releaseView(tabId)
        if (count == 0) {
            webViewContainer.contentItem = null
        }
        resetNewTabData()
    }

    // arguments of the signal handler: int tabId
    onActiveTabChanged: {
        if (hasNewTabData && loaded) {
            webViewContainer.currentTabChanged()
            return
        }

        // When all tabs are closed tabId is invalid and view will not get activated.
        if (activateView(tabId, true) && currentTab.valid && webViewContainer._readyToLoad
                && (webViewContainer.contentItem.tabId !== tabId || currentTab.url != webViewContainer.contentItem.url)) {
            webViewContainer.load(currentTab.url, currentTab.title)
        }

        webViewContainer.currentTabChanged()
    }

    // arguments of the signal handler: int tabId
    onTabClosed: releaseView(tabId)

    onLoadedChanged: {
        // Load placeholder for the case where now tabs exist. If a tab exists,
        // it gets initialized by onActiveTabChanged.
        if (loaded && !webViewContainer.contentItem) {
            activateView(nextTabId, true)
        }
    }
}
