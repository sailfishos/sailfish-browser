/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/


import QtQuick 2.0
import Sailfish.Silica 1.0
import Qt5Mozilla 1.0
import Sailfish.Browser 1.0
import "components" as Browser


Page {
    id: browserPage

    property alias tabs: tabModel
    property alias favorites: favoriteModel
    property alias currentTabIndex: tabModel.currentTabIndex
    property bool fullscreenMode

    property string favicon
    property Component _controlPageComponent
    property Item _contextMenu
    property bool _ctxMenuActive: _contextMenu != null && _contextMenu.active
    property bool _ctxMenuVisible: _contextMenu != null && _contextMenu.visible
    // As QML can't disconnect closure from a signal (but methods only)
    // let's keep auth data in this auxilary attribute whose sole purpose is to
    // pass arguments to openAuthDialog().
    property variant _authData: null

    // Indicates that the url being loaded was entered from the current tab,
    // used to catch redirects so that the url can be updated in db.
    property bool loadingInitiatedByTab: false

    function newTab(link, foreground) {
        if (foreground) {
            if (webView.loading) {
                webView.stop()
            }
            tab.loadWhenTabChanges = true
        }
        tabModel.addTab(link, foreground)
    }

    function closeTab(index) {
        if (tabModel.count == 0) {
            return
        }
        if (tabModel.count > 0) {
            tab.loadWhenTabChanges = true
        }

        var tabIndex = index ? currentTabIndex : index
        tabModel.remove(tabIndex)
    }

    function load(url) {
        if (tabModel.count == 0) {
            newTab(url, true)
        }
        if (url !== "" && webView.url != url) {
            webView.load(url)
        }
    }

    function loadTab(index) {
        if (webView.loading) {
            webView.stop()
        }
        tab.loadWhenTabChanges = true;
        currentTabIndex = index
    }

    function deleteTabHistory() {
        historyModel.clear()
    }

    function captureScreen() {
        if (status == PageStatus.Active) {
            tab.captureScreen(webView.url, 0, 0, webView.width,
                              webView.width, window.screenRotation)
        }
    }

    function closeAllTabs() {
        tabModel.clear()
    }

    function openAuthDialog(input) {
        var data = input !== undefined ? input : browserPage._authData
        var winid = data.winid

        if (browserPage._authData !== null) {
            auxTimer.triggered.disconnect(browserPage.openAuthDialog)
            browserPage._authData = null
        }

        var dialog = pageStack.push(Qt.resolvedUrl("components/AuthDialog.qml"),
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

    function openContextMenu(linkHref, imageSrc) {
        var ctxMenuComp

        if (_contextMenu) {
            _contextMenu.linkHref = linkHref
            _contextMenu.imageSrc = imageSrc
            _contextMenu.show(browserPage)
        } else {
            ctxMenuComp = Qt.createComponent(Qt.resolvedUrl("components/BrowserContextMenu.qml"))
            if (ctxMenuComp.status !== Component.Error) {
                _contextMenu = ctxMenuComp.createObject(browserPage,
                                                        {
                                                            "linkHref": linkHref,
                                                            "imageSrc": imageSrc
                                                        })
                _contextMenu.show(browserPage)
            } else {
                console.log("Can't load BrowserContentMenu.qml")
            }
        }
    }

    function saveTab(url, title) {
        if (browserPage.loadingInitiatedByTab) {
            browserPage.loadingInitiatedByTab = false
            tab.updateTab(url, title, "")
        } else {
            tab.navigateTo(url, title, "")
        }
    }

    onStatusChanged: fullscreenMode = status < PageStatus.Active

    TabModel {
        id: tabModel
    }

    HistoryModel {
        id: historyModel

        tabId: tabModel.currentTabId
    }

    Tab {
        id: tab

        // Indicates whether the next url that is set to this Tab element will be loaded.
        // Used when new tabs are created, tabs are loaded, and with back and forward,
        // All of these actions load data asynchronously from the DB, and the changes
        // are reflected in the Tab element.
        property bool loadWhenTabChanges: false

        tabId: tabModel.currentTabId

        onUrlChanged: {
            if (loadWhenTabChanges) {
                loadWhenTabChanges = false
                browserPage.loadingInitiatedByTab = true
                load(url)
            }
        }
    }

    Browser.DownloadRemorsePopup { id: downloadPopup }

    QmlMozView {
        id: webView

        property real startY
        property real moveDelta
        readonly property real moveLimit: toolBarContainer.height
        readonly property bool active: browserPage.status == PageStatus.Active

        signal selectionRangeUpdated(variant data)
        signal selectionCopied(variant data)
        signal contextMenuRequested(variant data)

        function updateFullscreenMode() {
            if (controlArea.y < window.height - controlArea.height) return

            var offset = scrollableOffset.y
            var currentDelta = offset - startY
            if (Math.abs(currentDelta) < moveDelta) {
                startY = offset
            }

            if (currentDelta > moveLimit) {
                fullscreenMode = true
            } else if (currentDelta < -moveLimit) {
                fullscreenMode = false
            }
            moveDelta = Math.abs(currentDelta)
        }

        clip: true

        anchors {
            top: parent.top
            left: parent.left
        }
        focus: true
        width: browserPage.width
        height: browserPage.height
        //{ // TODO
        // No resizes while page is not active
        // also contextmenu size
        //           if (browserPage.status == PageStatus.Active) {
        //               return (_contextMenu != null && (_contextMenu.height > tools.height)) ? browserPage.height - _contextMenu.height : browserPage.height - tools.height
        //               return (_contextMenu != null && (_contextMenu.height > tools.height)) ? 200 : 300

        onTitleChanged: saveTab(url, title)

        onUrlChanged: saveTab(url, "")

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
            addMessageListener("Content:ContextMenu")
            addMessageListener("Content:SelectionRange");
            addMessageListener("Content:SelectionCopied");
            addMessageListener("embed:selectasync")

            loadFrameScript("chrome://embedlite/content/SelectAsyncHelper.js")
            loadFrameScript("chrome://embedlite/content/embedhelper.js")
            loadFrameScript("chrome://embedlite/content/StyleSheetHandler.js")

            if (WebUtils.initialPage !== "") {
                browserPage.load(WebUtils.initialPage)
            } else if (historyModel.count == 0 ) {
                browserPage.load(WebUtils.homePage)
            } else {
                browserPage.load(tab.url)
            }
        }

        onLoadingChanged: {
            if (loading) {
                favicon = ""
                fullscreenMode = false
            }

            // store tab data
            if (!loading && url != "about:blank" && url) {
                saveTab(url, title)
                captureScreen()
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
                    browserPage._authData = data
                    // A better solution would be to connect to browserPage.statusChanged,
                    // but QML Page transitions keep corrupting even
                    // after browserPage.status === PageStatus.Active thus auxTimer.
                    auxTimer.triggered.connect(browserPage.openAuthDialog)
                    auxTimer.start()
                } else {
                    browserPage.openAuthDialog(data)
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
                    openContextMenu(data.linkURL, data.mediaURL)
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
        onScrollableOffsetChanged: {
            // TabPage and ControlPage cannot trigger updates to viewport
            if (!active) return

            var contentRect = child.contentRect
            var offset = scrollableOffset
            var size = child.scrollableSize

            var ySizeRatio = contentRect.height / size.height
            var xSizeRatio = contentRect.width / size.width

            verticalScrollDecorator.height = height * ySizeRatio
            verticalScrollDecorator.y = offset.y * resolution * ySizeRatio

            horizontalScrollDecorator.width = width * xSizeRatio
            horizontalScrollDecorator.x = offset.x * resolution * xSizeRatio

            scrollTimer.restart()
        }

        onViewAreaChanged: {
            if (!active) return
            updateFullscreenMode()
        }

        onDraggingChanged: {
            if (dragging) {
                startY = scrollableOffset.y
                moveDelta = 0
            }
        }

        // We decided to disable "text selection" until we understand how it
        // should look like in Sailfish.
        // TextSelectionController {}

        Rectangle {
            id: verticalScrollDecorator

            width: 5
            anchors.right: parent ? parent.right: undefined
            color: Theme.highlightDimmerColor
            smooth: true
            radius: 2.5
            visible: parent.height > height && !_ctxMenuVisible
            opacity: scrollTimer.running ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { properties: "opacity"; duration: 400 } }
        }

        Rectangle {
            id: horizontalScrollDecorator
            height: 5
            y: parent.height - (fullscreenMode ? 0 : toolBarContainer.height) - height
            color: Theme.highlightDimmerColor
            smooth: true
            radius: 2.5
            visible: parent.width > width && !_ctxMenuVisible
            opacity: scrollTimer.running ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { properties: "opacity"; duration: 400 } }
        }

        Timer {
            id: scrollTimer

            interval: 300
        }
    }

    // Dimmer for web content
    Rectangle {
        anchors.fill: webView

        color: Theme.highlightDimmerColor
        opacity: _ctxMenuActive? 0.8 : 0.0
        Behavior on opacity { FadeAnimation {} }
    }

    Column {
        id: controlArea

        // This should be just a binding for progressBar.progress but currently progress is going up and down
        property real loadProgress: webView.loadProgress / 100.0

        y: parent.height - height
        width: parent.width
        visible: !_ctxMenuActive
        opacity: fullscreenMode ? 0.0 : 1.0
        Behavior on opacity { FadeAnimation { duration: 300 } }

        onLoadProgressChanged: {
            if (loadProgress > progressBar.progress) {
                progressBar.progress = loadProgress
            }
        }

        function openControlPage() {
            captureScreen()
            var sendUrl = (webView.url != WebUtils.initialPage) ? webView.url : ""
            pageStack.push(_controlPageComponent, {
                               historyModel: historyModel,
                               url: sendUrl,
                               title: webView.title
                           })
        }

        Browser.StatusBar {
            width: parent.width
            height: visible ? toolBarContainer.height * 2 : 0
            opacity: progressBar.opacity
            title: webView.title
            url: webView.url
        }

        Browser.ToolBarContainer {
            id: toolBarContainer
            width: parent.width

            Browser.ProgressBar {
                id: progressBar
                anchors.fill: parent
                opacity: webView.loading ? 1.0 : 0.0
            }

            // ToolBar
            Row {
                anchors {
                    left: parent.left; leftMargin: Theme.paddingMedium
                    right: parent.right; rightMargin: Theme.paddingMedium
                    verticalCenter: parent.verticalCenter
                }
                // 5 icons, 4 spaces between
                spacing: (width - (backIcon.width * 5)) / 4

                IconButton {
                    id:backIcon
                    icon.source: "image://theme/icon-m-back"
                    enabled: tab.canGoBack && !fullscreenMode
                    onClicked: {
                        tab.loadWhenTabChanges = true
                        tab.goBack()
                    }
                }

                IconButton {
                    icon.source: "image://theme/icon-m-search"
                    enabled: !fullscreenMode
                    onClicked: controlArea.openControlPage()
                }

                IconButton {
                    icon.source: "image://theme/icon-m-tabs"
                    enabled: !fullscreenMode
                    onClicked: {
                        captureScreen()
                        pageStack.push(Qt.resolvedUrl("TabPage.qml"), {"browserPage" : browserPage})
                    }
                }
                IconButton {
                    enabled: !fullscreenMode
                    icon.source: webView.loading ? "image://theme/icon-m-reset" : "image://theme/icon-m-refresh"
                    onClicked: webView.loading ? webView.stop() : webView.reload()
                }

                IconButton {
                    icon.source: "image://theme/icon-m-forward"
                    enabled: tab.canGoForward && !fullscreenMode
                    onClicked: {
                        tab.loadWhenTabChanges = true
                        tab.goForward()
                    }
                }
            }
        }
    }

    CoverActionList {
        iconBackground: true

        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: {
                controlArea.openControlPage()
                activate()
            }
        }

        CoverAction {
            iconSource: "image://theme/icon-cover-refresh"
            onTriggered: {
                if (webView.loading) {
                    webView.stop()
                }
                webView.reload()
            }
        }
    }

    Connections {
        target: WebUtils
        onOpenUrlRequested: {
            if (webView.url != "") {
                captureScreen()
                if (!tabs.activateTab(url)) {
                    // Not found in tabs list, create newtab and load
                    newTab(url, true)
                }
            } else {
                // New browser instance, just load the content
                load(url)
            }
            if (status != PageStatus.Active) {
                pageStack.pop(browserPage, PageStackAction.Immediate)
            }
            if (!window.applicationActive) {
                window.activate()
            }
        }
    }

    Component.onCompleted: {
        // Since we dont have booster with gecko yet (see JB#5910) lets compile the
        // components needed by tab page here so that click on tab icon wont be too long
        if (!_controlPageComponent) {
            _controlPageComponent = Qt.createComponent("ControlPage.qml")
            if (_controlPageComponent.status !== Component.Ready) {
                console.log("Error loading component:", component.errorString());
                _controlPageComponent = undefined
                return
            }
        }
    }

    BookmarkModel {
        id: favoriteModel
    }

    Timer {
        id: auxTimer

        interval: 1000
    }

    Browser.BrowserNotification {
        id: notification
    }
}
