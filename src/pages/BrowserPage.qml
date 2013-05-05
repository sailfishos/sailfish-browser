/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/


import QtQuick 1.1
import Sailfish.Silica 1.0
import QtMozilla 1.0
import Sailfish.Browser 1.0
import "components"

import "history.js" as History

Page {
    id: browserPage

    property alias tabs: tabModel
    property alias favorites: favoriteModel
    property int currentTabIndex
    property variant webEngine: webContent.child
    property string favicon

    property variant _controlPageComponent
    property Item _contextMenu
    property bool _ctxMenuActive: _contextMenu && _contextMenu.active
    property bool _ctxMenuVisible: _contextMenu && _contextMenu.visible
    // As QML can't disconnect closure from a signal (but methods only)
    // let's keep auth data in this auxilary attribute whose sole purpose is to
    // pass arguments to openAuthDialog().
    property variant _authData: null


    function newTab() {
        var id = History.addTab("","")
        historyModel.clear()
        tabModel.append({"thumbPath": "", "url": "", "tabId":id})
        currentTabIndex = tabModel.count - 1
    }

    function closeTab() {
        if (tabModel.count == 0) {
            return
        }
        History.deleteTab(tabModel.get(currentTabIndex).tabId)
        tabModel.remove(currentTabIndex)
        currentTabIndex = tabModel.count - 1
        historyModel.clear()
        if (tabModel.count >= 1) {
            History.loadTabHistory(tabModel.get(currentTabIndex).tabId, historyModel)
            load(tabModel.get(currentTabIndex).url)
        }
    }

    function load(url) {
        if (tabModel.count == 0) {
            newTab()
        }
        if (!webEngine || webEngine.url == null) {
            console.log("No webengine")
        } else if (webEngine.url !== url) {
            webEngine.load(url)
        }
    }

    function loadTab(index) {
        if (currentTabIndex !== index) {
            currentTabIndex = index
            historyModel.clear()
            load(tabModel.get(currentTabIndex).url)
            History.loadTabHistory(tabModel.get(currentTabIndex).tabId, historyModel)
        }
    }

    function deleteTabHistory() {
        historyModel.clear()
        History.deleteTabHistory(tabModel.get(currentTabIndex).tabId)
    }

    function storeTab() {
        var screenPath = ""
        if (status == PageStatus.Active) {
            screenPath = BrowserTab.screenCapture(0, 0, webContent.width, webContent.width, window.screenRotation)
        }
        tabModel.set(currentTabIndex, {"thumbPath" : screenPath, "url" : webEngine.url})
        History.updateTab(tabModel.get(currentTabIndex).tabId, webEngine.url, screenPath)
    }

    function closeAllTabs() {
        historyModel.clear()
        History.deleteAllTabs()
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
            webEngine.sendAsyncMessage("authresponse",
                                       {
                                           "winid": winid,
                                           "accepted": true,
                                           "username": dialog.username,
                                           "password": dialog.password
                                       })
        })
        dialog.rejected.connect(function() {
            webEngine.sendAsyncMessage("authresponse",
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

    ListModel {
        id: historyModel
    }

    BookmarkModel {
        id: favoriteModel
    }

    ListModel {
        id:tabModel
    }

    QmlMozView {
        id: webContent
        anchors {
            top: parent.top
            left: parent.left
        }
        focus: true
        width: browserPage.width
        enabled: !_ctxMenuActive

        height: {
            // No resizes while page is not active
            // workaround for engine crashes on resizes while background
            if (browserPage.status == PageStatus.Active) {
                return _contextMenu && (_contextMenu.height > tools.height) ? browserPage.height - _contextMenu.height : browserPage.height - tools.height
            } else {
                return screen.height - tools.height
            }
        }

        Connections {
            target: webEngine

            onTitleChanged: {
                // Update title in model, title can come after load finished
                // and then we already have element in history
                if (historyModel.count > 0
                        && historyModel.get(0).url == webEngine.url
                        && webEngine.title !== historyModel.get(0).title ) {
                    historyModel.setProperty(0, "title", webEngine.title)
                }
            }

            onViewInitialized: {
                webEngine.addMessageListener("chrome:linkadded")
                webEngine.addMessageListener("embed:alert");
                webEngine.addMessageListener("embed:confirm");
                webEngine.addMessageListener("embed:prompt");
                webEngine.addMessageListener("embed:auth")
                webEngine.addMessageListener("context:info")

                webEngine.addMessageListener("embed:select") // this is sync message!

                webEngine.loadFrameScript("chrome://embedlite/content/SelectHelper.js");
                webEngine.loadFrameScript("chrome://embedlite/content/embedhelper.js");

                if (WebUtils.initialPage !== "") {
                    browserPage.load(WebUtils.initialPage)
                } else if (historyModel.count == 0 ) {
                    browserPage.load(WebUtils.homePage)
                } else {
                    browserPage.load(historyModel.get(0).url)
                }
            }
            onLoadingChanged: {
                progressBar.opacity = webEngine.loading ? 1.0 : 0.0
                if (!webEngine.loading) {
                    progressBar.progress = 0
                } else {
                    favicon = ""
                }

                if (!webEngine.loading && webEngine.url != "about:blank" &&
                    (historyModel.count == 0 || webEngine.url != historyModel.get(0).url)) {
                    var screenPath = ""
                    if (status == PageStatus.Active) {
                        screenPath = BrowserTab.screenCapture(0, 0, webContent.width, webContent.width, window.screenRotation)
                    }
                    History.addUrl(webEngine.url, webEngine.title, screenPath, tabModel.get(currentTabIndex).tabId)
                    historyModel.insert(0, {"title": webEngine.title, "url": webEngine.url, "icon": screenPath} )
                }
            }
            onLoadProgressChanged: {
                if ((webEngine.loadProgress / 100.0) > progressBar.progress) {
                    progressBar.progress = webEngine.loadProgress / 100.0
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
                    case "embed:alert": {
                        var winid = data.winid
                        var dialog = pageStack.push(Qt.resolvedUrl("components/AlertDialog.qml"),
                                                    {"text": data.text})
                        // TODO: also the Async message must be sent when window gets closed
                        dialog.done.connect(function() {
                            webEngine.sendAsyncMessage("alertresponse", {"winid": winid})
                        })
                        break
                    }
                    case "embed:confirm": {
                        var winid = data.winid
                        var dialog = pageStack.push(Qt.resolvedUrl("components/ConfirmDialog.qml"),
                                                    {"text": data.text})
                        // TODO: also the Async message must be sent when window gets closed
                        dialog.accepted.connect(function() {
                            webEngine.sendAsyncMessage("confirmresponse",
                                                       {"winid": winid, "accepted": true})
                        })
                        dialog.rejected.connect(function() {
                            webEngine.sendAsyncMessage("confirmresponse",
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
                            webEngine.sendAsyncMessage("promptresponse",
                                                       {
                                                           "winid": winid,
                                                           "accepted": true,
                                                           "promptvalue": dialog.value
                                                       })
                        })
                        dialog.rejected.connect(function() {
                            webEngine.sendAsyncMessage("promptresponse",
                                                       {"winid": winid, "accepted": false})
                        })
                        break
                    }
                    case "embed:auth": {
                        if (pageStack.busy) {
                            // User has just entered wrong credentials and webEngine wants
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
                    case "context:info": {
                        openContextMenu(data.LinkHref, data.ImageSrc)
                        break
                    }
                }
            }
            onRecvSyncMessage: {
                // sender expects that this handler will update `response` argument
                switch (message) {
                    case "embed:select": {
                        var dialog

                        dialog = pageStack.push(Qt.resolvedUrl("components/SelectDialog.qml"),
                                                {
                                                    "allItems": data.listitems,
                                                    "selectedItems": data.selected,
                                                    "multiple": data.multiple
                                                })
                        // HACK: block until dialog is closed
                        while (dialog.locked) {
                            WebUtils.processEvents()
                        }
                        response.message = {
                            button: dialog.selected
                        }
                        break;
                    }
                }
            }
            onViewAreaChanged: {
                var contentRect = webEngine.contentRect
                var offset = webEngine.scrollableOffset
                var size = webEngine.scrollableSize
                var resolution = webEngine.resolution

                var ySizeRatio = contentRect.height / size.height
                var xSizeRatio = contentRect.width / size.width

                verticalScrollDecorator.height = height * ySizeRatio
                verticalScrollDecorator.y = offset.y * resolution * ySizeRatio

                horizontalScrollDecorator.width = width * xSizeRatio
                horizontalScrollDecorator.x = offset.x * resolution * xSizeRatio

                scrollTimer.restart()
            }
        }

        Rectangle {
            id: verticalScrollDecorator

            width: 5
            anchors.right: parent ? parent.right: undefined
            color: theme.highlightDimmerColor
            border.width: 1
            border.color: "grey"
            smooth: true
            radius: 2.5
            visible: parent.height > height && !_ctxMenuVisible
            opacity: scrollTimer.running ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { properties: "opacity"; duration: 400 } }
        }

        Rectangle {
            id: horizontalScrollDecorator

            height: 5
            anchors.bottom: parent ? parent.bottom: undefined
            color: theme.highlightDimmerColor
            border.width: 1
            border.color: "grey"
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
        anchors.fill: webContent
        color: theme.highlightDimmerColor
        opacity: 0.8
        visible: _ctxMenuActive
    }

    Rectangle {
        anchors {
            left: tools.left
            right: tools.right
            bottom: tools.top
        }
        height: tools.height * 2
        opacity: progressBar.opacity

        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(1.0, 1.0, 1.0, 0.0) }
            GradientStop { position: 1.0; color: theme.highlightDimmerColor }
        }

        Column {
            width: parent.width
            anchors {
                bottom: parent.bottom; bottomMargin: theme.paddingMedium
            }

            Label {
                text: webEngine.title
                width: parent.width - theme.paddingMedium * 2
                color: theme.highlightColor
                font.pixelSize: theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
                truncationMode: TruncationMode.Fade
            }
            Label {
                text: webEngine.url
                width: parent.width - theme.paddingMedium * 2
                color: theme.secondaryColor
                font.pixelSize: theme.fontSizeExtraSmall
                horizontalAlignment: Text.AlignHCenter
                truncationMode: TruncationMode.Fade
            }
        }
    }

    Item {
        id: tools
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: visible ? theme.itemSizeMedium : 0
        visible: (parent.height === screen.height) && !(_contextMenu && theme.itemSizeMedium < _contextMenu.height)

        ProgressBar {
            id: progressBar
            anchors.fill: parent
            opacity: 0.0
        }

        Row {
            id: toolsrow
            anchors.fill: parent
            // 5 icons, 4 spaces between
            spacing: (width - (backIcon.width * 5)) / 4

            IconButton {
                id:backIcon
                icon.source: "image://theme/icon-m-back"
                enabled: webEngine.canGoBack
                onClicked: webEngine.goBack()
            }

            IconButton {
                icon.source: favoriteModel.count > 0 && favoriteModel.contains(webEngine.url) ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"
                enabled: true
                onClicked: {
                    if (favoriteModel.contains(webEngine.url)) {
                        favoriteModel.removeBookmark(webEngine.url)
                    } else {
                        favoriteModel.addBookmark(webEngine.url, webEngine.title, favicon)
                    }
                }
            }

            IconButton {
                icon.source: "image://theme/icon-m-tab"

                onClicked:  {
                    storeTab()
                    var sendUrl = (webEngine.url != WebUtils.initialPage) ? webEngine.url : ""
                    pageStack.push(_controlPageComponent, {historyModel: historyModel, url: sendUrl}, true)
                }
            }
            IconButton {
                icon.source: webEngine.loading? "image://theme/icon-m-reset" : "image://theme/icon-m-refresh"
                onClicked: webEngine.loading ? webEngine.stop() : webEngine.reload()
            }

            IconButton {
                id: right
                icon.source: "image://theme/icon-m-forward"
                enabled: webEngine.canGoForward
                onClicked: webEngine.goForward()
            }
        }
    }

    Connections {
        target: WebUtils
        onOpenUrlRequested: {
            if (webEngine.url != "") {
                storeTab()
                for (var i = 0; i < tabs.count; i++) {
                    if (tabs.get(i).url == url) {
                        // Found it in tabs, load if needed
                        if (i != currentTabIndex) {
                            currentTabIndex = i
                            load(url)
                        }
                        break
                    }
                }
                if (tabs.get(currentTabIndex).url != url) {
                    // Not found in tabs list, create newtab and load
                    newTab()
                    load(url)
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
        History.loadTabs(tabModel)
        if (tabModel.count == 0) {
            newTab()
        }
        History.loadTabHistory(tabModel.get(currentTabIndex).tabId, historyModel)

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

    Timer {
        id: auxTimer

        interval: 1000
    }
}
