/****************************************************************************
**
** Copyright (c) 2013 - 2021 Jolla Ltd.
** Copyright (c) 2019 - 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import QtQuick.Window 2.2 as QuickWindow
import Qt5Mozilla 1.0
import Sailfish.Silica 1.0
import Sailfish.Silica.private 1.0 as Private
import Sailfish.Browser 1.0
import Sailfish.Policy 1.0
import Sailfish.WebEngine 1.0
import Sailfish.WebView.Pickers 1.0 as Pickers
import Sailfish.WebView.Popups 1.0 as Popups
import Nemo.Configuration 1.0
import "components" as Browser
import "../shared" as Shared

Page {
    id: browserPage

    signal hostedThumbnailUpdated(string persistentId, string location,
                                  string locationRevision, string fileName)
    signal hostedThumbnailGrabbed(string persistentId, string location,
                                  string locationRevision, real generation)
    signal runtimeTabsClearFinished

    readonly property bool active: status == PageStatus.Active
    property bool tabPageActive
    readonly property size thumbnailSize: Qt.size(width - Theme.horizontalPageMargin * 2,
                                                  Math.max(height / 2.5, width / 1.66)
                                                  - (Theme.iconSizeSmall + Theme.paddingMedium * 2))
    property Item debug
    property Component tabPageComponent

    property alias overlay: overlay
    property alias tabs: webView.tabModel
    property alias history: historyModel
    readonly property var chromeHostView: _runtimeChromeView
    readonly property var _runtimeChromeView: webView.privateMode
                                               ? privateChromeHostLoader.item : chromeHostLoader.item
    readonly property bool viewLoading: chromeHostView ? chromeHostView.loading : webView.loading
    readonly property int loadProgress: chromeHostView ? chromeHostView.loadProgress : webView.loadProgress
    readonly property string url: chromeHostView ? String(chromeHostView.url) : webView.url
    readonly property string title: chromeHostView ? (_hostedMetadataTitle || chromeHostView.title) : webView.title
    readonly property var security: chromeHostView ? chromeHostView.security : webView.security
    readonly property bool contentFullscreen: chromeHostView ? chromeHostView.fullscreen
                                                        : webView.contentFullscreen
    readonly property int _hostedCutoutTop: webView._hostBaseCutoutTop
    readonly property int _hostedCutoutRight: webView._hostBaseCutoutRight
    readonly property int _hostedCutoutBottom: webView._hostBaseCutoutBottom
    readonly property int _hostedCutoutLeft: webView._hostBaseCutoutLeft
    readonly property int _hostedCutoutInsetUsage: webView._safeAreaInsetUsage(
            _hostedCutoutTop, _hostedCutoutRight, _hostedCutoutBottom, _hostedCutoutLeft)
    readonly property bool hostedDisplayCutoutAllowed: contentFullscreen
            || (_hostedViewportFit === "cover"
                && (webView.cutoutGuardConfig.value === "strict"
                    || (webView.cutoutGuardConfig.value === "top_guard"
                        && (_hostedSafeAreaInsetUsage & _hostedCutoutInsetUsage)
                            === _hostedCutoutInsetUsage)))
    property string _hostedMetadataTitle
    property string _hostedFavicon
    property bool _hostedAcceptedTouchIcon
    property string _hostedViewportFit
    property int _hostedSafeAreaInsetUsage
    property string _hostedThemeColor
    property var _hostedViewportFitStates: ({})
    readonly property color _hostedSurfaceColor: (hostedDisplayCutoutAllowed
            || !_hostedThemeColor.length) ? webView._defaultThemeColor : _hostedThemeColor
    property var _hostedTabCrashStates: ({})
    readonly property bool hostedTabCrashed: selectedHostedTabCrashState() !== null
    property int _hostedExpectedOrientation
    property real _hostedOrientationFrameBaseline
    property bool _hostedOrientationAcknowledged
    property Item _hostedTextSelectionController
    property string _hostedSelectionTabId
    property var _pendingHostedClipboardPaste
    property var _pendingHostedModalRequests: []
    property var _activeHostedModalTarget
    property var _hostedMessageTargets: []
    // recvAsyncMessageFromTab() and recvAsyncMessage() both report selected
    // tab messages.  Buffer generic delivery for one event turn, so either
    // Qt signal ordering reaches the feature handler exactly once.
    property var _pendingHostedGenericMessages: []
    property var _hostedTabAsyncMessages: []
    property alias webView: webView
    property alias inputRegion: inputRegion

    onChromeHostViewChanged: {
        clearHostedSelection()
        finishHostedOrientationWait()
        _hostedMetadataTitle = ""
        _hostedFavicon = ""
        _hostedAcceptedTouchIcon = false
        if (chromeHostView) {
            webView.reportWindowOrientation(
                        webView._qtScreenOrientation(browserPage.orientation))
            syncHostedContainerState(chromeHostView)
            syncHostedDesktopMode(chromeHostView)
            applyHostedViewportFitState(chromeHostView)
            updateHostedViewSuspension(chromeHostView)
            resetHostedThumbnailCapture(chromeHostView)
            requestHostedThumbnail()
        }
    }

    function runtimeSession(hostView) {
        var view = hostView || _runtimeChromeView
        return view ? view.hostedSession : null
    }

    function sendPageMessage(name, data) {
        if (chromeHostView) {
            chromeHostView.sendAsyncMessage(name, data)
            return true
        }

        return false
    }

    function syncHostedContainerState(hostView, notifySecurity) {
        var view = hostView || _runtimeChromeView
        if (!view || view !== chromeHostView) {
            return
        }

        webView.updateHostedState(String(view.url),
                                  _hostedMetadataTitle || view.title,
                                  !!view.loading, view.loadProgress,
                                  !!view.canGoBack, !!view.canGoForward,
                                  view.security, !!notifySecurity)
    }

    function beginHostedOrientationWait(hostView, orientation) {
        if (!hostView || !hostView.active) {
            finishHostedOrientationWait()
            return
        }

        _hostedExpectedOrientation = webView._qtScreenOrientation(orientation)
        _hostedOrientationFrameBaseline = hostView.platformFrameGeneration
        _hostedOrientationAcknowledged = false
        orientationFader.waitForWebContentOrientationChanged = true
    }

    function acknowledgeHostedOrientation(hostView, data) {
        if (!orientationFader.waitForWebContentOrientationChanged
                || !hostView || hostView !== chromeHostView
                || !data || !data.painted) {
            return
        }

        var width = Number(data.width)
        var height = Number(data.height)
        if (!(width > 0) || !(height > 0) || width === height) {
            return
        }

        var expectedLandscape = _hostedExpectedOrientation === Qt.LandscapeOrientation
                || _hostedExpectedOrientation === Qt.InvertedLandscapeOrientation
        if ((width > height) !== expectedLandscape) {
            return
        }

        _hostedOrientationAcknowledged = true
        noteHostedOrientationFrame(hostView)
    }

    function noteHostedOrientationFrame(hostView) {
        if (!orientationFader.waitForWebContentOrientationChanged
                || !_hostedOrientationAcknowledged
                || !hostView || hostView !== chromeHostView
                || hostView.orientation !== _hostedExpectedOrientation
                || hostView.platformFrameGeneration
                   <= _hostedOrientationFrameBaseline) {
            return
        }

        finishHostedOrientationWait()
    }

    function finishHostedOrientationWait() {
        _hostedExpectedOrientation = Qt.PrimaryOrientation
        _hostedOrientationAcknowledged = false
        orientationFader.waitForWebContentOrientationChanged = false
    }

    function sendHostedMessageToTab(hostView, tabId, persistentId, name, data,
                                    resolveBeforeUnload) {
        if (!hostView || !tabId || !String(tabId).length) {
            return false
        }

        var messageData = {}
        if (data) {
            for (var key in data) {
                messageData[key] = data[key]
            }
        }
        messageData.tabId = String(tabId)
        if (persistentId && String(persistentId).length) {
            messageData.persistentId = String(persistentId)
        }

        if (resolveBeforeUnload) {
            // Qt resolves chrome-hosted beforeunload prompts through the
            // selected-view confirmresponse entry point, which uses the
            // requestId/tabId pair rather than frame-message routing.
            hostView.sendAsyncMessage(name, messageData)
            return true
        }
        return hostView.sendAsyncMessageToTab(String(tabId), name, messageData)
    }

    function findInPage(text, backwards, again) {
        sendPageMessage("embedui:find", {
                            "text": text,
                            "backwards": !!backwards,
                            "again": !!again
                        })
    }

    function resetFindInPage() {
        findInPage("", false, false)
        webView.findInPageHasResult = false
    }

    function exitFullscreen() {
        sendPageMessage("embedui:exitFullscreen", {})
    }

    function desktopModeForPersistentId(persistentId) {
        if (!persistentId || !String(persistentId).length) {
            return false
        }
        return webView.tabModel.runtimeDesktopMode(String(persistentId))
    }

    function syncHostedDesktopMode(hostView) {
        var view = hostView || chromeHostView
        if (!view || view !== chromeHostView) {
            return
        }

        view.desktopMode = desktopModeForPersistentId(selectedPersistentId(view))
    }

    function setDesktopMode(desktopMode) {
        desktopMode = !!desktopMode
        if (chromeHostView) {
            var persistentId = selectedPersistentId(chromeHostView)
            if (!persistentId.length || chromeHostView.desktopMode === desktopMode) {
                return
            }
            if (webView.tabModel.setRuntimeDesktopMode(
                        persistentId, desktopMode)) {
                chromeHostView.desktopMode = desktopMode
                chromeHostView.reload()
            }
            return
        }

    }

    function clearHostedSelection() {
        var controller = _hostedTextSelectionController
        _hostedTextSelectionController = null
        _hostedSelectionTabId = ""
        inputRegion.selectionStartHandleMask = Qt.rect(0, 0, 0, 0)
        inputRegion.selectionEndHandleMask = Qt.rect(0, 0, 0, 0)
        if (controller) {
            var target = controller.contentItem
            controller.clearSelection()
            releaseHostedMessageTarget(target)
        } else if (chromeHostView) {
            chromeHostView.sendAsyncMessage("Browser:SelectionClose", {
                                                "clearSelection": true
                                            })
        }
    }

    function clearSelection() { clearHostedSelection() }

    function updateHostedViewSuspension(hostView) {
        var view = hostView || chromeHostView
        if (!view) {
            return
        }

        if (browserPage.active && view === chromeHostView
                && view.visible && webView.foreground) {
            view.resumeView()
        } else {
            view.suspendView()
        }
    }

    function hostedAsyncMessageMatches(first, second) {
        return first && second && first.message === second.message
                && String(first.tabId) === String(second.tabId)
    }

    function noteHostedAsyncMessage(tabId, persistentId, message) {
        var received = {
            "tabId": String(tabId),
            "persistentId": String(persistentId || ""),
            "message": message
        }
        var pending = []
        for (var index = 0; index < _pendingHostedGenericMessages.length; ++index) {
            if (!hostedAsyncMessageMatches(received, _pendingHostedGenericMessages[index])) {
                pending.push(_pendingHostedGenericMessages[index])
            }
        }
        _pendingHostedGenericMessages = pending
        _hostedTabAsyncMessages.push(received)
        hostedGenericDuplicateTimer.restart()
    }

    function queueHostedGenericAsyncMessage(hostView, message, data) {
        var generic = {
            "hostView": hostView,
            "tabId": data && data.tabId !== undefined
                     ? String(data.tabId) : String(hostView.selectedTabId),
            "persistentId": data && data.persistentId !== undefined
                            ? String(data.persistentId) : "",
            "message": message,
            "data": data
        }
        for (var index = 0; index < _hostedTabAsyncMessages.length; ++index) {
            if (hostedAsyncMessageMatches(generic, _hostedTabAsyncMessages[index])) {
                return
            }
        }
        _pendingHostedGenericMessages.push(generic)
        hostedGenericDuplicateTimer.restart()
    }

    function loadInternalPage(url) {
        if (url == "about:config" || url == "about:settings") {
            overlay.loadPage(url)
            return true
        }

        return false
    }

    function load(url, title) {
        if (loadInternalPage(url)) {
            return
        }

        if (chromeHostView) {
            loadHostedContainerRequest(url, false)
        } else {
            webView.load(url, false)
        }
    }

    function loadHostedContainerRequest(url, fromExternal) {
        if (!chromeHostView) {
            return
        }

        if (chromeHostView.selectedTabId.length) {
            chromeHostView.load(url, !!fromExternal)
        } else {
            webView.tabModel.newTab(url, !!fromExternal)
        }
    }

    function newTab(url, fromExternal) {
        return webView.tabModel.newTab(url, !!fromExternal)
    }

    function goBack() {
        if (chromeHostView) {
            // Page-driven traversals can leave the database cursor behind Gecko.
            var persistentId = selectedPersistentId()
            if (persistentId.length) {
                webView.tabModel.runtimeGoBack(persistentId)
            }
            chromeHostView.goBack()
        }
    }

    function goForward() {
        if (chromeHostView) {
            var persistentId = selectedPersistentId()
            if (persistentId.length) {
                webView.tabModel.runtimeGoForward(persistentId)
            }
            chromeHostView.goForward()
        }
    }

    function stop() {
        if (chromeHostView) {
            chromeHostView.stop()
        }
    }

    function reload() {
        if (chromeHostView) {
            chromeHostView.reload()
        }
    }

    function selectedPersistentId(runtimeHostView) {
        var hostView = runtimeHostView || _runtimeChromeView
        if (!hostView || !hostView.selectedTabId.length) {
            return ""
        }
        var snapshot = hostView.tabModel.snapshot()
        for (var index = 0; index < snapshot.length; ++index) {
            if (String(snapshot[index].tabId) === hostView.selectedTabId) {
                return String(snapshot[index].persistentId)
            }
        }
        return ""
    }

    function hostedRuntimeTabByRuntimeId(runtimeHostView, runtimeId) {
        var hostView = runtimeHostView || _runtimeChromeView
        if (!hostView || !runtimeId || !String(runtimeId).length) {
            return null
        }
        var snapshot = hostView.tabModel.snapshot()
        for (var index = 0; index < snapshot.length; ++index) {
            if (String(snapshot[index].tabId) === String(runtimeId)) {
                return snapshot[index]
            }
        }
        return null
    }

    function hostedTabCrashState(hostView, runtimeId) {
        var tab = hostedRuntimeTabByRuntimeId(hostView, runtimeId)
        var state = _hostedTabCrashStates[String(runtimeId)]
        return tab && state
                && state.persistentId === String(tab.persistentId)
                && state.locationRevision === String(tab.locationRevision)
                ? state : null
    }

    function selectedHostedTabCrashState() {
        var hostView = chromeHostView
        return hostView && String(hostView.selectedTabId).length
                ? hostedTabCrashState(hostView, hostView.selectedTabId) : null
    }

    function markHostedTabCrashed(hostView, tabId, persistentId, data) {
        var runtimeId = String(tabId)
        var tab = hostedRuntimeTabByRuntimeId(hostView, runtimeId)
        var locationRevision = data && data.locationRevision !== undefined
                ? String(data.locationRevision) : ""
        if (!tab || (persistentId
                     && String(tab.persistentId) !== String(persistentId))
                || (locationRevision.length
                    && locationRevision !== String(tab.locationRevision))) {
            return
        }

        var states = {}
        for (var key in _hostedTabCrashStates) {
            states[key] = _hostedTabCrashStates[key]
        }
        states[runtimeId] = {
            "persistentId": String(tab.persistentId),
            "locationRevision": String(tab.locationRevision)
        }
        _hostedTabCrashStates = states

        if (hostView === chromeHostView && hostView && String(hostView.selectedTabId) === runtimeId) {
            resetHostedThumbnailCapture(hostView)
            clearHostedSelection()
            overlay.animator.showChrome()
        }
    }

    function clearHostedTabCrashState(runtimeId) {
        runtimeId = String(runtimeId)
        if (!_hostedTabCrashStates[runtimeId]) {
            return
        }

        var states = {}
        for (var key in _hostedTabCrashStates) {
            if (key !== runtimeId) {
                states[key] = _hostedTabCrashStates[key]
            }
        }
        _hostedTabCrashStates = states
    }

    function pruneHostedTabCrashStates(snapshot) {
        var states = {}
        var retainedCount = 0
        var previousCount = 0
        for (var key in _hostedTabCrashStates) {
            ++previousCount
        }
        for (var index = 0; index < snapshot.length; ++index) {
            var tab = snapshot[index]
            var runtimeId = String(tab.tabId)
            var state = _hostedTabCrashStates[runtimeId]
            if (state && state.persistentId === String(tab.persistentId)
                    && state.locationRevision === String(tab.locationRevision)) {
                states[runtimeId] = state
                ++retainedCount
            }
        }
        if (retainedCount !== previousCount) {
            _hostedTabCrashStates = states
        }
    }

    function applyHostedViewportFitState(hostView) {
        _hostedViewportFit = ""
        _hostedSafeAreaInsetUsage = 0
        _hostedThemeColor = ""
        if (!hostView || !String(hostView.selectedTabId).length) {
            return
        }

        var runtimeId = String(hostView.selectedTabId)
        var tab = hostedRuntimeTabByRuntimeId(hostView, runtimeId)
        var state = _hostedViewportFitStates[runtimeId]
        if (!tab || !state
                || state.persistentId !== String(tab.persistentId)
                || state.locationRevision !== String(tab.locationRevision)) {
            return
        }

        _hostedViewportFit = state.viewportFit
        _hostedSafeAreaInsetUsage = state.safeAreaInsetUsage
        _hostedThemeColor = state.themeColor || ""
    }

    function updateHostedViewportFitState(hostView, tabId, persistentId, data) {
        var runtimeId = String(tabId)
        var tab = hostedRuntimeTabByRuntimeId(hostView, runtimeId)
        var eventRevision = data && data.locationRevision !== undefined
                ? String(data.locationRevision) : ""
        if (!tab || (persistentId
                     && String(tab.persistentId) !== String(persistentId))
                || (eventRevision.length
                    && eventRevision !== String(tab.locationRevision))) {
            return
        }

        var states = {}
        for (var key in _hostedViewportFitStates) {
            states[key] = _hostedViewportFitStates[key]
        }
        states[runtimeId] = {
            "persistentId": String(tab.persistentId),
            "locationRevision": String(tab.locationRevision),
            "viewportFit": data.viewportFit || data.value || "",
            "safeAreaInsetUsage": Number(data.safeAreaInsetUsage || 0),
            "themeColor": data.themeColor || ""
        }
        _hostedViewportFitStates = states
        if (hostView === chromeHostView && hostView && String(hostView.selectedTabId) === runtimeId) {
            applyHostedViewportFitState(hostView)
        }
    }

    function hostedMessageTabIsLive(hostView, tabId, persistentId) {
        var tab = hostedRuntimeTabByRuntimeId(hostView, tabId)
        return tab && (!persistentId || String(tab.persistentId) === String(persistentId))
    }

    function hostedMessageTabIsSelected(hostView, tabId, persistentId) {
        return hostView === chromeHostView && hostView
                && String(hostView.selectedTabId) === String(tabId)
                && hostedMessageTabIsLive(hostView, tabId, persistentId)
    }

    function hostedModalRequestIsLive(request) {
        if (!request) {
            return false
        }
        var tab = hostedRuntimeTabByRuntimeId(request.hostView, request.tabId)
        return tab
                && (!request.persistentId
                    || String(tab.persistentId) === request.persistentId)
                && (!request.locationRevision
                    || String(tab.locationRevision) === request.locationRevision)
    }

    function rejectHostedModalRequest(request) {
        if (!request || !request.hostView || !request.data) {
            return
        }

        var data = request.data
        var response
        var responseMessage
        switch (request.message) {
        case "embed:alert":
            responseMessage = "alertresponse"
            response = { "winId": data.winId, "checkvalue": false }
            break
        case "embed:confirm":
            responseMessage = "confirmresponse"
            response = {
                "winId": data.winId,
                "accepted": false,
                "checkvalue": false
            }
            if (data.requestId !== undefined) {
                response.requestId = data.requestId
            }
            break
        case "embed:prompt":
            responseMessage = "promptresponse"
            response = { "winId": data.winId, "accepted": false,
                         "checkvalue": false }
            break
        case "embed:login":
            responseMessage = "embedui:login"
            response = { "buttonidx": 1, "id": data.id }
            break
        case "embed:auth":
            responseMessage = "authresponse"
            response = { "winId": data.winId, "accepted": false }
            break
        case "embed:permissions":
            responseMessage = "embedui:permissions"
            response = { "allow": false, "checkedDontAsk": false,
                         "id": data.id }
            break
        case "embed:webrtcrequest":
            responseMessage = "embedui:webrtcresponse"
            response = { "allow": false, "checkedDontAsk": false,
                         "choices": {}, "id": data.id }
            break
        case "embed:popupblocked":
            if (data.popupId === undefined || data.winId === undefined) {
                return
            }
            responseMessage = "embedui:popupblocked"
            response = { "allow": false, "popupId": data.popupId,
                         "winId": data.winId }
            break
        case "embed:select":
            responseMessage = "selectresponse"
            response = { "winId": data.winId, "button": 1 }
            break
        case "embed:colorpicker":
            responseMessage = "embedui:colorpickerresponse"
            response = { "winId": data.winId, "accepted": false,
                         "color": "" }
            break
        case "embed:filepicker":
            responseMessage = "filepickerresponse"
            response = { "winId": data.winId, "accepted": false,
                         "items": [] }
            break
        case "embed:selectasync":
            responseMessage = "embedui:selectresponse"
            response = { "id": data.id, "result": -1 }
            break
        case "embedui:downloadpicker":
        case "embed:downloadpicker":
            if (data.requestId !== undefined) {
                WebEngine.notifyObservers("embedui:downloadpicker", {
                                              "cancelled": true,
                                              "requestId": data.requestId,
                                              "winId": data.winId,
                                              "tabId": request.tabId,
                                              "persistentId": request.persistentId
                                          })
            }
            return
        default:
            return
        }

        sendHostedMessageToTab(request.hostView, request.tabId,
                               request.persistentId, responseMessage, response,
                               request.message === "embed:confirm"
                               && !!data.inPermitUnload)
    }

    function presentHostedModalRequest(request) {
        if (!request || _activeHostedModalTarget
                || !hostedModalRequestIsLive(request)
                || !hostedMessageTabIsSelected(request.hostView,
                                                request.tabId,
                                                request.persistentId)) {
            return false
        }
        return request.kind === "picker"
                ? presentHostedPicker(request.hostView, request.tabId,
                                      request.persistentId, request.message,
                                      request.data, request)
                : presentHostedPopup(request.hostView, request.tabId,
                                     request.persistentId, request.message,
                                     request.data, request)
    }

    function queueHostedModalRequest(kind, hostView, tabId, persistentId, message, data) {
        var request = {
            "kind": kind,
            "hostView": hostView,
            "tabId": String(tabId),
            "persistentId": String(persistentId || ""),
            // The content bridge can deliver a modal request before the
            // runtime tab snapshot observes the same navigation revision.
            // Tab and persistent ids still bind the request to its source;
            // requiring the snapshot revision here drops valid dialogs.
            "locationRevision": "",
            "message": message,
            "data": data || {}
        }
        if (!hostedModalRequestIsLive(request)) {
            rejectHostedModalRequest(request)
            return true
        }
        if (_activeHostedModalTarget || _pendingHostedModalRequests.length) {
            _pendingHostedModalRequests.push(request)
            return true
        }
        if (hostedMessageTabIsSelected(hostView, request.tabId, request.persistentId)) {
            if (!presentHostedModalRequest(request)) {
                rejectHostedModalRequest(request)
            }
            return true
        }

        _pendingHostedModalRequests.push(request)
        if (!hostView.selectTab(request.tabId)) {
            var pending = []
            for (var index = 0; index < _pendingHostedModalRequests.length; ++index) {
                if (_pendingHostedModalRequests[index] !== request) {
                    pending.push(_pendingHostedModalRequests[index])
                }
            }
            _pendingHostedModalRequests = pending
            rejectHostedModalRequest(request)
            return true
        }
        hostedModalRequestTimer.restart()
        return true
    }

    function processPendingHostedModalRequests(hostView) {
        if (_activeHostedModalTarget) {
            return
        }
        var pending = _pendingHostedModalRequests
        _pendingHostedModalRequests = []
        var remaining = []
        for (var index = 0; index < pending.length; ++index) {
            var request = pending[index]
            if (request.hostView !== hostView) {
                remaining.push(request)
            } else if (!hostedModalRequestIsLive(request)) {
                rejectHostedModalRequest(request)
            } else if (hostedMessageTabIsSelected(hostView, request.tabId,
                                                   request.persistentId)) {
                if (!presentHostedModalRequest(request)) {
                    rejectHostedModalRequest(request)
                }
                for (++index; index < pending.length; ++index) {
                    remaining.push(pending[index])
                }
                break
            } else {
                remaining.push(request)
                for (++index; index < pending.length; ++index) {
                    remaining.push(pending[index])
                }
                if (!hostView.selectTab(request.tabId)) {
                    var kept = []
                    for (var remainingIndex = 0;
                         remainingIndex < remaining.length; ++remainingIndex) {
                        if (remaining[remainingIndex] !== request) {
                            kept.push(remaining[remainingIndex])
                        }
                    }
                    remaining = kept
                    rejectHostedModalRequest(request)
                }
                break
            }
        }
        _pendingHostedModalRequests = remaining
        if (remaining.length && !_activeHostedModalTarget) {
            hostedModalRequestTimer.restart()
        } else {
            hostedModalRequestTimer.stop()
        }
    }

    function expirePendingHostedModalRequests() {
        if (_activeHostedModalTarget) {
            return
        }
        var pending = _pendingHostedModalRequests
        _pendingHostedModalRequests = []
        var remaining = []
        for (var index = 0; index < pending.length; ++index) {
            var request = pending[index]
            if (!_activeHostedModalTarget
                    && hostedModalRequestIsLive(request)
                    && hostedMessageTabIsSelected(request.hostView,
                                                  request.tabId,
                                                  request.persistentId)
                    && presentHostedModalRequest(request)) {
                for (++index; index < pending.length; ++index) {
                    remaining.push(pending[index])
                }
                break
            } else {
                rejectHostedModalRequest(request)
            }
        }
        _pendingHostedModalRequests = remaining
    }

    function createHostedMessageTarget(hostView, tabId, persistentId) {
        if (!hostView || !tabId || !String(tabId).length) {
            return null
        }
        return hostedMessageTargetComponent.createObject(browserPage, {
                                                            "hostView": hostView,
                                                            "tabId": String(tabId),
                                                            "persistentId": String(persistentId || ""),
                                                            "owner": browserPage
                                                        })
    }

    function releaseHostedMessageTarget(target) {
        if (!target || target.releaseScheduled) {
            return
        }
        if (_activeHostedModalTarget === target) {
            _activeHostedModalTarget = null
        }
        target.releaseScheduled = true
        _hostedMessageTargets.push(target)
        hostedMessageTargetCleanupTimer.restart()
    }

    function destroyHostedMessageTarget(target) {
        if (!target) {
            return
        }
        if (target.opener) {
            target.opener.destroy()
        }
        target.destroy()
    }

    function cancelHostedSelect(hostView, tabId, persistentId, data) {
        function matches(request) {
            return request && request.message === "embed:selectasync"
                    && request.hostView === hostView
                    && request.tabId === String(tabId)
                    && (!persistentId || request.persistentId === String(persistentId))
                    && String(request.data.id) === String(data.id)
        }

        var pending = []
        for (var index = 0; index < _pendingHostedModalRequests.length; ++index) {
            var request = _pendingHostedModalRequests[index]
            if (!matches(request)) pending.push(request)
        }
        _pendingHostedModalRequests = pending

        var target = _activeHostedModalTarget
        if (target && matches(target.modalRequest) && target.opener) {
            target.opener.message("embed:selectabort", data)
        }
    }

    function openHostedPicker(hostView, tabId, persistentId, message, data) {
        if (message === "embed:selectabort") {
            cancelHostedSelect(hostView, tabId, persistentId, data)
            return true
        }

        var pickerTopics = [ "embed:colorpicker", "embed:filepicker",
                             "embed:selectasync", "embed:selectabort", "embedui:downloadpicker",
                             "embed:downloadpicker" ]
        if (pickerTopics.indexOf(message) === -1) {
            return false
        }

        return queueHostedModalRequest("picker", hostView, tabId, persistentId,
                                       message, data)
    }

    function presentHostedPicker(hostView, tabId, persistentId, message, data, request) {
        if (!hostedMessageTabIsSelected(hostView, tabId, persistentId)) {
            return false
        }

        var target = createHostedMessageTarget(hostView, tabId, persistentId)
        if (!target) {
            return false
        }
        target.modalRequest = request
        _activeHostedModalTarget = target
        target.responseMessages = [ "embedui:colorpickerresponse",
                                    "filepickerresponse",
                                    "embedui:selectresponse" ]
        var pickerMessage = message === "embedui:downloadpicker"
                            ? "embed:downloadpicker" : message
        var opener = hostedPickerOpenerComponent.createObject(browserPage, {
                                                                    "pageStack": window.pageStack,
                                                                    "contentItem": target
                                                                })
        if (opener && pickerMessage === "embed:downloadpicker") {
            opener.downloadPickerClosed.connect(function() {
                browserPage.releaseHostedMessageTarget(target)
            })
        }
        if (!opener || !opener.message(pickerMessage, data)) {
            if (_activeHostedModalTarget === target) {
                _activeHostedModalTarget = null
            }
            if (opener) {
                opener.destroy()
            }
            target.destroy()
            return false
        }
        target.opener = opener
        // DownloadPicker replies through Gecko's observer service rather than
        // this content target. Its close signal releases the modal token.
        return true
    }

    function openHostedPopup(hostView, tabId, persistentId, message, data) {
        var popupTopics = [ "Content:ContextMenu", "embed:alert",
                            "embed:confirm", "embed:prompt", "embed:login",
                            "embed:auth", "embed:permissions",
                            "embed:webrtcrequest", "embed:popupblocked",
                            "embed:select" ]
        if (popupTopics.indexOf(message) === -1) {
            return false
        }

        return queueHostedModalRequest("popup", hostView, tabId, persistentId,
                                       message, data)
    }

    function presentHostedPopup(hostView, tabId, persistentId, message, data, request) {
        if (!hostedMessageTabIsSelected(hostView, tabId, persistentId)) {
            return false
        }

        var target = createHostedMessageTarget(hostView, tabId, persistentId)
        if (!target) {
            return false
        }
        target.modalRequest = request
        _activeHostedModalTarget = target
        target.beforeUnload = message === "embed:confirm" && !!data.inPermitUnload
        target.responseMessages = [ "alertresponse", "confirmresponse",
                                    "promptresponse", "embedui:login",
                                    "authresponse", "embedui:permissions",
                                    "embedui:webrtcresponse",
                                    "embedui:popupblocked", "selectresponse" ]
        var opener = hostedPopupOpenerComponent.createObject(browserPage, {
                                                                  "pageStack": window.pageStack,
                                                                  "parentItem": browserPage,
                                                                  "contentItem": target,
                                                                  "tabModel": hostView.browserTabModel
                                                              })
        if (!opener || !opener.message(message, data)) {
            if (_activeHostedModalTarget === target) {
                _activeHostedModalTarget = null
            }
            if (opener) {
                opener.destroy()
            }
            target.destroy()
            return false
        }
        target.opener = opener
        return true
    }

    function sendHostedClipboardPasteResponse(request, accepted) {
        if (!request || !request.target) {
            return
        }
        var data = request.data || {}
        var response = {
            "id": data.id,
            "accepted": !!accepted
        }
        if (data.winId) {
            response.winId = data.winId
        }
        request.target.sendAsyncMessage("embedui:clipboardreadpasteresponse", response)
    }

    function rejectHostedClipboardPaste(hostView, tabId, persistentId, data) {
        var response = {
            "id": data && data.id,
            "accepted": false
        }
        if (data && data.winId) {
            response.winId = data.winId
        }
        sendHostedMessageToTab(hostView, tabId, persistentId,
                               "embedui:clipboardreadpasteresponse", response)
    }

    function requestHostedClipboardPaste(hostView, tabId, persistentId, data) {
        // Clipboard permission is foreground-only. Do not bring a background
        // page forward for it; reject rather than showing a misleading dialog.
        if (!hostedMessageTabIsSelected(hostView, tabId, persistentId)) {
            rejectHostedClipboardPaste(hostView, tabId, persistentId, data)
            return
        }
        openHostedClipboardPasteDialog(hostView, tabId, persistentId, data)
    }

    function openPendingHostedClipboardPasteDialog() {
        if (window.pageStack.busy || !_pendingHostedClipboardPaste) {
            return
        }
        var request = _pendingHostedClipboardPaste
        _pendingHostedClipboardPaste = null
        if (!hostedMessageTabIsSelected(request.hostView, request.tabId,
                                        request.persistentId)) {
            sendHostedClipboardPasteResponse(request, false)
            return
        }
        openHostedClipboardPasteDialog(request.hostView, request.tabId,
                                       request.persistentId, request.data,
                                       request.target)
    }

    function openHostedClipboardPasteDialog(hostView, tabId, persistentId, data, existingTarget) {
        if (!hostedMessageTabIsSelected(hostView, tabId, persistentId)) {
            if (existingTarget) {
                sendHostedClipboardPasteResponse({
                                                      "data": data || {},
                                                      "target": existingTarget
                                                  }, false)
            } else {
                rejectHostedClipboardPaste(hostView, tabId, persistentId, data)
            }
            return
        }

        var target = existingTarget || createHostedMessageTarget(hostView, tabId,
                                                                  persistentId)
        if (!target) {
            return
        }
        target.responseMessages = [ "embedui:clipboardreadpasteresponse" ]
        var request = {
            "hostView": hostView,
            "tabId": String(tabId),
            "persistentId": String(persistentId || ""),
            "data": data || {},
            "target": target
        }

        if (window.pageStack.busy) {
            if (_pendingHostedClipboardPaste) {
                sendHostedClipboardPasteResponse(_pendingHostedClipboardPaste, false)
            }
            _pendingHostedClipboardPaste = request
            return
        }

        var page = window.pageStack.animatorPush(webView.clipboardPasteDialogComponent, {
                                                     "origin": request.data.origin || "",
                                                     "delay": Math.max(0,
                                                                       request.data.delay || 0)
                                                 })
        page.pageCompleted.connect(function(dialog) {
            dialog.accepted.connect(function() {
                sendHostedClipboardPasteResponse(request, true)
            })
            dialog.rejected.connect(function() {
                sendHostedClipboardPasteResponse(request, false)
            })
        })
    }

    function updateHostedSelection(hostView, tabId, persistentId, data) {
        if (!hostView || String(hostView.selectedTabId) !== String(tabId)) {
            return
        }

        if (_hostedTextSelectionController
                && _hostedSelectionTabId !== String(tabId)) {
            clearHostedSelection()
        }
        if (!_hostedTextSelectionController) {
            var target = createHostedMessageTarget(hostView, tabId, persistentId)
            if (!target) {
                return
            }
            _hostedTextSelectionController = webView.textSelectionControllerComponent.createObject(
                        browserPage, {
                            "contentItem": target,
                            // Keep selection handles above the hosted texture.
                            "z": 1
                        })
            _hostedSelectionTabId = String(tabId)
        }
        if (!_hostedTextSelectionController) {
            releaseHostedMessageTarget(target)
            return
        }
        // Selection coordinates are relative to the hosted surface, while the
        // controller is parented to the Browser page so that its handles paint
        // above the hosted texture.
        var origin = hostView.mapToItem(browserPage, 0, 0)
        _hostedTextSelectionController.anchors.leftMargin = origin.x
        _hostedTextSelectionController.anchors.topMargin = origin.y
        _hostedTextSelectionController.selectionRangeUpdated(data)
    }

    function refreshHostedHistoryIcon(hostView, tabId, persistentId, iconUrl) {
        if (hostView.privateMode || !iconUrl
                || !hostedMessageTabIsSelected(hostView, tabId, persistentId)) {
            return
        }
        var tab = hostedRuntimeTabByRuntimeId(hostView, tabId)
        if (!tab || !String(tab.location).length) {
            return
        }

        var fetcher = hostedHistoryIconFetcherComponent.createObject(browserPage, {
                                                                          "hostView": hostView,
                                                                          "tabId": String(tabId),
                                                                          "persistentId": String(persistentId),
                                                                          "location": String(tab.location),
                                                                          "locationRevision": String(tab.locationRevision)
                                                                      })
        if (fetcher) {
            fetcher.fetch(iconUrl)
        }
    }

    function handleHostedAsyncMessage(hostView, tabId, persistentId, message, data) {
        var targetTabId = String(tabId || (data && data.tabId) || "")
        var targetPersistentId = String(persistentId
                                        || (data && data.persistentId) || "")
        if (!targetTabId.length) {
            return
        }

        var tab = hostedRuntimeTabByRuntimeId(hostView, targetTabId)
        var selected = hostView && String(hostView.selectedTabId) === targetTabId
        // PickerOpener registers its listeners session-wide, but its delayed
        // replies must retain the tab that made this request.
        if (openHostedPicker(hostView, targetTabId, targetPersistentId,
                             message, data)) {
            return
        }
        if (openHostedPopup(hostView, targetTabId, targetPersistentId,
                            message, data)) {
            return
        }

        switch (message) {
        case "EmbedLiteChrome:TabCrashed":
            markHostedTabCrashed(hostView, targetTabId,
                                 targetPersistentId, data)
            break
        case "embed:clipboardreadpaste":
            requestHostedClipboardPaste(hostView, targetTabId, targetPersistentId, data)
            break
        case "Link:SetIcon":
            // data.url names the icon itself, not the page location. The
            // tab/persistent ids are the authoritative association.
            if (selected && hostedMessageTabIsLive(hostView, targetTabId,
                                                   targetPersistentId)) {
                if (_hostedAcceptedTouchIcon) {
                    break
                }
                var previousFavicon = _hostedFavicon
                _hostedAcceptedTouchIcon = !!data.isRichIcon
                _hostedFavicon = data.url || ""
                if (_hostedFavicon && _hostedFavicon !== previousFavicon) {
                    refreshHostedHistoryIcon(hostView, targetTabId,
                                             targetPersistentId, _hostedFavicon)
                }
            }
            break
        case "embed:pageMetadata":
            if (selected && (!data.url || (tab && data.url === String(tab.location)))) {
                if (data.title) {
                    _hostedMetadataTitle = data.title
                }
                var richIcon = !!data.isRichIcon
                if (data.favicon && (richIcon || !_hostedAcceptedTouchIcon)) {
                    var oldFavicon = _hostedFavicon
                    _hostedAcceptedTouchIcon = richIcon
                    _hostedFavicon = data.favicon
                    if (_hostedFavicon !== oldFavicon) {
                        refreshHostedHistoryIcon(hostView, targetTabId,
                                                 targetPersistentId, _hostedFavicon)
                    }
                }
                syncHostedContainerState(hostView)
            }
            break
        case "embed:internalPage":
            if (selected && data.url) {
                loadInternalPage(data.url)
            }
            break
        case "Content:SelectionRange":
            updateHostedSelection(hostView, targetTabId, targetPersistentId, data)
            break
        case "Content:SelectionSwap":
            if (_hostedTextSelectionController
                    && _hostedSelectionTabId === targetTabId) {
                _hostedTextSelectionController.swap()
            }
            break
        case "Content:SelectionCopied":
            if (data.succeeded && _hostedTextSelectionController
                    && _hostedSelectionTabId === targetTabId) {
                _hostedTextSelectionController.showNotification()
            }
            break
        case "embed:find":
            if (selected) {
                webView.findInPageHasResult = data.r === 0 || data.r === 2
            }
            break
        case "Link:AddSearch":
            if (!hostView.privateMode && data.engine) {
                SearchEngineModel.add(data.engine.title, data.engine.href)
            }
            break
        case "embed:viewportfit":
            updateHostedViewportFitState(hostView, targetTabId,
                                         targetPersistentId, data)
            break
        case "embed:fullscreenchanged":
            if (selected) {
                if (hostView.fullscreen) {
                    overlay.animator.showFullscreen()
                } else {
                    overlay.animator.showChrome()
                }
            }
            break
        case "embed:contentOrientationChanged":
            if (selected) {
                acknowledgeHostedOrientation(hostView, data)
            }
            break
        case "chrome:contentloaded":
            if (selected) {
                requestHostedThumbnail()
            }
            break
        }
    }

    function initializeHostedContentBridge(hostView) {
        if (!hostView) {
            return
        }

        // QmlMozView registers EmbedLite's core helper for every hosted tab.
        // Install only the Browser-specific frame scripts here.
        hostView.loadFrameScript("file:///usr/share/sailfish-browser/shared/ViewportFit.js")
        hostView.loadFrameScript("file:///usr/share/sailfish-browser/shared/PageMetadata.js")

        var listeners = [ "Content:SelectionRange", "Content:SelectionCopied",
                          "Content:SelectionSwap", "embed:clipboardreadpaste",
                          "embed:fullscreenchanged", "chrome:contentloaded",
                          "embed:pageMetadata", "embed:internalPage",
                          "Link:SetIcon", "Link:AddFeed",
                          "Link:AddSearch", "embed:find",
                          "embed:contentOrientationChanged", "embed:viewportfit",
                          "Content:ContextMenu", "embed:alert", "embed:confirm",
                          "embed:prompt", "embed:login", "embed:auth",
                          "embed:permissions", "embed:webrtcrequest",
                          "embed:popupblocked", "embed:select",
                          "embed:colorpicker", "embed:filepicker",
                          "embed:selectasync", "embed:selectabort", "embedui:downloadpicker",
                          "embed:downloadpicker" ]
        for (var index = 0; index < listeners.length; ++index) {
            hostView.addMessageListener(listeners[index])
        }
    }

    function restoreRuntimeTabs(hostView) {
        var session = runtimeSession(hostView)
        if (session) {
            return session.restoreRuntimeTabs(hostView)
        }
    }

    function queueRuntimeCommand(command) {
        var session = runtimeSession(null)
        if (session) {
            return session.queueRuntimeCommand(command)
        }
    }

    function drainRuntimeNewTabs() {
        var session = runtimeSession(null)
        if (session) {
            return session.drainRuntimeNewTabs()
        }
    }

    function removeQueuedRuntimeNewTab(persistentId) {
        var session = runtimeSession(null)
        if (session) {
            return session.removeQueuedRuntimeNewTab(persistentId)
        }
    }

    function cancelPendingRuntimeNavigation() {
        var session = runtimeSession(null)
        if (session) {
            return session.cancelPendingRuntimeNavigation()
        }
    }

    function queueRuntimeClose(persistentId) {
        var session = runtimeSession(null)
        if (session) {
            return session.queueRuntimeClose(persistentId)
        }
    }

    function finishRuntimeTabsClear() {
        var session = runtimeSession(null)
        if (session) {
            return session.finishRuntimeTabsClear()
        }
    }

    function startNextRuntimeClose(runtimeHostView) {
        var session = runtimeSession(runtimeHostView)
        if (session) {
            return session.startNextRuntimeClose(runtimeHostView)
        }
    }

    function resolveRuntimeCloseAfterSnapshot(runtimeHostView) {
        var session = runtimeSession(runtimeHostView)
        if (session) {
            return session.resolveRuntimeCloseAfterSnapshot(runtimeHostView)
        }
    }

    function runtimeTabCloseResult(runtimeId, closed) {
        var session = runtimeSession(null)
        if (session) {
            return session.runtimeTabCloseResult(runtimeId, closed)
        }
    }

    function dispatchRuntimeCommand(command, runtimeHostView) {
        var session = runtimeSession(runtimeHostView)
        if (session) {
            return session.dispatchRuntimeCommand(command, runtimeHostView)
        }
    }

    function flushSelectedRuntimeNavigation(runtimeHostView) {
        var session = runtimeSession(runtimeHostView)
        if (session) {
            return session.flushSelectedRuntimeNavigation(runtimeHostView)
        }
    }

    function flushRuntimeCommands(runtimeHostView) {
        var session = runtimeSession(runtimeHostView)
        if (session) {
            return session.flushRuntimeCommands(runtimeHostView)
        }
    }

    function hasPendingRuntimeTitles() {
        var session = runtimeSession(null)
        if (session) {
            return session.hasPendingRuntimeTitles()
        }
    }

    function pairedRuntimeSnapshot(snapshot, acceptDeferredTitles) {
        var session = runtimeSession(null)
        if (session) {
            return session.pairedRuntimeSnapshot(snapshot, acceptDeferredTitles)
        }
    }

    function runtimeSnapshotChanged(snapshot) {
        var session = runtimeSession(null)
        if (session) {
            return session.runtimeSnapshotChanged(snapshot)
        }
    }

    function applyRuntimeSnapshot(acceptDeferredTitles, runtimeHostView) {
        var session = runtimeSession(runtimeHostView)
        if (session) {
            return session.applyRuntimeSnapshot(acceptDeferredTitles, runtimeHostView)
        }
    }

    function refreshRuntimeHistory() {
        var search = overlay.searchField.text === browserPage.url
                ? "" : overlay.searchField.text
        historyModel.search(search)
    }

    function hostedRuntimeTab(persistentId, location, locationRevision) {
        if (!chromeHostView) {
            return null
        }
        var tabs = chromeHostView.tabModel.snapshot()
        for (var index = 0; index < tabs.length; ++index) {
            var tab = tabs[index]
            if (String(tab.persistentId) === String(persistentId)
                    && String(tab.location) === String(location)
                    && String(tab.locationRevision) === String(locationRevision)) {
                return tab
            }
        }
        return null
    }

    property bool _hostedThumbnailCapturePending
    property bool _hostedThumbnailCaptureScheduled
    property bool _hostedThumbnailCaptureSuspended
    property int _hostedThumbnailRequiredGeneration: 1

    function resetHostedThumbnailCapture(hostView) {
        hostedThumbnailCaptureTimer.stop()
        _hostedThumbnailCapturePending = false
        _hostedThumbnailCaptureScheduled = false
        _hostedThumbnailRequiredGeneration = hostView
                ? hostView.platformFrameGeneration + 1 : 1
    }

    function scheduleHostedThumbnailCapture(hostView) {
        if (!_hostedThumbnailCaptureSuspended
                && !_hostedThumbnailCaptureScheduled) {
            _hostedThumbnailCaptureScheduled = true
            hostedThumbnailCaptureTimer.restart()
        }
    }

    function continueHostedThumbnailCapture(hostView) {
        if (_hostedThumbnailCapturePending && hostView
                && hostView.platformFrameGeneration
                   >= _hostedThumbnailRequiredGeneration) {
            scheduleHostedThumbnailCapture(hostView)
        }
    }

    property var privateCoverGrab
    property var _privateTabGrabs: ({})
    property string _privateCoverTab
    property bool _privateCoverPending
    property int _privateCaptureGeneration

    function prunePrivateTabGrabs(snapshot) {
        var grabs = {}
        for (var i = 0; i < snapshot.length; ++i) {
            var id = String(snapshot[i].persistentId)
            if (_privateTabGrabs[id]) grabs[id] = _privateTabGrabs[id]
        }
        _privateTabGrabs = grabs
        if (!snapshot.length) {
            privateCoverGrab = null
            _privateCoverTab = ""
        }
    }

    function requestPrivateCover(tabViewCapture) {
        var view = chromeHostView
        if (!view || !view.privateMode || !view.selectedTabId.length
                || !webView.foreground || (_privateCoverPending && !tabViewCapture)) return null
        var tabId = view.selectedTabId
        var persistentId = selectedPersistentId(view)
        if (!persistentId.length) return null
        var generation = --_privateCaptureGeneration
        _privateCoverPending = true
        var accepted = view.grabToImage(function(result) {
            _privateCoverPending = false
            if (view === chromeHostView && view.selectedTabId === tabId) {
                var grabs = {}
                for (var id in _privateTabGrabs) grabs[id] = _privateTabGrabs[id]
                grabs[persistentId] = result
                _privateTabGrabs = grabs
                privateCoverGrab = result
                _privateCoverTab = tabId
                webView.privateTabModel.updateThumbnailPath(Number(persistentId), result.url)
                if (tabViewCapture) {
                    browserPage.hostedThumbnailGrabbed(persistentId, "", "", generation)
                }
            }
        }, Qt.size(Math.max(1, Math.round(width / 2)), Math.max(1, Math.round(height / 2))))
        if (!accepted) _privateCoverPending = false
        return accepted ? { "persistentId": persistentId, "generation": generation } : null
    }

    function requestHostedThumbnail() {
        if (webView.privateMode) {
            requestPrivateCover()
            return
        }
        _hostedThumbnailCapturePending = true
        continueHostedThumbnailCapture(chromeHostView)
    }

    function captureHostedThumbnail(tabViewCapture) {
        if (_hostedThumbnailCaptureSuspended && !tabViewCapture) {
            return null
        }
        var hostView = chromeHostView
        if (!hostView || webView.privateMode || !browserPage.active
                || !hostView.active || !hostView.visible) {
            return false
        }
        var persistentId = selectedPersistentId(hostView)
        if (!persistentId.length) {
            return false
        }
        var tabs = hostView.tabModel.snapshot()
        for (var index = 0; index < tabs.length; ++index) {
            var tab = tabs[index]
            if (String(tab.tabId) === hostView.selectedTabId
                    && String(tab.location).length
                    && String(tab.location) !== "about:blank") {
                var generation = hostedThumbnailGrabber.grab(
                            hostView, persistentId, String(tab.location),
                            String(tab.locationRevision),
                            Qt.size(hostView.width, hostView.height))
                if (generation) {
                    return {
                        "persistentId": persistentId,
                        "location": String(tab.location),
                        "locationRevision": String(tab.locationRevision),
                        "generation": generation
                    }
                }
            }
        }
        return null
    }

    function beginHostedTabViewThumbnailCapture() {
        if (webView.privateMode) {
            return requestPrivateCover(true)
        }
        // No other capture may supersede this one before the tab page has
        // deactivated the hosted view. Resume automatic captures when the
        // browser page becomes active again.
        hostedThumbnailCaptureTimer.stop()
        _hostedThumbnailCapturePending = false
        _hostedThumbnailCaptureScheduled = false
        _hostedThumbnailCaptureSuspended = true
        hostedThumbnailGrabber.invalidateAll()
        var capture = captureHostedThumbnail(true)
        if (!capture) {
            requestHostedThumbnailRetry()
        }
        return capture
    }

    function resumeHostedThumbnailCapture() {
        if (!_hostedThumbnailCaptureSuspended) {
            return
        }
        _hostedThumbnailCaptureSuspended = false
        var hostView = chromeHostView
        if (_hostedThumbnailCapturePending && hostView
                && hostView.platformFrameGeneration
                   >= _hostedThumbnailRequiredGeneration) {
            scheduleHostedThumbnailCapture(hostView)
        }
    }

    function requestHostedThumbnailRetry() {
        var hostView = chromeHostView
        _hostedThumbnailCapturePending = true
        _hostedThumbnailRequiredGeneration = hostView
                ? hostView.platformFrameGeneration : 1
    }

    function cancelHostedThumbnailCaptureForBackground() {
        hostedThumbnailCaptureTimer.stop()
        _hostedThumbnailCapturePending = false
        _hostedThumbnailCaptureScheduled = false
        hostedThumbnailGrabber.invalidateAll()
    }

    function cancelHostedThumbnailCapture(persistentId, generation) {
        hostedThumbnailGrabber.cancel(persistentId, generation)
        requestHostedThumbnailRetry()
    }

    Timer {
        id: hostedThumbnailCaptureTimer

        interval: 100
        onTriggered: {
            browserPage._hostedThumbnailCaptureScheduled = false
            var hostView = chromeHostView
            if (browserPage.captureHostedThumbnail()) {
                browserPage._hostedThumbnailCapturePending = false
                // An early first-paint capture must not make the finished
                // load wait for another frame which may never arrive. Keep
                // the navigation's original frame requirement until loading
                // finishes; that accepted frame can then be captured again
                // against the final committed tab snapshot.
                if (!hostView || !hostView.loading) {
                    browserPage._hostedThumbnailRequiredGeneration = hostView
                            ? hostView.platformFrameGeneration + 1 : 1
                }
            }
        }
    }

    function updateHostedThumbnail(persistentId, location, locationRevision, fileName) {
        var tab = hostedRuntimeTab(persistentId, location, locationRevision)
        if (tab && webView.persistentTabModel.runtimeIdForPersistentId(
                    persistentId) === String(tab.tabId)) {
            webView.persistentTabModel.updateThumbnailPath(Number(persistentId), fileName)
            hostedThumbnailUpdated(persistentId, location, locationRevision,
                                   fileName)
        } else {
            hostedThumbnailGrabber.discard(fileName)
        }
    }

    function bringToForeground(window) {
        if (!webView.foreground && window) {
            window.raise()
        }
    }

    function activateNewTabView() {
        // Only open new tab if not blocked MDM, otherwise just bring to foreground
        if (AccessPolicy.browserEnabled) {
            pageStack.pop(browserPage, PageStackAction.Immediate)
            overlay.enterNewTabUrl(PageStackAction.Immediate)
        }
        bringToForeground(webView.chromeWindow)
        // after bringToForeground, webView has focus => activate chrome
        window.activate()
    }

    cutoutMode: CutoutMode.FullScreen
    background: null
    onUrlChanged: {
        if (chromeHostView) {
            _hostedMetadataTitle = ""
            _hostedFavicon = ""
            _hostedAcceptedTouchIcon = false
            webView.findInPageHasResult = false
            syncHostedContainerState(chromeHostView)
        }
    }
    onStatusChanged: {
        if (status == PageStatus.Active) {
            resumeHostedThumbnailCapture()
        } else {
            finishHostedOrientationWait()
        }
        if (overlay.enteringNewTabUrl
                || webView.tabModel.count === 0) {
            return
        }

        if (status == PageStatus.Inactive && overlay.visible) {
            overlay.animator.hide()
            overlay.toolBar.certOverlayActive = false
        }
    }

    property int pageOrientation: pageStack.currentPage._windowOrientation
    onPageOrientationChanged: {
        // When on other pages update immediately.
        if (!active) {
            webView.applyContentOrientation(pageOrientation)
        }
    }
    onOrientationChanged: webView.applyContentOrientation(orientation)

    orientationTransitions: orientationFader.orientationTransition

    Keys.onPressed: {
        webView.handleKeyPress(event.key)
    }

    Shared.OrientationFader {
        id: orientationFader

        z: 100
        visible: browserPage.chromeHostView || webView.contentItem
        immediate: true
        page: browserPage
        fadeTarget: overlay.animator.allowContentUse ? overlay : overlay.dragArea
        color: browserPage.chromeHostView
               ? (browserPage.contentFullscreen ? "black" : browserPage._hostedSurfaceColor)
               : (webView.contentItem ? (webView.resourceController.videoActive
                                         && webView.contentItem.fullscreen
                                         ? "black" : webView.contentItem.backgroundColor)
                                      : "white")
    }

    HistoryModel {
        id: historyModel
    }

    Private.VirtualKeyboardObserver {
        id: virtualKeyboardObserver

        active: webView.enabled || browserPage.chromeHostView
        transpose: window._transpose
        orientation: browserPage.orientation

        onWindowChanged: webView.chromeWindow = window

    }

    Browser.DownloadRemorsePopup { id: downloadPopup }

    Shared.WebView {
        id: webView

        enabled: overlay.animator.allowContentUse
        fullscreenHeight: portrait ? Screen.height : Screen.width
        portrait: browserPage.isPortrait
        contentItem: browserPage.chromeHostView
        toolbarHeight: overlay.animator.opened ? overlay.toolBar.rowHeight : 0
        rotationHandler: browserPage
        imOpened: virtualKeyboardObserver.opened
        canShowSelectionMarkers: !orientationFader.waitForWebContentOrientationChanged
        historyModel: historyModel

        // Show overlay immediately at top if needed.
        onTabModelChanged: handleModelChanges(true)

        onForegroundChanged: {
            if (foreground && webView.chromeWindow) {
                webView.chromeWindow.raise()
            }
        }

        function applyContentOrientation(orientation) {
            browserPage.beginHostedOrientationWait(browserPage.chromeHostView, orientation)
            reportWindowOrientation(_qtScreenOrientation(orientation))
        }

        // Both model change and model count change are connected to this.
        function handleModelChanges(openOverlayImmediately) {
            if (webView.completed
                    && (!webView.tabModel || webView.tabModel.count === 0)) {
                overlay.startPage(openOverlayImmediately ? PageStackAction.Immediate
                                                         : PageStackAction.Animated)
            }
        }
    }

    HostedThumbnailGrabber {
        id: hostedThumbnailGrabber

        onGrabReady: browserPage.hostedThumbnailGrabbed(
                         persistentId, location, locationRevision, generation)
        onCaptureReady: browserPage.updateHostedThumbnail(persistentId, location,
                                                           locationRevision, fileName)
    }

    Component {
        id: hostedHistoryIconFetcherComponent

        DataFetcher {
            property var hostView
            property string tabId
            property string persistentId
            property string location
            property string locationRevision

            onDataChanged: {
                var tab = browserPage.hostedRuntimeTabByRuntimeId(hostView, tabId)
                if (!webView.privateMode && tab
                        && String(tab.persistentId) === persistentId
                        && String(tab.location) === location
                        && String(tab.locationRevision) === locationRevision) {
                    FaviconManager.add("history", location, data,
                                       hasAcceptedTouchIcon)
                }
                destroy()
            }
        }
    }

    Timer {
        id: hostedMessageTargetCleanupTimer

        interval: 0
        onTriggered: {
            var targets = browserPage._hostedMessageTargets
            browserPage._hostedMessageTargets = []
            for (var index = 0; index < targets.length; ++index) {
                browserPage.destroyHostedMessageTarget(targets[index])
            }
            browserPage.processPendingHostedModalRequests(browserPage.chromeHostView)
        }
    }

    Timer {
        id: hostedModalRequestTimer

        interval: 1000
        onTriggered: browserPage.expirePendingHostedModalRequests()
    }

    Timer {
        id: hostedGenericDuplicateTimer

        interval: 0
        onTriggered: {
            var pending = browserPage._pendingHostedGenericMessages
            browserPage._pendingHostedGenericMessages = []
            browserPage._hostedTabAsyncMessages = []
            for (var index = 0; index < pending.length; ++index) {
                var generic = pending[index]
                browserPage.handleHostedAsyncMessage(generic.hostView,
                                                     generic.tabId,
                                                     generic.persistentId,
                                                     generic.message,
                                                     generic.data)
            }
        }
    }

    Component {
        id: hostedMessageTargetComponent

        QtObject {
            property var hostView
            property var owner
            property string tabId
            property string persistentId
            property var opener
            property var responseMessages
            property var modalRequest
            property bool beforeUnload
            property bool releaseScheduled
            readonly property int uniqueId: hostView ? hostView.uniqueId : 0
            readonly property real resolution: hostView ? hostView.resolution : 1.0
            readonly property point scrollableOffset: hostView ? hostView.scrollableOffset
                                                               : Qt.point(0, 0)

            function addMessageListener(name) {
                if (hostView) {
                    hostView.addMessageListener(name)
                }
            }

            function cancelPendingNavigation() {
                // QmlMozView currently has a selected-tab-only cancel API.
                // Never cancel the wrong tab while a delayed prompt is open.
                if (hostView && String(hostView.selectedTabId) === tabId) {
                    hostView.cancelPendingNavigation()
                }
            }

            function sendAsyncMessage(name, data) {
                var sent = owner && owner.sendHostedMessageToTab(
                            hostView, tabId, persistentId, name, data,
                            beforeUnload && name === "confirmresponse")
                if (owner && responseMessages && responseMessages.indexOf(name) !== -1) {
                    owner.releaseHostedMessageTarget(this)
                }
                return sent
            }
        }
    }

    Component {
        id: hostedPickerOpenerComponent

        Pickers.PickerOpener {
        }
    }

    Component {
        id: hostedPopupOpenerComponent

        Popups.PopupOpener {
            property bool _hostedContextMenuOpened

            onAboutToOpenContextMenu: {
                _hostedContextMenuOpened = true
                if (Qt.inputMethod.visible) {
                    browserPage.focus = true
                    Qt.inputMethod.hide()
                }

                if (contentItem && contentItem.hostView
                        && String(contentItem.hostView.selectedTabId) === contentItem.tabId) {
                    browserPage.captureHostedThumbnail()
                }
                if (data.types.indexOf("content-text") !== -1
                        || data.types.indexOf("input-text") !== -1) {
                    contentItem.sendAsyncMessage("Browser:SelectionStart", {
                                                     "xPos": data.xPos,
                                                     "yPos": data.yPos,
                                                     "setFocus": data.types.indexOf("input-text") !== -1
                                                 })
                }
                if (data.types.indexOf("image") === -1
                        && data.types.indexOf("link") === -1) {
                    // Text-only long presses start selection but do not open
                    // PopupOpener's context menu, so no activeChanged edge
                    // will release this request target.
                    browserPage.releaseHostedMessageTarget(contentItem)
                }
            }

            onActiveChanged: {
                if (_hostedContextMenuOpened && !active) {
                    _hostedContextMenuOpened = false
                    browserPage.releaseHostedMessageTarget(contentItem)
                }
            }
        }
    }

    Loader {
        id: chromeHostLoader

        property bool privateSession: false

        anchors.fill: parent
        active: webView.persistentTabModel.loaded
        sourceComponent: chromeHostComponent
        onLoaded: {
            item.hostedSession.restoreRuntimeTabs(item)
            item.hostedSession.applyRuntimeSnapshot(false, item)
        }
    }

    Loader {
        id: privateChromeHostLoader

        property bool privateSession: true

        anchors.fill: parent
        active: webView.privateTabModel.loaded
        sourceComponent: chromeHostComponent
        onLoaded: {
            item.hostedSession.restoreRuntimeTabs(item)
            item.hostedSession.applyRuntimeSnapshot(false, item)
        }
    }

    Component {
        id: chromeHostComponent

            QmlMozView {
                id: chromeView

                property alias hostedSession: tabSession
                readonly property var browserTabModel: privateMode
                                                       ? webView.privateTabModel : webView.persistentTabModel
                privateMode: parent.privateSession

                Shared.HostedTabSession {
                    id: tabSession

                    model: chromeView.browserTabModel
                    view: chromeView
                    onSnapshotApplied: {
                        if (chromeView.privateMode) browserPage.prunePrivateTabGrabs(snapshot)
                        if (chromeView === browserPage.chromeHostView) browserPage.pruneHostedTabCrashStates(snapshot)
                    }
                    onHistoryChanged: browserPage.refreshRuntimeHistory()
                    onEmpty: {
                        if (chromeView === browserPage.chromeHostView && browserPage.active) overlay.startPage()
                    }
                    privateMode: chromeView.privateMode
                    onLocationInvalidated: hostedThumbnailGrabber.invalidate(persistentId)
                    onRuntimeTabsClearFinished: {
                        if (chromeView === browserPage.chromeHostView) browserPage.runtimeTabsClearFinished()
                    }
                }

                property string _qmozChromeInitialUrl: ""
                property QtObject pickerOpener
                property QtObject popupOpener
                property bool chromeForced: virtualKeyboardObserver.opened
                                            || webView.fixedToolbarConfig.value
                                            || overlay.toolBar.findInPageActive
                                            || (browserPage._hostedTextSelectionController
                                                && browserPage._hostedTextSelectionController.active)

                anchors {
                    fill: parent
                    topMargin: browserPage.hostedDisplayCutoutAllowed
                               ? 0 : browserPage._hostedCutoutTop
                    rightMargin: browserPage.hostedDisplayCutoutAllowed
                                 ? 0 : browserPage._hostedCutoutRight
                    bottomMargin: browserPage.hostedDisplayCutoutAllowed
                                  ? 0 : browserPage._hostedCutoutBottom
                    leftMargin: browserPage.hostedDisplayCutoutAllowed
                                ? 0 : browserPage._hostedCutoutLeft
                }
                active: browserPage.active && privateMode === webView.privateMode
                orientation: webView._screenOrientation
                clip: true
                focus: true
                visible: privateMode === webView.privateMode
                chromeGestureEnabled: !browserPage.hostedTabCrashed
                                      && !chromeForced && active
                                      && overlay.animator.allowContentUse
                chromeGestureThreshold: webView.toolbarHeight / 3
                dynamicToolbarHeight: chromeForced ? 0 : webView.toolbarHeight
                marginTop: 0
                marginRight: 0
                marginBottom: virtualKeyboardObserver.opened
                              ? virtualKeyboardObserver.imSize
                              : (chromeForced ? webView.toolbarHeight : 0)
                marginLeft: 0
                safeAreaInsetTop: browserPage.hostedDisplayCutoutAllowed
                                  ? browserPage._hostedCutoutTop : 0
                safeAreaInsetRight: browserPage.hostedDisplayCutoutAllowed
                                    ? browserPage._hostedCutoutRight : 0
                safeAreaInsetBottom: browserPage.hostedDisplayCutoutAllowed
                                     ? browserPage._hostedCutoutBottom : 0
                safeAreaInsetLeft: browserPage.hostedDisplayCutoutAllowed
                                   ? browserPage._hostedCutoutLeft : 0
                throttlePainting: !webView.foreground && !webView.resourceController.videoActive
                                  && webView.applicationVisible || !webView.applicationVisible

                Component.onCompleted: {
                    browserPage.initializeHostedContentBridge(chromeView)
                    pickerOpener = hostedPickerOpenerComponent.createObject(chromeView, {
                                                                                 "pageStack": window.pageStack,
                                                                                 "contentItem": chromeView
                                                                             })
                    popupOpener = hostedPopupOpenerComponent.createObject(chromeView, {
                                                                              "pageStack": window.pageStack,
                                                                              "parentItem": browserPage,
                                                                              "contentItem": chromeView,
                                                                              "tabModel": chromeView.browserTabModel
                    })
                    browserPage.syncHostedDesktopMode(chromeView)
                    browserPage.syncHostedContainerState(chromeView)
                    browserPage.updateHostedViewSuspension(chromeView)
                }

                Component.onDestruction: {
                    if (chromeView === browserPage.chromeHostView) {
                        browserPage.finishHostedOrientationWait()
                        webView.clearHostedState()
                    }
                }

                onChromeForcedChanged: {
                    if (chromeForced) {
                        chrome = true
                    }
                }

                onSelectedTabChanged: {
                    tabSession.applyRuntimeSnapshot(false, chromeView)
                    if (chromeView !== browserPage.chromeHostView) return
                    if (virtualKeyboardObserver.opened) {
                        browserPage.focus = true
                        Qt.inputMethod.hide()
                    }
                    chrome = true
                    browserPage.resetHostedThumbnailCapture(chromeView)
                    browserPage.requestHostedThumbnail()
                    browserPage.clearHostedSelection()
                    browserPage._hostedMetadataTitle = ""
                    browserPage._hostedFavicon = ""
                    browserPage._hostedAcceptedTouchIcon = false
                    browserPage.applyHostedViewportFitState(chromeView)
                    browserPage.applyRuntimeSnapshot(false, chromeView)
                    browserPage.syncHostedDesktopMode(chromeView)
                    browserPage.syncHostedContainerState(chromeView)
                    browserPage.processPendingHostedModalRequests(chromeView)
                }

                onLoadingChanged: {
                    if (chromeView !== browserPage.chromeHostView) return
                    if (loading) {
                        browserPage.clearHostedTabCrashState(chromeView.selectedTabId)
                        chrome = true
                        browserPage.resetHostedThumbnailCapture(chromeView)
                        browserPage._hostedMetadataTitle = ""
                        browserPage._hostedFavicon = ""
                        browserPage._hostedAcceptedTouchIcon = false
                        webView.findInPageHasResult = false
                    } else {
                        browserPage.requestHostedThumbnail()
                    }
                    browserPage.syncHostedContainerState(chromeView)
                }

                onFirstPaint: if (chromeView === browserPage.chromeHostView) browserPage.requestHostedThumbnail()
                onAtYBeginningChanged: {
                    if (atYBeginning && active && domContentLoaded) {
                        chrome = true
                    }
                }
                onAtYEndChanged: {
                    // Keep chrome visible when the whole page fits in the viewport.
                    if (!atYBeginning && atYEnd && !chromeForced && chrome
                            && active && domContentLoaded) {
                        chrome = false
                    }
                }
                onPlatformFrameGenerationChanged: {
                    if (chromeView !== browserPage.chromeHostView) return
                    if (privateMode && browserPage._privateCoverTab !== selectedTabId) browserPage.requestPrivateCover()
                    browserPage.noteHostedOrientationFrame(chromeView)
                    browserPage.continueHostedThumbnailCapture(chromeView)
                }
                onTouched: {
                    if (browserPage.contentFullscreen) {
                        fullscreenCloseVisibleTimer.restart()
                    }
                    if (browserPage._hostedTextSelectionController) {
                        browserPage.clearHostedSelection()
                    }
                }
                onTitleChanged: browserPage.syncHostedContainerState(chromeView)
                onLoadProgressChanged: browserPage.syncHostedContainerState(chromeView)
                onCanGoBackChanged: browserPage.syncHostedContainerState(chromeView)
                onCanGoForwardChanged: browserPage.syncHostedContainerState(chromeView)
                onSecurityChanged: browserPage.syncHostedContainerState(chromeView, true)
                onActiveChanged: {
                    if (!active && chromeView === browserPage.chromeHostView) {
                        browserPage.finishHostedOrientationWait()
                    }
                    browserPage.updateHostedViewSuspension(chromeView)
                }
                onVisibleChanged: browserPage.updateHostedViewSuspension(chromeView)
                onFullscreenChanged: {
                    if (chromeView !== browserPage.chromeHostView) return
                    if (fullscreen) {
                        overlay.animator.showFullscreen()
                    } else {
                        overlay.animator.showChrome()
                    }
                }

                onRecvAsyncMessageFromTab: {
                    browserPage.noteHostedAsyncMessage(tabId, persistentId, message)
                    browserPage.handleHostedAsyncMessage(chromeView, tabId, persistentId,
                                                         message, data)
                }

                onRecvAsyncMessage: {
                    browserPage.queueHostedGenericAsyncMessage(chromeView,
                                                                message, data)
                }

                onTabCloseResult: tabSession.runtimeTabCloseResult(tabId, closed)

                onWindowCloseRequestedFromTab: {
                    if (chromeView !== browserPage.chromeHostView) return
                    // Gecko removes this tab itself. Keep only tab-scoped UI
                    // state from surviving until the authoritative snapshot.
                    if (browserPage._hostedSelectionTabId === String(tabId)) {
                        browserPage.clearHostedSelection()
                    }
                }

                Connections {
                    target: chromeView.tabModel
                    ignoreUnknownSignals: true
                    onRevisionChanged: {
                        tabSession.applyRuntimeSnapshot(false, chromeView)
                        if (chromeView !== browserPage.chromeHostView) return
                        browserPage.applyHostedViewportFitState(chromeView)
                        browserPage.applyRuntimeSnapshot(false, chromeView)
                        browserPage.syncHostedDesktopMode(chromeView)
                        // The accepted frame can arrive before its committed
                        // tab snapshot. Retry from this state transition
                        // without waiting for an unrelated later frame.
                        browserPage.continueHostedThumbnailCapture(chromeView)
                    }
                }

                Connections {
                    target: webView
                    ignoreUnknownSignals: true
                    onForegroundChanged: {
                        browserPage.updateHostedViewSuspension(chromeView)
                        if (chromeView !== browserPage.chromeHostView) return
                        if (!webView.foreground) {
                            browserPage.cancelHostedThumbnailCaptureForBackground()
                        }
                        browserPage.updateHostedViewSuspension(chromeView)
                        if (webView.foreground) {
                            browserPage.resetHostedThumbnailCapture(chromeView)
                            browserPage.requestHostedThumbnail()
                        }
                    }
                    onPrivateModeChanged: browserPage.updateHostedViewSuspension(chromeView)
                    onHostedLoadRequested: if (chromeView === browserPage.chromeHostView) browserPage.loadHostedContainerRequest(url, fromExternal)
                    onHostedReloadRequested: if (chromeView === browserPage.chromeHostView) browserPage.reload()
                    onHostedGoBackRequested: if (chromeView === browserPage.chromeHostView) browserPage.goBack()
                    onHostedGoForwardRequested: if (chromeView === browserPage.chromeHostView) browserPage.goForward()
                }
            }
    }

    Item {
        id: hostedTabCrashPage

        anchors.fill: parent
        visible: browserPage.chromeHostView && browserPage.hostedTabCrashed

        Shared.Background {
            anchors.fill: parent
        }

        MouseArea {
            anchors.fill: parent
        }

        Column {
            anchors.centerIn: parent
            width: parent.width - 2 * Theme.horizontalPageMargin
            spacing: Theme.paddingLarge

            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                source: "image://theme/icon-l-attention"
                sourceSize.width: Theme.iconSizeLarge
                sourceSize.height: Theme.iconSizeLarge
            }

            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeLarge
                //% "Tab crashed"
                text: qsTrId("sailfish_browser-la-tab_crashed")
            }

            Label {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                //% "This page stopped working."
                text: qsTrId("sailfish_browser-la-tab_crashed_description")
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                //% "Reload tab"
                text: qsTrId("sailfish_browser-la-reload_tab")
                onClicked: browserPage.reload()
            }
        }
    }

    Connections {
        target: window.pageStack
        ignoreUnknownSignals: true
        onBusyChanged: {
            if (!window.pageStack.busy) {
                browserPage.openPendingHostedClipboardPasteDialog()
            }
        }
    }

    IconButton {
        id: fullscreenClose

        opacity: fullscreenCloseVisibleTimer.running || pressed ? 1.0 : 0.0
        Behavior on opacity { FadeAnimation {} }
        visible: opacity > 0
        x: Theme.paddingLarge
        y: Theme.paddingLarge
        icon.source: "image://theme/icon-m-close"
        onClicked: browserPage.exitFullscreen()

        Timer {
            id: fullscreenCloseVisibleTimer

            interval: 2000
            running: browserPage.contentFullscreen
        }
    }

    // Use Connections so that target updates when model changes.
    Connections {
        target: AccessPolicy.browserEnabled && webView && webView.tabModel || null
        ignoreUnknownSignals: true
        // Animate overlay to top if needed.
        onCountChanged: {
            if (webView.tabModel.count === 0) {
                webView.handleModelChanges(false)
            }
            window.setBrowserCover(webView.tabModel)
        }
    }

    InputRegion {
        id: inputRegion

        window: webView.chromeWindow
        orientation: browserPage.orientation // Qt and Silica orientations match
        overlayMask: Qt.rect(0, 0, browserPage.width, browserPage.height)
        closeButtonMask: fullscreenClose.visible ? Qt.rect(fullscreenClose.x, fullscreenClose.y,
                                                           fullscreenClose.width, fullscreenClose.height)
                                                 : Qt.rect(0, 0, 0, 0)
    }

    MouseArea {
        width: browserPage.width
        height: Math.ceil(overlay.y)
        enabled: browserPage.chromeHostView && overlay.animator.atTop
                 && webView.tabModel.count > 0
        onClicked: overlay.dismiss(true)
    }

    Label {
        x: (browserPage.width - implicitWidth) / 2
        // Allow only half of the width
        width: parent.width / 2
        truncationMode: TruncationMode.Fade
        opacity: webView.privateMode && overlay.animator.atTop ? 1.0 : 0.0
        anchors {
            bottom: overlay.top
            bottomMargin: (overlay.toolBar.rowHeight - height) / 2
        }

        //: Label for private browsing above address bar
        //% "Private browsing"
        text: qsTrId("sailfish_browser-la-private_mode")
        color: Theme.highlightColor
        font.pixelSize: Theme.fontSizeLarge

        Behavior on opacity { FadeAnimation {} }
    }

    Browser.Overlay {
        id: overlay

        active: browserPage.status == PageStatus.Active && webView.tabModel.loaded
        webView: webView
        historyModel: historyModel
        browserPage: browserPage

        animator.onAtBottomChanged: {
            if (!animator.atBottom) {
                browserPage.clearSelection()
            }
        }

        onActiveChanged: {
            var isFullScreen = browserPage.contentFullscreen
            if (!isFullScreen && active && !overlay.enteringNewTabUrl) {
                if (webView.hasInitialUrl
                        || webView.tabModel.count !== 0) {
                    overlay.animator.showChrome()
                } else {
                    overlay.startPage()
                }
            }

            if (!active) {
                browserPage.clearSelection()
                if (webView.chromeWindow && webView.foreground) {
                    webView.chromeWindow.raise()
                }
            }
        }
    }

    Component {
        id: desktopBookmarkWriter_

        DesktopBookmarkWriter {
            onSaved: destroy()
        }
    }

    Browser.PopUpMenu {
        id: popupMenu

        width: parent.width
        height: parent.height

        active: overlay.toolBar.secondaryToolsActive
        menuItem: Component {
            Browser.PopUpMenuItem {
                desktopBookmarkWriter: desktopBookmarkWriter_
                iconWidth: Theme.iconSizeMedium + Theme.paddingLarge
            }
        }

        footer: Component {
            Browser.PopUpMenuFooter {
                hostedView: browserPage.chromeHostView
                height: Math.max(implicitHeight,
                                 (isPortrait ? overlay.toolBar.scaledPortraitHeight
                                             : overlay.toolBar.scaledLandscapeHeight)
                                 - popupMenu.verticalMargin)
            }
        }

        onClosed: overlay.dismiss(true)
    }

    CoverActionList {
        enabled: browserPage.status === PageStatus.Active
                 || browserPage.tabPageActive
                 || !webView.tabModel
                 || webView.tabModel.count === 0
        iconBackground: true
        window: webView.chromeWindow

        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: activateNewTabView()
        }
    }

    Connections {
        target: WebUtils
        onOpenUrlRequested: {
            // Refuse if blocked by MDM
            if (!AccessPolicy.browserEnabled) {
                bringToForeground(webView.chromeWindow)
                window.activate()
                return
            }

            // Url is empty when user tapped icon when browser was already open.
            // In case first use not done show the overlay immediately.
            if (url == "") {
                bringToForeground(webView.chromeWindow)
                if (!WebUtils.firstUseDone) {
                    overlay.enterNewTabUrl(PageStackAction.Immediate)
                }

                window.activate()
                return
            }

            if (browserPage.status !== PageStatus.Active) {
                pageStack.pop(browserPage, PageStackAction.Immediate)
            }

            if (loadInternalPage(url)) {
                bringToForeground(webView.chromeWindow)
                window.activate()
                return
            }

            if (webView.tabModel.activateTab(url)) {
                webView.releaseActiveTabOwnership()
            } else if (!webView.tabModel.loaded) {
                webView.load(url, false, true)
            } else {
                browserPage.clearSelection()
                webView.tabModel.newTab(url, true)
                overlay.dismiss(true, !Qt.application.active /* immediate */)
            }
            bringToForeground(webView.chromeWindow)
            window.activate()
        }
        onActivateNewTabViewRequested: activateNewTabView()
        onShowChrome: {
            pageStack.pop(browserPage, PageStackAction.Immediate)
            overlay.dismiss(true, !Qt.application.active /* immediate */)
            bringToForeground(webView.chromeWindow)
            window.activate()
        }
        onOpenSettingsRequested: {
            pageStack.pop(browserPage, PageStackAction.Immediate)
            pageStack.push(Qt.resolvedUrl("SettingsPage.qml"), {}, PageStackAction.Immediate)
            bringToForeground(webView.chromeWindow)
            window.activate()
        }
        onFirstUseDoneChanged: window.setBrowserCover(webView.tabModel)
    }

    Component.onCompleted: {
        if (chromeHostView) {
            webView.reportWindowOrientation(
                        webView._qtScreenOrientation(browserPage.orientation))
        }
        window.setBrowserCover(webView.tabModel)
        if (Qt.application.arguments.indexOf("-debugMode") > 0) {
            var component = Qt.createComponent(Qt.resolvedUrl("components/DebugOverlay.qml"))
            if (component.status === Component.Ready) {
                debug = component.createObject(browserPage)
            } else {
                console.warn("Failed to create DebugOverlay " + component.errorString())
            }
        }
    }
}
