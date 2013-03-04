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
    property string url: ""

    function newTab() {
        tabModel.append({"thumbPath": "", "url": ""})
        currentTab = tabModel.count -1
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
        visible: true
        focus: true

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: tools.top
        }

        Connections {
            target: webContent.child()

            onViewInitialized: {
                webContent.child().load("www.yle.fi") // Workaround for initial page loadign
            }

            onTitleChanged: {
                pageTitleChanged(webContent.child().title)
            }
            onUrlChanged: {
                browserPage.url = webContent.child().url
                if(webContent.child().url=="about:blank") { // Workaround for initial page loadign
                    if(historyModel.count == 0 ) {
                        webContent.child().load(Parameters.initialPage())
                    } else {
                        webContent.child().load(historyModel.get(0).url)
                    }
                }
            }
            onLoadingChanged: {
                if (!webContent.child().loading && url !="about:blank" &&
                    (historyModel.count == 0 || url !== historyModel.get(0).url)) {
                    History.addRow(url,webContent.child().title, "image://theme/icon-m-region")
                    historyModel.insert(0,{"title": webContent.child().title, "url": url, "icon": "image://theme/icon-m-region"} )
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
        height: theme.itemSizeMedium

        Row {
            id: toolsrow
            anchors.fill: parent
            spacing: (width - (backIcon.width*5))/4

            IconButton {
                id:backIcon
                icon.source: "image://theme/icon-m-back"
                enabled: true // TODO webContent.back.enabled

                onClicked: {
                    webContent.child().goBack()
                }
            }

            IconButton {
                icon.source: "image://theme/icon-m-favorite"
                enabled: false
                onClicked: {
                    ignoreStoreUrl = true
                }
            }

            IconButton {
                icon.source: "image://theme/icon-m-other"
                enabled: true // TODO webContent.back.enabled

                onClicked:  {
                    console.log("Other clicked")
                    // TODO grab widget
                    /*var screenPath = (window.screenRotation == 0) ? BrowserTab.screenCapture(0,0,webContent.width, webContent.height) :
                                                                    BrowserTab.screenCapture(0,0,webContent.height, webContent.width);  */
                    var screenPath = ""
                    tabModel.set(currentTab, {"thumbPath" : screenPath, "url" : browserPage.url})
                    var component = Qt.createComponent("ControlPage.qml");
                    if (component.status === Component.Ready) {
                        var sendUrl = (browserPage.url != "about:blank" || browserPage.url != Parameters.initialPage()) ? browserPage.url : ""
                        pageStack.push(component, {historyModel: historyModel, url: sendUrl}, true);
                    } else {
                        console.log("Error loading component:", component.errorString());
                    }
                }
            }
            IconButton {
                icon.source: "image://theme/icon-m-sync"

                onClicked: {
                    webContent.child().reload()
                }
            }

            IconButton {
                id: right
                icon.source: "image://theme/icon-m-forward"
                enabled: true // webContent.forward.enabled
                onClicked: {
                    webContent.child().goForward()
                }
            }
        }
    }

    onUrlChanged: {
        if(webContent.child().url!==url) {
            webContent.child().load(url)
        }
    }

    Component.onCompleted: {
        console.log("Component completed")
        History.loadModel(historyModel)
    }
}


