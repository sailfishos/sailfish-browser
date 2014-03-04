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

var initialized = false

// Private
var _webViewComponent
var _arguments
var _parent
var _tabCount

var _activeWebView
var _activeTabs = {}

function init(args, component, container) {
    _arguments = args
    _webViewComponent = component
    _parent = container
    initialized = true
    _tabCount = 0
}

function getView(tabId) {
    if (!_webViewComponent || (_activeWebView && _activeWebView.tabId === tabId)) {
        _activeWebView.resumeView()
        _activeWebView.visible = true
        return { "view": _activeWebView, "activated": false }
    }

    console.log("WebViewTabCache::getView about to create a new tab or activate old")

    var webView = _activeTabs[tabId]
    if (!webView){
        webView = _webViewComponent.createObject(_parent, _arguments)
        _activeTabs[tabId] = webView
        ++_tabCount
    }

    webView.tabId = tabId
    _updateActiveView(webView)
    _dumpTabs()
    return { "view": webView, "activated": true }
}

function releaseView(tabId) {
    var viewToRelease = _activeTabs[tabId]
    if (viewToRelease) {
        delete _activeTabs[tabId]
        viewToRelease.destroy()
        --_tabCount
        if (_tabCount === 0 || _activeWebView && _activeWebView.tabId === tabId) {
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
}

function _dumpTabs() {
    console.log("---- dump tabs from function:", arguments.callee.caller.name, " --------")
    console.trace()
    for (var i in _activeTabs) {
        console.log("tabId: ", i, " view: ", _activeTabs[i])
    }
    console.log("---- dump tabs end -----------------------------------------------------")
}
