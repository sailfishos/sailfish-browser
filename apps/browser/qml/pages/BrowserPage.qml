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
import Sailfish.WebView.Popups 1.0 as Popups
import Nemo.Configuration 1.0
import "components" as Browser
import "../shared" as Shared

Page {
    id: browserPage

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
    readonly property bool chromeHostMode: !!chromeHostLoader.item && !webView.privateMode
    readonly property var chromeHostView: chromeHostMode ? chromeHostLoader.item : null
    readonly property var _runtimeChromeView: chromeHostLoader.item
    readonly property bool viewLoading: chromeHostView ? chromeHostView.loading : webView.loading
    readonly property int loadProgress: chromeHostView ? chromeHostView.loadProgress : webView.loadProgress
    readonly property string url: chromeHostView ? String(chromeHostView.url) : webView.url
    readonly property string title: chromeHostView ? chromeHostView.title : webView.title
    property bool _runtimeRestoreSent
    property bool _runtimeSnapshotInitialized
    property var _runtimeAppliedState: ({})
    property var _pendingRuntimeTitles: ({})
    property var _pendingRuntimeCommands: []
    property var _pendingRuntimeNavigation: null
    property alias webView: webView
    property alias inputRegion: inputRegion

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
            if (chromeHostView.selectedTabId.length) {
                chromeHostView.load(url, false)
            } else {
                webView.persistentTabModel.newTab(url, false)
            }
        } else {
            webView.load(url, title)
        }
    }

    function newTab(url, fromExternal) {
        if (chromeHostView) {
            return webView.persistentTabModel.newTab(url, !!fromExternal)
        }
        return webView.tabModel.newTab(url, !!fromExternal)
    }

    function goBack() {
        if (chromeHostView) {
            var persistentId = selectedPersistentId()
            if (persistentId.length
                    && webView.persistentTabModel.runtimeGoBack(persistentId)) {
                chromeHostView.goBack()
            }
        } else {
            webView.goBack()
        }
    }

    function goForward() {
        if (chromeHostView) {
            var persistentId = selectedPersistentId()
            if (persistentId.length
                    && webView.persistentTabModel.runtimeGoForward(persistentId)) {
                chromeHostView.goForward()
            }
        } else {
            webView.goForward()
        }
    }

    function stop() {
        if (chromeHostView) {
            chromeHostView.stop()
        } else {
            webView.stop()
        }
    }

    function reload() {
        if (chromeHostView) {
            chromeHostView.reload()
        } else {
            webView.reload()
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

    function restoreRuntimeTabs(hostView) {
        if (!hostView || _runtimeRestoreSent
                || !webView.persistentTabModel.loaded) {
            return
        }

        var restoreBatch = webView.persistentTabModel.runtimeRestoreBatch()
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
        var pendingTabs = webView.persistentTabModel.takePendingRuntimeNewTabs()
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

    function dispatchRuntimeCommand(command, runtimeHostView) {
        var hostView = runtimeHostView || _runtimeChromeView
        if (!hostView || !_runtimeRestoreSent || !_runtimeSnapshotInitialized) {
            queueRuntimeCommand(command)
            return
        }

        if (command.type === "new") {
            cancelPendingRuntimeNavigation()
            if (!hostView.newTab(command.url, command.persistentId,
                                 command.fromExternal, false)) {
                webView.persistentTabModel.cancelRuntimeTabReservation(
                            command.persistentId)
            }
        } else if (command.type === "activate") {
            cancelPendingRuntimeNavigation()
            var runtimeId = webView.persistentTabModel.runtimeIdForPersistentId(
                        command.persistentId)
            if (runtimeId.length && hostView.selectTab(runtimeId) && command.reload) {
                hostView.reload()
            }
        } else if (command.type === "close") {
            cancelPendingRuntimeNavigation()
            var closeRuntimeId = webView.persistentTabModel.runtimeIdForPersistentId(
                        command.persistentId)
            if (closeRuntimeId.length) {
                hostView.closeTab(closeRuntimeId)
            }
        } else if (command.type === "navigate") {
            cancelPendingRuntimeNavigation()
            var navigateRuntimeId = webView.persistentTabModel.runtimeIdForPersistentId(
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
            cancelPendingRuntimeNavigation()
            var tabs = hostView.tabModel.snapshot()
            for (var index = tabs.length - 1; index >= 0; --index) {
                hostView.closeTab(String(tabs[index].tabId))
            }
        }
    }

    function flushSelectedRuntimeNavigation(runtimeHostView) {
        var hostView = runtimeHostView || _runtimeChromeView
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

        if (!webView.persistentTabModel.runtimeIdForPersistentId(
                    command.persistentId).length) {
            cancelPendingRuntimeNavigation()
        }
    }

    function flushRuntimeCommands(runtimeHostView) {
        var hostView = runtimeHostView || _runtimeChromeView
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

    function refreshRuntimeHistory() {
        var search = overlay.searchField.text === browserPage.url
                ? "" : overlay.searchField.text
        historyModel.search(search)
    }

    function applyRuntimeSnapshot(acceptDeferredTitles, runtimeHostView) {
        var hostView = runtimeHostView || _runtimeChromeView
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
        webView.persistentTabModel.applyRuntimeSnapshot(
                    snapshot, hostView.selectedTabId)
        if (historyChanged) {
            // PersistentTabModel is the sole writer. Queueing a search after
            // its snapshot update refreshes the live History UI without
            // incrementing the visit count a second time.
            refreshRuntimeHistory()
        }
        _runtimeSnapshotInitialized = true
        flushRuntimeCommands(hostView)
        flushSelectedRuntimeNavigation(hostView)

        if (chromeHostMode && snapshot.length === 0 && browserPage.active) {
            overlay.startPage()
        }
    }

    Timer {
        id: runtimeTitlePairingTimer

        interval: 120
        onTriggered: browserPage.applyRuntimeSnapshot(true)
    }

    Timer {
        id: runtimeNavigationTimer

        interval: 1000
        onTriggered: browserPage.cancelPendingRuntimeNavigation()
    }

    Timer {
        id: runtimeNewTabDrainTimer

        interval: 0
        onTriggered: browserPage.drainRuntimeNewTabs()
    }

    function bringToForeground(window) {
        if ((webView.visibility < QuickWindow.Window.Maximized) && window) {
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
    onStatusChanged: {
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

    orientationTransitions: orientationFader.orientationTransition

    Keys.onPressed: {
        webView.handleKeyPress(event.key)
    }

    Shared.OrientationFader {
        id: orientationFader

        visible: webView.contentItem
        page: browserPage
        fadeTarget: overlay.animator.allowContentUse ? overlay : overlay.dragArea
        color: webView.contentItem ? (webView.resourceController.videoActive
                                      && webView.contentItem.fullscreen
                                      ? "black" : webView.contentItem.backgroundColor)
                                   : "white"

        onApplyContentOrientation: webView.applyContentOrientation(browserPage.orientation)
    }

    HistoryModel {
        id: historyModel
    }

    Private.VirtualKeyboardObserver {
        id: virtualKeyboardObserver

        active: webView.enabled || browserPage.chromeHostMode
        transpose: window._transpose
        orientation: browserPage.orientation

        onWindowChanged: webView.chromeWindow = window

        // Update content height only after virtual keyboard fully opened.
        states: State {
            name: "boundHeightControl"
            when: virtualKeyboardObserver.opened && webView.enabled
            PropertyChanges {
                target: webView.contentItem
                virtualKeyboardHeight: virtualKeyboardObserver.imSize
            }
        }
    }

    ConfigurationValue {
        id: maxliveTabs

        key: "/apps/sailfish-browser/settings/max_live_tab_count"
        defaultValue: 3
    }

    Browser.DownloadRemorsePopup { id: downloadPopup }

    Shared.WebView {
        id: webView

        // The chrome-hosted view lives in this QQuickWindow, so keep
        // the full window input region on this window instead of forwarding
        // content-area input to the legacy web-content window underneath.
        enabled: !browserPage.chromeHostMode && overlay.animator.allowContentUse
        fullscreenHeight: portrait ? Screen.height : Screen.width
        portrait: browserPage.isPortrait
        maxLiveTabCount: maxliveTabs.value
        toolbarHeight: overlay.animator.opened ? overlay.toolBar.rowHeight : 0
        rotationHandler: browserPage
        imOpened: virtualKeyboardObserver.opened
        canShowSelectionMarkers: !orientationFader.waitForWebContentOrientationChanged
        historyModel: historyModel

        // Show overlay immediately at top if needed.
        onTabModelChanged: handleModelChanges(true)

        onChromeExposed: {
            if (overlay.animator.atTop && overlay.searchField.focus && !WebUtils.firstUseDone) {
                webView.chromeWindow.raise()
            }
        }

        onForegroundChanged: {
            if (foreground && webView.chromeWindow) {
                webView.chromeWindow.raise()
            }
        }

        onTouched: {
            if (contentFullscreen) {
                fullscreenCloseVisibleTimer.restart()
            }
        }

        onWebContentOrientationChanged: orientationFader.waitForWebContentOrientationChanged = false

        function applyContentOrientation(orientation) {
            orientationFader.waitForWebContentOrientationChanged = (contentItem && contentItem.active)

            switch (orientation) {
            case Orientation.None:
            case Orientation.Portrait:
                updateContentOrientation(Qt.PortraitOrientation)
                break
            case Orientation.Landscape:
                updateContentOrientation(Qt.LandscapeOrientation)
                break
            case Orientation.PortraitInverted:
                updateContentOrientation(Qt.InvertedPortraitOrientation)
                break
            case Orientation.LandscapeInverted:
                updateContentOrientation(Qt.InvertedLandscapeOrientation)
                break
            }
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

    Loader {
        id: chromeHostLoader

        anchors.fill: parent
        active: webView.persistentTabModel.loaded
                && webView.persistentTabModel.runtimeAuthoritative
        onLoaded: {
            browserPage.restoreRuntimeTabs(item)
            // The initial empty snapshot can be replayed synchronously while
            // the Loader is publishing its item, before Connections can see
            // the model's revision change.
            browserPage.applyRuntimeSnapshot(false, item)
        }
        sourceComponent: Component {
            QmlMozView {
                id: chromeView

                property bool _qmozChromeHosted: true
                property string _qmozChromeInitialUrl: ""
                property QtObject popupOpener: Popups.PopupOpener {
                    pageStack: window.pageStack
                    parentItem: browserPage
                    contentItem: chromeView
                    tabModel: webView.tabModel
                }

                anchors.fill: parent
                active: browserPage.active && !webView.privateMode
                clip: true
                focus: true
                visible: !webView.privateMode

                onRecvAsyncMessage: {
                    if (popupOpener.message(message, data)) {
                        return
                    }
                }

                Connections {
                    target: chromeView.tabModel
                    ignoreUnknownSignals: true
                    onRevisionChanged: browserPage.applyRuntimeSnapshot(false,
                                                                         chromeView)
                }
            }
        }
    }

    Connections {
        target: webView.persistentTabModel
        // The persistent model records the recoverable command in another
        // handler for this signal. Drain on the next event-loop turn so that
        // state is visible regardless of connection ordering.
        onRuntimeNewTabRequested: runtimeNewTabDrainTimer.restart()
        onRuntimeTabActivationRequested: browserPage.dispatchRuntimeCommand({
            "type": "activate",
            "persistentId": persistentId,
            "reload": reload
        })
        onRuntimeTabCloseRequested: browserPage.dispatchRuntimeCommand({
            "type": "close",
            "persistentId": persistentId
        })
        onRuntimeTabNavigationRequested: browserPage.dispatchRuntimeCommand({
            "type": "navigate",
            "persistentId": persistentId,
            "url": url,
            "fromExternal": fromExternal
        })
        onRuntimeTabsClearRequested: browserPage.dispatchRuntimeCommand({
            "type": "clear"
        })
        onRuntimeTabAdopted: {
            if (browserPage._runtimeChromeView) {
                browserPage._runtimeChromeView.associateTab(runtimeId, persistentId)
            }
        }
        onRuntimeTabReservationRejected: {
            browserPage.removeQueuedRuntimeNewTab(persistentId)
            if (browserPage._pendingRuntimeNavigation
                    && browserPage._pendingRuntimeNavigation.persistentId
                    === persistentId) {
                browserPage.cancelPendingRuntimeNavigation()
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
        onClicked: {
            webView.sendAsyncMessage("embedui:exitFullscreen", {})
        }

        Timer {
            id: fullscreenCloseVisibleTimer

            interval: 2000
            running: webView.contentFullscreen
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

        window: browserPage.chromeHostMode
                ? (virtualKeyboardObserver.window || null) : webView.chromeWindow
        orientation: browserPage.orientation // Qt and Silica orientations match
        overlayMask: (webView.enabled && browserPage.active && !webView.touchBlocked && !downloadPopup.visible)
                     ? Qt.rect(0, overlay.y, browserPage.width, browserPage.height - overlay.y)
                     : Qt.rect(0, 0, browserPage.width, browserPage.height)
        closeButtonMask: fullscreenClose.visible ? Qt.rect(fullscreenClose.x, fullscreenClose.y,
                                                           fullscreenClose.width, fullscreenClose.height)
                                                 : Qt.rect(0, 0, 0, 0)
    }

    Browser.DimmerEffect {
        id: contentDimmer

        width: browserPage.width
        height: Math.ceil(overlay.y)

        dimmerOpacity: overlay.animator.atBottom
                       ? 0.0
                       : 0.9 - (overlay.y / (webView.fullscreenHeight - overlay.toolBar.rowHeight)) * 0.9

        MouseArea {
            property bool inEmptyPrivateMode: webView.privateMode && webView.privateTabModel.count === 0
                                              && webView.persistentTabModel.count > 0

            anchors.fill: parent
            enabled: overlay.animator.atTop
                     && (webView.tabModel.count > 0 || inEmptyPrivateMode)
            onClicked: {
                if (inEmptyPrivateMode) {
                    webView.privateMode = false
                    //% "Leaving private mode"
                    Notices.show(qsTrId("sailfish_browser-la-leaving_private_mode"), Notice.Short, Notice.Top)
                }
                overlay.dismiss(true)
            }
        }

        Browser.PrivateModeTexture {
            id: privateModeTexture

            anchors.fill: contentDimmer
            visible: webView.privateMode && !overlay.animator.allowContentUse
        }
    }

    Label {
        x: (contentDimmer.width - implicitWidth) / 2
        // Allow only half of the width
        width: parent.width / 2
        truncationMode: TruncationMode.Fade
        opacity: privateModeTexture.visible ? 1.0 : 0.0
        anchors {
            bottom: contentDimmer.bottom
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
                webView.clearSelection()
            }
        }

        onActiveChanged: {
            var isFullScreen = webView.contentItem && webView.contentItem.fullscreen
            if (!isFullScreen && active && !overlay.enteringNewTabUrl) {
                if (webView.hasInitialUrl
                        || webView.tabModel.count !== 0
                        || (!browserPage.chromeHostMode
                            && WebUtils.homePage !== "about:blank"
                            && WebUtils.homePage.length > 0)) {
                    overlay.animator.showChrome()
                } else {
                    overlay.startPage()
                }
            }

            if (!active) {
                webView.clearSelection()
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
        window: webView

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

            if (!browserPage.chromeHostMode) {
                webView.grabActivePage()
            }
            if (webView.tabModel.activateTab(url)) {
                if (!browserPage.chromeHostMode) {
                    webView.releaseActiveTabOwnership()
                }
            } else if (!webView.tabModel.loaded) {
                if (browserPage.chromeHostMode) {
                    browserPage.newTab(url, true)
                } else {
                    webView.load(url)
                }
            } else {
                webView.clearSelection()
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
