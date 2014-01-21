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
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import Qt5Mozilla 1.0
import org.nemomobile.connectivity 1.0

WebContainer {
    id: webContainer

    // This cannot be bindings in multiple mozview case. Will change in
    // later commits.
    property bool active
    // This property should cover all possible popus
    property alias popupActive: webView._ctxMenuActive

    property alias loading: webView.loading
    property int loadProgress
    property alias contentItem: webView
    property TabModel tabModel
    property alias currentTab: tab
    readonly property bool fullscreenMode: (webView.chromeGestureEnabled && !webView.chrome) || webContainer.inputPanelVisible || !webContainer.foreground
    property alias canGoBack: tab.canGoBack
    property alias canGoForward: tab.canGoForward

    // Groupped properties
    property alias popups: webPopus

    // TODO : This must be encapsulated into a newTab / loadTab function
    // Load goes so that we first use engine load to resolve url
    // and then save that resolved url to the history. This way
    // urls that resolve to download urls won't get saved to the
    // history (as those won't trigger url change). As an added bonus
    // a redirected url will be saved with the redirected url not with
    // the input url.
    //
    // "newWebView" branch has some building blocks to help here.
    property var newTabData

    function goBack() {
        tab.goBack()
    }

    function goForward() {
        tab.goForward()
    }

    function stop() {
        webView.stop()
    }

    function reload() {
        var url = webView.url.toString()
        browserPage.url = url

        if (url.substring(0, 6) !== "about:" && url.substring(0, 5) !== "file:"
            && !browserPage._deferredReload
            && !connectionHelper.haveNetworkConnectivity()) {

            browserPage._deferredReload = true
            browserPage._deferredLoad = null
            connectionHelper.attemptToConnectNetwork()
            return
        }

        webView.reload()
    }

    function suspend() {
        webView.suspendView()
    }

    function resume() {
        webView.resumeView()
    }

    function sendAsyncMessage(name, data) {
        webView.sendAsyncMessage(name, data)
    }

    function captureScreen() {
        if (active && resourceController.firstFrameRendered) {
            var size = Screen.width
            if (browserPage.isLandscape && !webContainer.fullscreenMode) {
                size -= toolbarRow.height
            }

            tab.captureScreen(webView.url, 0, 0, size, size, browserPage.rotation)
        }
    }

    // Temporary functions / properties, remove once all functions have been moved
    property alias chrome: webView.chrome
    property alias resourceController: resourceController
    property alias connectionHelper: connectionHelper
    function load(url) {
        webView.load(url)
    }

    width: parent.width
    height: browserPage.orientation === Orientation.Portrait ? Screen.height : Screen.width

    pageActive: active
    webView: webView

    foreground: Qt.application.active
    inputPanelHeight: window.pageStack.panelSize
    inputPanelOpenHeight: window.pageStack.imSize
    toolbarHeight: toolBarContainer.height

    Rectangle {
        id: background
        anchors.fill: parent
        color: webView.bgcolor ? webView.bgcolor : "white"
    }

    Tab {
        id: tab

        // TODO: this will be internal of the WebView in newWebView branch.
        property bool backForwardNavigation: false

        onUrlChanged: {
            if (tab.valid && backForwardNavigation) {
                // Both url and title are updated before url changed is emitted.
                load(url, title)
            }
        }
    }

    QmlMozView {
        id: webView

        readonly property bool loaded: loadProgress === 100
        readonly property bool readyToLoad: viewReady && tabModel.loaded
        property bool userHasDraggedWhileLoading
        property bool viewReady

        property Item _contextMenu
        property bool _ctxMenuActive: _contextMenu != null && _contextMenu.active

        // As QML can't disconnect closure from a signal (but methods only)
        // let's keep auth data in this auxilary attribute whose sole purpose is to
        // pass arguments to openAuthDialog().
        property var _authData: null

        function openAuthDialog(input) {
            var data = input !== undefined ? input : webView._authData
            var winid = data.winid

            if (webView._authData !== null) {
                auxTimer.triggered.disconnect(webView.openAuthDialog)
                webView._authData = null
            }

            console.log("password component url: ", webPopus.passwordComponentUrl)

            var dialog = pageStack.push(webPopus.passwordComponentUrl,
                                        {
                                            "hostname": data.text,
                                            "realm": data.title,
                                            "username": data.defaultValue,
                                            "passwordOnly": data.passwordOnly
                                        })
            dialog.accepted.connect(function () {
                webView.sendAsyncMessage("authresponse",
                                         {
                                             "winid": winid,
                                             "accepted": true,
                                             "username": dialog.username,
                                             "password": dialog.password
                                         })
            })
            dialog.rejected.connect(function() {
                webView.sendAsyncMessage("authresponse",
                                         {"winid": winid, "accepted": false})
            })
        }

        function openContextMenu(linkHref, imageSrc, linkTitle, contentType) {
            var ctxMenuComp

            if (_contextMenu) {
                _contextMenu.linkHref = linkHref
                _contextMenu.linkTitle = linkTitle.trim()
                _contextMenu.imageSrc = imageSrc
                hideVirtualKeyboard()
                _contextMenu.show()
            } else {
                ctxMenuComp = Qt.createComponent(webPopus.contextMenuComponentUrl)
                if (ctxMenuComp.status !== Component.Error) {
                    _contextMenu = ctxMenuComp.createObject(browserPage,
                                                            {
                                                                "linkHref": linkHref,
                                                                "imageSrc": imageSrc,
                                                                "linkTitle": linkTitle.trim(),
                                                                "contentType": contentType,
                                                                "viewId": webView.uniqueID()
                                                            })
                    hideVirtualKeyboard()
                    _contextMenu.show()
                } else {
                    console.log("Can't load BrowserContextMenu.qml")
                }
            }
        }

        function hideVirtualKeyboard() {
            if (Qt.inputMethod.visible) {
                webContainer.parent.focus = true
            }
        }

        visible: WebUtils.firstUseDone

        enabled: parent.active
        // There needs to be enough content for enabling chrome gesture
        chromeGestureThreshold: toolBarContainer.height
        chromeGestureEnabled: contentHeight > webContainer.height + chromeGestureThreshold

        signal selectionRangeUpdated(variant data)
        signal selectionCopied(variant data)
        signal contextMenuRequested(variant data)

        focus: true
        width: browserPage.width
        state: ""

        onReadyToLoadChanged: {
            if (!visible) {
                return
            }

            if (WebUtils.initialPage !== "") {
                browserPage.load(WebUtils.initialPage)
            } else if (tabModel.count > 0) {
                // First tab is actived when tabs are loaded to the tabs model.
                browserPage.load(tab.url, tab.title)
            } else {
                browserPage.load(WebUtils.homePage)
            }
        }

        onLoadProgressChanged: {
            if (loadProgress > webContainer.loadProgress) {
                webContainer.loadProgress = loadProgress
            }
        }

        //{ // TODO
        // No resizes while page is not active
        // also contextmenu size
        //           if (browserPage.status == PageStatus.Active) {
        //               return (_contextMenu != null && (_contextMenu.height > tools.height)) ? browserPage.height - _contextMenu.height : browserPage.height - tools.height
        //               return (_contextMenu != null && (_contextMenu.height > tools.height)) ? 200 : 300

        // Order of onTitleChanged and onUrlChanged is unknown. Hence, use always browserPage.title and browserPage.url
        // as they are set in the load function of BrowserPage.
        onTitleChanged: {
            // This is always after url has changed
            browserPage.title = title
            tab.updateTab(browserPage.url, browserPage.title, "")
        }

        onUrlChanged: {
            browserPage.url = url

            if (!resourceController.isRejectedGeolocationUrl(url)) {
                resourceController.rejectedGeolocationUrl = ""
            }

            if (!resourceController.isAcceptedGeolocationUrl(url)) {
                resourceController.acceptedGeolocationUrl = ""
            }

            if (tab.backForwardNavigation) {
                tab.updateTab(browserPage.url, browserPage.title, "")
                tab.backForwardNavigation = false
            } else if (!newTabData) {
                // Use browserPage.title here to avoid wrong title to blink.
                // browserPage.load() updates browserPage's title before load starts.
                // QmlMozView's title is not correct over here.
                tab.navigateTo(browserPage.url)
            } else {
                // Delay adding of the new tab until url has been resolved.
                // Url will not change if there is download link behind.
                tabModel.addTab(browserPage.url, newTabData.foreground)
                if (newTabData.title) {
                    tab.title = newTabData.title
                }
            }

            newTabData = null
        }

        onBgcolorChanged: {
            var bgLightness = WebUtils.getLightness(bgcolor)
            var dimmerLightness = WebUtils.getLightness(Theme.highlightDimmerColor)
            var highBgLightness = WebUtils.getLightness(Theme.highlightBackgroundColor)

            if (Math.abs(bgLightness - dimmerLightness) > Math.abs(bgLightness - highBgLightness)) {
                verticalScrollDecorator.color = Theme.highlightDimmerColor
                horizontalScrollDecorator.color = Theme.highlightDimmerColor
            } else {
                verticalScrollDecorator.color = Theme.highlightBackgroundColor
                horizontalScrollDecorator.color = Theme.highlightBackgroundColor
            }

            sendAsyncMessage("Browser:SelectionColorUpdate",
                             {
                                 "color": Theme.secondaryHighlightColor
                             })
        }

        onViewInitialized: {
            addMessageListener("chrome:linkadded")
            addMessageListener("embed:alert")
            addMessageListener("embed:confirm")
            addMessageListener("embed:prompt")
            addMessageListener("embed:auth")
            addMessageListener("embed:login")
            addMessageListener("embed:permissions")
            addMessageListener("Content:ContextMenu")
            addMessageListener("Content:SelectionRange");
            addMessageListener("Content:SelectionCopied");
            addMessageListener("embed:selectasync")

            loadFrameScript("chrome://embedlite/content/SelectAsyncHelper.js")
            loadFrameScript("chrome://embedlite/content/embedhelper.js")

            viewReady = true
        }

        onDraggingChanged: {
            if (dragging && loading) {
                userHasDraggedWhileLoading = true
            }
        }

        onLoadedChanged: {
            if (loaded && !userHasDraggedWhileLoading) {
                webContainer.resetHeight(false)
            }
        }

        onLoadingChanged: {
            if (loading) {
                userHasDraggedWhileLoading = false
                favicon = ""
                webView.chrome = true
                webContainer.resetHeight(false)
            }
        }
        onRecvAsyncMessage: {
            switch (message) {
            case "chrome:linkadded": {
                if (data.rel === "shortcut icon") {
                    favicon = data.href
                }
                break
            }
            case "embed:selectasync": {
                var dialog

                dialog = pageStack.push(Qt.resolvedUrl("components/SelectDialog.qml"),
                                        {
                                            "options": data.options,
                                            "multiple": data.multiple,
                                            "webview": webView
                                        })
                break;
            }
            case "embed:alert": {
                var winid = data.winid
                var dialog = pageStack.push(Qt.resolvedUrl("components/AlertDialog.qml"),
                                            {"text": data.text})
                // TODO: also the Async message must be sent when window gets closed
                dialog.done.connect(function() {
                    sendAsyncMessage("alertresponse", {"winid": winid})
                })
                break
            }
            case "embed:confirm": {
                var winid = data.winid
                var dialog = pageStack.push(Qt.resolvedUrl("components/ConfirmDialog.qml"),
                                            {"text": data.text})
                // TODO: also the Async message must be sent when window gets closed
                dialog.accepted.connect(function() {
                    sendAsyncMessage("confirmresponse",
                                     {"winid": winid, "accepted": true})
                })
                dialog.rejected.connect(function() {
                    sendAsyncMessage("confirmresponse",
                                     {"winid": winid, "accepted": false})
                })
                break
            }
            case "embed:prompt": {
                var winid = data.winid
                var dialog = pageStack.push(Qt.resolvedUrl("components/PromptDialog.qml"),
                                            {"text": data.text, "value": data.defaultValue})
                // TODO: also the Async message must be sent when window gets closed
                dialog.accepted.connect(function() {
                    sendAsyncMessage("promptresponse",
                                     {
                                         "winid": winid,
                                         "accepted": true,
                                         "promptvalue": dialog.value
                                     })
                })
                dialog.rejected.connect(function() {
                    sendAsyncMessage("promptresponse",
                                     {"winid": winid, "accepted": false})
                })
                break
            }
            case "embed:auth": {
                if (pageStack.busy) {
                    // User has just entered wrong credentials and webView wants
                    // user's input again immediately even thogh the accepted
                    // dialog is still deactivating.
                    webView._authData = data
                    // A better solution would be to connect to browserPage.statusChanged,
                    // but QML Page transitions keep corrupting even
                    // after browserPage.status === PageStatus.Active thus auxTimer.
                    auxTimer.triggered.connect(webView.openAuthDialog)
                    auxTimer.start()
                } else {
                    webView.openAuthDialog(data)
                }
                break
            }
            case "embed:permissions": {
                // Ask for location permission
                if (resourceController.isAcceptedGeolocationUrl(webView.url)) {
                    sendAsyncMessage("embedui:premissions", {
                                         allow: true,
                                         checkedDontAsk: false,
                                         id: data.id })
                } else if (resourceController.isRejectedGeolocationUrl(webView.url)) {
                    sendAsyncMessage("embedui:premissions", {
                                         allow: false,
                                         checkedDontAsk: false,
                                         id: data.id })
                } else {
                    var dialog = pageStack.push(Qt.resolvedUrl("components/LocationDialog.qml"), {})
                    dialog.accepted.connect(function() {
                        sendAsyncMessage("embedui:premissions", {
                                             allow: true,
                                             checkedDontAsk: false,
                                             id: data.id })
                        resourceController.acceptedGeolocationUrl = WebUtils.displayableUrl(webView.url)
                        resourceController.rejectedGeolocationUrl = ""
                    })
                    dialog.rejected.connect(function() {
                        sendAsyncMessage("embedui:premissions", {
                                             allow: false,
                                             checkedDontAsk: false,
                                             id: data.id })
                        resourceController.rejectedGeolocationUrl = WebUtils.displayableUrl(webView.url)
                        resourceController.acceptedGeolocationUrl = ""
                    })
                }
                break
            }
            case "embed:login": {
                pageStack.push(Qt.resolvedUrl("components/PasswordManagerDialog.qml"),
                               {
                                   "webView": webView,
                                   "requestId": data.id,
                                   "notificationType": data.name,
                                   "formData": data.formdata
                               })
                break
            }
            case "Content:ContextMenu": {
                webView.contextMenuRequested(data)
                if (data.types.indexOf("image") !== -1 || data.types.indexOf("link") !== -1) {
                    openContextMenu(data.linkURL, data.mediaURL, data.linkTitle, data.contentType)
                }
                break
            }
            case "Content:SelectionRange": {
                webView.selectionRangeUpdated(data)
                break
            }
            }
        }
        onRecvSyncMessage: {
            // sender expects that this handler will update `response` argument
            switch (message) {
            case "Content:SelectionCopied": {
                webView.selectionCopied(data)

                if (data.succeeded) {
                    //% "Copied to clipboard"
                    notification.show(qsTrId("sailfish_browser-la-selection_copied"))
                }
                break
            }
            }
        }

        // We decided to disable "text selection" until we understand how it
        // should look like in Sailfish.
        // TextSelectionController {}

        Rectangle {
            id: verticalScrollDecorator

            width: 5
            height: webView.verticalScrollDecorator.size
            y: webView.verticalScrollDecorator.position
            anchors.right: parent ? parent.right: undefined
            color: Theme.highlightDimmerColor
            smooth: true
            radius: 2.5
            visible: webView.contentHeight > webView.height && !webView.pinching && !webView._ctxMenuActive
            opacity: webView.verticalScrollDecorator.moving ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { properties: "opacity"; duration: 400 } }
        }

        Rectangle {
            id: horizontalScrollDecorator
            width: webView.horizontalScrollDecorator.size
            height: 5
            x: webView.horizontalScrollDecorator.position
            y: browserPage.height - (fullscreenMode ? 0 : toolBarContainer.height) - height
            color: Theme.highlightDimmerColor
            smooth: true
            radius: 2.5
            visible: webView.contentWidth > webView.width && !webView.pinching && !webView._ctxMenuActive
            opacity: webView.horizontalScrollDecorator.moving ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { properties: "opacity"; duration: 400 } }
        }

        states: State {
            name: "boundHeightControl"
            when: webContainer.inputPanelVisible || !webContainer.foreground
            PropertyChanges {
                target: webView
                height: browserPage.height
            }
        }
    }

    Connections {
        target: tabModel

        onCountChanged: {
            if (tabModel.count === 0) {
                webContainer.newTabData = null
            }
        }

        onAboutToCloseActiveTab: {
            if (webView.loading) {
                webView.stop()
            }
        }

        onActiveTabClosed: {
            webContainer.newTabData = null
            if (tabModel.count > 0) {
                browserPage.title = tab.title
                browserPage.url = tab.url
                load(tab.url, tab.title)
            }
        }
    }

    ConnectionHelper {
        id: connectionHelper

        onNetworkConnectivityEstablished: {
            var url
            var title

            if (browserPage._deferredLoad) {
                url = browserPage._deferredLoad["url"]
                title = browserPage._deferredLoad["title"]
                browserPage._deferredLoad = null

                browserPage.load(url, title, true)
            } else if (browserPage._deferredReload) {
                browserPage._deferredReload = false
                webView.reload()
            }
        }

        onNetworkConnectivityUnavailable: {
            browserPage._deferredLoad = null
            browserPage._deferredReload = false
        }
    }

    ResourceController {
        id: resourceController
        webView: webView
        background: webContainer.background

        onWebViewSuspended: connectionHelper.closeNetworkSession()
        onFirstFrameRenderedChanged: {
            if (firstFrameRendered) {
                captureScreen()
            }
        }
    }

    Timer {
        id: auxTimer

        interval: 1000
    }

    QtObject {
        id: webPopus

        property string passwordComponentUrl
        property string contextMenuComponentUrl
    }

    Component.onDestruction: connectionHelper.closeNetworkSession()
}
