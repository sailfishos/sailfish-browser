/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/


import QtQuick 1.1
import Sailfish.Silica 1.0
import QtMozilla 1.0

import "history.js" as History

Page {
    id: browserPage

    property alias tabs: tabModel
    property bool ignoreStoreUrl: true
    property int currentTab: 0
    property string url
    property variant webEngine: webContent.child()

    function newTab() {
        tabModel.append({"thumbPath": "", "url": ""})
        currentTab = tabModel.count - 1
    }

    ListModel {
        id: historyModel
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
                if (!webEngine.loading && url !="about:blank" &&
                    (historyModel.count == 0 || url !== historyModel.get(0).url)) {
                    History.addRow(url,webEngine.title, "image://theme/icon-m-region")
                    historyModel.insert(0,{"title": webEngine.title, "url": url, "icon": "image://theme/icon-m-region"} )
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
        visible: parent.height === screen.height

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
                icon.source: "image://theme/icon-m-favorite"
                enabled: true
            }

            IconButton {
                icon.source: "image://theme/icon-m-tab"

                onClicked:  {
                    var screenPath = BrowserTab.screenCapture(0, 0, webContent.width, webContent.width)
                    tabModel.set(currentTab, {"thumbPath" : screenPath, "url" : browserPage.url})
                    var component = Qt.createComponent("ControlPage.qml");
                    if (component.status === Component.Ready) {
                        var sendUrl = (browserPage.url != "about:blank" || browserPage.url !== Parameters.initialPage()) ? browserPage.url : ""
                        pageStack.push(component, {historyModel: historyModel, url: sendUrl}, true);
                    } else {
                        console.log("Error loading component:", component.errorString());
                    }
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
    }
}
