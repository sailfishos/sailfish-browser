/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

.pragma library
.import "WebPopupHandler.js" as PopupHandler
.import "WebPromptHandler.js" as PromptHandler

var initialized = false

// Private
var _webViewComponent
var _arguments
var _parent
var _promptHandler
var _popupHandler
var _tabCount

var _activeWebView
var _tabs = []
var _activeTabs = {}

function init(args, component, container) {
    _arguments = args
    _webViewComponent = component
    _parent = container
    initialized = true
    _tabs.push(_webViewComponent.createObject(_parent, _arguments))
    _tabCount = _tabs.length
}

function getView(tabId) {
    if (!_webViewComponent || (_activeWebView && _activeWebView.tabId === tabId)) {
        return _activeWebView
    }

    var webView = _activeTabs[tabId]
    if (!webView && _tabs.length > 0) {
        webView = _tabs.shift()
        _activeTabs[tabId] = webView
    } else if (!webView){
        webView = _webViewComponent.createObject(_parent, _arguments)
        _activeTabs[tabId] = webView
        ++_tabCount
    }

    webView.tabId = tabId
    _updateActiveView(webView)
    return webView
}

function releaseView(tabId) {
    var viewToRelease = _activeTabs[tabId]
    if (viewToRelease) {
        delete _activeTabs[tabId]
        viewToRelease.destroy()
        --_tabCount
        if (_tabCount === 0) {
            _activeWebView = null
        }
    }
}

function _updateActiveView(webView) {
    if (_activeWebView) {
        _activeWebView.visible = false
        if (_activeWebView.loading) {
            _activeWebView.stop()
        }
        _activeWebView.suspendView()
    }
    _activeWebView = webView
    _activeWebView.resumeView()
    _activeWebView.visible = true
    PopupHandler.activeWebView = _activeWebView
    PromptHandler.activeWebView = _activeWebView
}
