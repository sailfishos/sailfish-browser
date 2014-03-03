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
    property Item contentItem
    property Item webViewContainer

    // Load goes so that we first use engine load to resolve url
    // and then save that resolved url to the history. This way
    // urls that resolve to download urls won't get saved to the
    // history (as those won't trigger url change). By doing this way
    // a redirected url will be saved with the redirected url not with
    // the input url.
    property var _newTabData

    function newTab(url, title) {
        // Url is not need in model._newTabData as we let engine to resolve
        // the url and use the resolved url.
        _newTabData = { "url": url, "title": title, "previousView": webViewContainer.contentItem }
        webViewContainer.load(url, title)
    }

    // TODO: Check could this be merged with activateTab(tabId)
    // TabModel could keep also TabCache internally.
    function activateView(tabId) {
        if (!TabCache.initialized) {
            TabCache.init({"tab": currentTab, "container": webViewContainer},
                          webViewComponent, webViewContainer)
        }

        if (tabId > 0 || !webViewContainer.contentItem) {
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
    }

    // arguments of the signal handler: int tabId
    onActiveTabChanged: {
        if (_newTabData) {
            webViewContainer.currentTabChanged()
            return
        }

        // When all tabs are closed tabId is invalid and view will not get activated.
        if (activateView(tabId) && currentTab.valid && webViewContainer._readyToLoad &&
                (contentItem.tabId !== tabId || currentTab.url != contentItem.url)) {
            webViewContainer.load(currentTab.url, currentTab.title)
        }
        webViewContainer.currentTabChanged()
    }

    // arguments of the signal handler: int tabId
    onTabClosed: {
        releaseView(tabId)
        _newTabData = null
    }
}
