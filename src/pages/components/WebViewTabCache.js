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
var count

// Private
var _webViewComponent
var _arguments
var _parent

var _activeWebView
var _activeTabs = {}

var debug = false

function init(args, component, container) {
    _arguments = args
    _webViewComponent = component
    _parent = container
    initialized = true
    count = 0
}

function getTab(tabId, parentId) {
    if (!_webViewComponent || (_activeWebView && _activeWebView.tabId === tabId)) {
        _activeWebView.resumeView()
        _activeWebView.visible = true
        return { "view": _activeWebView, "activated": false }
    }

    if (debug) console.log("WebViewTabCache::getTab about to create a new tab or activate old: ", tabId)

    var tab = _activeTabs[tabId]
    var resurrect = tab && !tab.view
    if (!tab || tab && !tab.view){
        _arguments.parentId = parentId
        _arguments.tabId = tabId
        var webView = _webViewComponent.createObject(_parent, _arguments)
        if (debug) console.log("New view id: ", webView.uniqueID(), parentId, "tab id:", tabId)
        if (!tab) {
            tab = { "view": webView, "cssContentRect": null }
        } else {
            tab.view = webView
        }
        _activeTabs[tabId] = tab
        ++count
    }

    _updateActiveTab(tab, resurrect)

    if (debug) _dumpTabs()

    return { "view": tab.view, "activated": true }
}

function releaseTab(tabId, virtualize) {
    var tab = _activeTabs[tabId]
    if (debug) {
        console.log("--- releaseTab: ", tabId, (tab ? tab.view : null))
        _dumpTabs()
    }
    if (tab) {
        tab.view.destroy()
        if (virtualize) {
            _activeTabs[tabId].view = null
        } else {
            delete _activeTabs[tabId]
        }

        --count
        if (count === 0 || _activeWebView && _activeWebView.tabId === tabId) {
            _activeWebView = null
        }
    }

    if (debug) {
        console.log("--- releaseTab end --- ")
        _dumpTabs()
    }
}

function parentTabId(tabId) {
    var tab = _activeTabs[tabId]
    if (tab && tab.view) {
        var parentId = tab.view.parentId
        for (var i in _activeTabs) {
            var parentTab = _activeTabs[i]
            if (parentTab.view && parentTab.view.uniqueID() == parentId) {
                return parentTab.view.tabId
            }
        }
    }
    return null
}


function _updateActiveTab(tab, resurrect) {
    if (_activeWebView) {
        _activeTabs[_activeWebView.tabId].cssContentRect = {
            "x": _activeWebView.contentRect.x, "y": _activeWebView.contentRect.y,
            "width": _activeWebView.contentRect.width, "height": _activeWebView.contentRect.height
        }
        _activeWebView.visible = false

        // Allow subpending only current active is not creator (parent).
        if (tab.view.parentId !== _activeWebView.uniqueID()) {
            if (_activeWebView.loading) {
                _activeWebView.stop()
            }
            _activeWebView.suspendView()
        }
    }
    _activeWebView = tab.view
    if (resurrect) {
        _activeWebView.resurrectedContentRect = Qt.rect(tab.cssContentRect.x, tab.cssContentRect.y,
                                                        tab.cssContentRect.width, tab.cssContentRect.height)
        _activeTabs[_activeWebView.tabId].cssContentRect = null
    }

    _activeWebView.resumeView()
    _activeWebView.visible = true
}

function _dumpTabs() {
    console.log("---- dump tabs from function:", arguments.callee.caller.name, " --------")
    console.trace()
    for (var i in _activeTabs) {
        console.log("tabId: ", i, " view: ", _activeTabs[i].view,
                    "title: ", (_activeTabs[i].view ? _activeTabs[i].view.title : "VIEW NOT ALIVE!"),
                    "cssContentRect:", _activeTabs[i].cssContentRect)
    }
    console.log("---- dump tabs end -----------------------------------------------------")
}
