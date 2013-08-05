/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/


import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0
import QtQuick.LocalStorage 2.0
import Qt5Mozilla 1.0
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
    property bool _ctxMenuActive: _contextMenu != null && _contextMenu.active
    property bool _ctxMenuVisible: _contextMenu != null && _contextMenu.visible
    // As QML can't disconnect closure from a signal (but methods only)
    // let's keep auth data in this auxilary attribute whose sole purpose is to
    // pass arguments to openAuthDialog().
    property variant _authData: null


    function newTab(link, foreground) {
        var id = History.addTab(link,"")
        tabModel.append({"thumbPath": {"path":""}, "url": link, "tabId": id})

        if(foreground) {
            historyModel.clear()
            currentTabIndex = tabModel.count - 1
            History.saveSetting("ActiveTab", id.toString())

            if (link !== "" && webEngine.url != link) {
                webEngine.load(link)
            }
        }
    }

    function closeTab(index) {
        var tabIndex = index ? currentTabIndex : index

        if (tabModel.count == 0) {
            return
        }
        History.deleteTab(tabModel.get(tabIndex).tabId)
        tabModel.remove(tabIndex)
        currentTabIndex = tabModel.count - 1
        historyModel.clear()
        if (tabModel.count >= 1) {
            History.loadTabHistory(tabModel.get(currentTabIndex).tabId, historyModel)
            // url from tabModel is only used for new tabs (without history)
            var url = historyModel.count > 0 ? historyModel.get(0).url : tabModel.get(currentTabIndex).url
            load(url)
            History.saveSetting("ActiveTab", tabModel.get(currentTabIndex).tabId.toString())
        }
    }

    function load(url) {
        if (tabModel.count == 0) {
            newTab(url, true)
        } else if (webEngine.url != url) {
            webEngine.load(url)
        }
    }

    function loadTab(index) {
        if (currentTabIndex !== index) {
            currentTabIndex = index
            historyModel.clear()

            History.loadTabHistory(tabModel.get(currentTabIndex).tabId, historyModel)
            // url from tabModel is only used for new tabs (without history)
            var url = historyModel.count > 0 ? historyModel.get(0).url : tabModel.get(currentTabIndex).url
            load(url)
            History.saveSetting("ActiveTab", tabModel.get(currentTabIndex).tabId.toString())
        }
    }

    function deleteTabHistory() {
        historyModel.clear()
        History.deleteTabHistory(tabModel.get(currentTabIndex).tabId)
    }

    function storeTab() {
        var webThumb

        // Try to capture fresh screen capture if still active
        // if not, then try to use from history and if that does not exist, lets not store a thumb
        if (status == PageStatus.Active) {
            webThumb = BrowserTab.screenCapture(0, 0, webContent.width, webContent.width, window.screenRotation)
        } else if (historyModel.count > 0 && historyModel.get(0).url == webEngine.url) {
            webThumb = historyModel.get(0).icon
        } else {
            webThumb = {"path":"", "source":""}
        }

        tabModel.set(currentTabIndex, {"thumbPath" : webThumb, "url" : webEngine.url.toString()})
        History.updateTab(tabModel.get(currentTabIndex).tabId, webEngine.url, webThumb)
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

    ListModel {
        id:tabModel
    }

    DownloadRemorsePopup { id: downloadPopup }

    QmlMozView {
        id: webContent

        signal selectionRangeUpdated(variant data)
        signal selectionCopied(variant data)
        signal contextMenuRequested(variant data)
        clip: true

        anchors {
            top: parent.top
            left: parent.left
        }
        focus: true
        width: browserPage.width
        enabled: !_ctxMenuActive && !downloadPopup.visible
        // This flag instucts web engine to ignore mouse and single-touch
        // events (multi-touch ones are not ignored). This means that we have to
        // relay the events from QML to web engine through a special MouseArea
        // that fully covers web view (TextSelectionController).
        useQmlMouse: true

        height: browserPage.height - Theme.itemSizeMedium
        //{ // TODO
        // No resizes while page is not active
        // also contextmenu size
        //           if (browserPage.status == PageStatus.Active) {
        //               return (_contextMenu != null && (_contextMenu.height > tools.height)) ? browserPage.height - _contextMenu.height : browserPage.height - tools.height
        //               return (_contextMenu != null && (_contextMenu.height > tools.height)) ? 200 : 300

        onTitleChanged: {
            // Update title in model, title can come after load finished
            // and then we already have element in history
            if (historyModel.count > 0
                    && historyModel.get(0).url == url
                    && title !== historyModel.get(0).title ) {
                historyModel.setProperty(0, "title", title)
            }
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
                browserPage.load(historyModel.get(0).url)
            }
        }
        onLoadingChanged: {
            progressBar.opacity = loading ? 1.0 : 0.0
            if (!loading) {
                progressBar.progress = 0
            } else {
                favicon = ""
            }

            if (!loading && url != "about:blank" &&
                    (historyModel.count == 0 || url != historyModel.get(0).url)) {
                var webThumb
                if (status == PageStatus.Active) {
                    webThumb = BrowserTab.screenCapture(0, 0, webContent.width, webContent.width, window.screenRotation)
                } else {
                    webThumb = {"path":"", "source":""}
                }
                historyModel.insert(0, {"title": title, "url": webContent.url.toString(), "icon": webThumb} )
                History.addUrl(url, title, webThumb, tabModel.get(currentTabIndex).tabId)
            }
        }
        onLoadProgressChanged: {
            if ((loadProgress / 100.0) > progressBar.progress) {
                progressBar.progress = loadProgress / 100.0
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
                                            "webview": webContent
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
            case "embed:login": {
                pageStack.push(Qt.resolvedUrl("components/PasswordManagerDialog.qml"),
                               {
                                   "webEngine": webEngine,
                                   "requestId": data.id,
                                   "notificationType": data.name,
                                   "formData": data.formdata
                               })
                break
            }
            case "Content:ContextMenu": {
                webContent.contextMenuRequested(data)
                if (data.types.indexOf("image") !== -1 || data.types.indexOf("link") !== -1) {
                    openContextMenu(data.linkURL, data.mediaURL)
                }
                break
            }
            case "Content:SelectionRange": {
                webContent.selectionRangeUpdated(data)
                break
            }
            }
        }
        onRecvSyncMessage: {
            // sender expects that this handler will update `response` argument
            switch (message) {
            case "Content:SelectionCopied": {
                webContent.selectionCopied(data)

                if (data.succeeded) {
                    //% "Copied to clipboard"
                    notification.show(qsTrId("sailfish_browser-la-selection_copied"))
                }
                break
            }
            }
        }
        onViewAreaChanged: {
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


        TextSelectionController {}

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
            anchors.bottom: parent ? parent.bottom: undefined
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
        anchors.fill: webContent

        color: Theme.highlightDimmerColor
        opacity: _ctxMenuActive? 0.8 : 0.0
        Behavior on opacity { FadeAnimation {} }
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
            GradientStop { position: 1.0; color: Theme.highlightDimmerColor }
        }

        Column {
            width: parent.width
            anchors {
                bottom: parent.bottom; bottomMargin: Theme.paddingMedium
            }

            Label {
                text: webEngine.title
                width: parent.width - Theme.paddingMedium * 2
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
                truncationMode: TruncationMode.Fade
            }
            Label {
                text: webEngine.url
                width: parent.width - Theme.paddingMedium * 2
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
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
        height: visible ? Theme.itemSizeMedium : 0
        visible: !_ctxMenuActive

        ProgressBar {
            id: progressBar
            anchors.fill: parent
            opacity: 0.0
        }

        Row {
            id: toolsrow

            function openControlPage() {
                storeTab()
                var sendUrl = (webEngine.url != WebUtils.initialPage) ? webEngine.url : ""
                pageStack.push(_controlPageComponent, {historyModel: historyModel, url: sendUrl}, PageStackAction.Animated)
            }

            anchors {
                left: parent.left
                right: parent.right
                verticalCenter: parent.verticalCenter
            }

            // 5 icons, 4 spaces between
            spacing: (width - (backIcon.width * 5)) / 4

            IconButton {
                id:backIcon
                icon.source: "image://theme/icon-m-back"
                enabled: webEngine.canGoBack
                onClicked: webEngine.goBack()
            }

            IconButton {
                icon.source: "image://theme/icon-m-search"
                enabled: true
                onClicked: toolsrow.openControlPage()
            }

            IconButton {
                icon.source: "image://theme/icon-m-levels"

                onClicked:  {
                    storeTab()
                    var sendUrl = (webEngine.url != WebUtils.initialPage) ? webEngine.url : ""
                    pageStack.push(Qt.resolvedUrl("TabPage.qml"), {"browserPage" : browserPage}, PageStackAction.Immediate)
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


    CoverActionList {
        iconBackground: true

        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: {
                toolsrow.openControlPage()
                activate()
            }
        }

        CoverAction {
            iconSource: "image://theme/icon-cover-refresh"
            onTriggered: {
                if (webEngine.loading) {
                    webEngine.stop()
                }
                webEngine.reload()
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
        History.loadTabs(tabModel)
        if (tabModel.count == 0) {
            newTab("", true)
        } else {
            var tabId = parseInt(History.loadSetting("ActiveTab"))
            for(var i=0; i< tabModel.count; i++) {
                if(tabModel.get(i).tabId == tabId) {
                    currentTabIndex = i
                }
            }
            History.loadTabHistory(tabId, historyModel)
        }
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

    WorkerScript {
        id: dbWorker
        source: "dbWorker.js"
    }

    BrowserNotification {
        id: notification
    }
}
