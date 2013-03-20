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
    property bool ignoreStoreUrl: true
    property int currentTabIndex: 0
    property string url
    property variant webEngine: webContent.child

    property variant _controlPageComponent

    function newTab() {
        tabModel.append({"thumbPath": "", "url": ""})
        currentTabIndex = tabModel.count - 1
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
                console.log("Title: " + webEngine.title)
            }

            onViewInitialized: {
                if(historyModel.count == 0 ) {
                    browserPage.url = Parameters.initialPage()
                } else {
                    browserPage.url = historyModel.get(0).url
                }
            }
            onUrlChanged: {
                var urlStr = webEngine.url.toString()
                if(urlStr !== "about:blank" ) {
                    browserPage.url = webEngine.url // To ignore initial "about:blank"
                }
            }
            onLoadingChanged: {
                progressBar.opacity = webEngine.loading ? 1.0 : 0.0
                if(!webEngine.loading)
                    progressBar.progress = 0

                if (!webEngine.loading && url !="about:blank" &&
                    (historyModel.count == 0 || url !== historyModel.get(0).url)) {
                    History.addRow(url,webEngine.title, "image://theme/icon-m-region")
                    historyModel.insert(0, {"title": webEngine.title, "url": url, "icon": "image://theme/icon-m-region"} )
                }
            }
            onLoadProgressChanged: {
                if ((webEngine.loadProgress / 100.0) > progressBar.progress) {
                    progressBar.progress = webEngine.loadProgress / 100.0
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
                icon.source: favoriteModel.count > 0 && favoriteModel.contains(url) ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"
                enabled: true
                onClicked: {
                    if (favoriteModel.contains(url)) {
                        favoriteModel.removeBookmark(url)
                    } else {
                        // Saving url both as url and title since title is not yet coming correctly from engine
                        favoriteModel.addBookmark(url, url)
                    }
                    favoriteModel.save();
                }
            }

            IconButton {
                icon.source: "image://theme/icon-m-tab"

                onClicked:  {
                    var screenPath = BrowserTab.screenCapture(0, 0, webContent.width, webContent.width)
                    tabModel.set(currentTabIndex, {"thumbPath" : screenPath, "url" : browserPage.url})
                    var sendUrl = (browserPage.url != "about:blank" || browserPage.url !== Parameters.initialPage()) ? browserPage.url : ""
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
        title: url

        onStopped: {
            webEngine.stop()
        }
    }

    onUrlChanged: {
        if(!webEngine || webEngine.url == null) {
            console.log("No webengine")
        }
        else if(webEngine.url !== url) {
            webEngine.load(url)
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
