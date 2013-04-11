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

    function newTab() {
        tabModel.append({"thumbPath": "", "url": ""})
        currentTabIndex = tabModel.count - 1
    }

    function load(url) {
        if (!webEngine || webEngine.url == null) {
            console.log("No webengine")
        } else if (webEngine.url !== url) {
            webEngine.load(url)
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
        ListElement {thumbPath: ""; url: ""}
    }

    QmlMozView {
        id: webContent
        anchors {
            top: parent.top
            left: parent.left
        }
        focus: true
        width: browserPage.width

        // No resizes while page is not active
        // workaround for engine crashes on resizes while background
        height: (browserPage.status == PageStatus.Active) ? browserPage.height - tools.height : screen.height - tools.height

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
                webContent.child.addMessageListener("chrome:linkadded")
                if (Parameters.initialPage !== "") {
                    browserPage.load(Parameters.initialPage)
                } else if (historyModel.count == 0 ) {
                    browserPage.load(Parameters.homePage)
                } else {
                    browserPage.load(historyModel.get(0).url)
                }
            }
            onLoadingChanged: {
                progressBar.opacity = webEngine.loading ? 1.0 : 0.0
                if(!webEngine.loading) {
                    progressBar.progress = 0
                } else {
                    favicon = ""
                }

                if (!webEngine.loading && webEngine.url != "about:blank" &&
                    (historyModel.count == 0 || webEngine.url != historyModel.get(0).url)) {
                    var screenPath = BrowserTab.screenCapture(0, 0, webContent.width, webContent.width, window.screenRotation)
                    History.addRow(webEngine.url, webEngine.title, screenPath)
                    historyModel.insert(0, {"title": webEngine.title, "url": webEngine.url, "icon": screenPath} )
                }
            }
            onLoadProgressChanged: {
                if ((webEngine.loadProgress / 100.0) > progressBar.progress) {
                    progressBar.progress = webEngine.loadProgress / 100.0
                }
            }
            onRecvAsyncMessage: {
                if (message == "chrome:linkadded" && data.rel == "shortcut icon") {
                    favicon = data.href
                }
            }
        }
    }

    Rectangle {
        id: tools
        color:"black"
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        height: visible ? theme.itemSizeMedium : 0
        visible: (parent.height === screen.height)

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
                    var screenPath = BrowserTab.screenCapture(0, 0, webContent.width, webContent.width, window.screenRotation)
                    tabModel.set(currentTabIndex, {"thumbPath" : screenPath, "url" : webEngine.url})
                    var sendUrl = (webEngine.url != Parameters.initialPage) ? webEngine.url : ""
                    pageStack.push(_controlPageComponent, {historyModel: historyModel, url: sendUrl}, true);
                }
            }
            IconButton {
                icon.source: "image://theme/icon-m-refresh"
                onClicked: webEngine.reload()
            }

            IconButton {
                id: right
                icon.source: "image://theme/icon-m-forward"
                enabled: webEngine.canGoForward
                onClicked: webEngine.goForward()
            }
        }
    }

    ProgressBar {
        id:progressBar
        anchors.fill: tools
        opacity: 0.0
        title: webEngine.title !== "" ? webEngine.title : webEngine.url

        onStopped: {
            webEngine.stop()
        }
    }

    Component.onCompleted: {
        History.loadModel(historyModel)

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
}
