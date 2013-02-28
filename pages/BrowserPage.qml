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
        ListElement {title: "Jolla"; url: "http://www.jolla.com/"; icon: "image://theme/icon-m-region"}
        ListElement {title: "Sailfish OS"; url: "http://www.sailfishos.org/"; icon: "image://theme/icon-m-region"}
        ListElement {title: "Mer-project"; url: "http://www.merproject.org/"; icon: "image://theme/icon-m-region"}
        ListElement {title: "Twitter"; url: "http://www.twitter.com/"; icon: "image://theme/icon-m-region"}
        ListElement {title: "Google"; url: "http://www.google.com/"; icon: "image://theme/icon-m-region"}
        ListElement {title: "Facebook"; url: "http://www.facebook.com/"; icon: "image://theme/icon-m-region"}
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
                webContent.child().load(Parameters.initialPage())
            }

            onTitleChanged: {
                pageTitleChanged(webViewport.child().title)
            }
            onUrlChanged: {
                browserPage.url = webContent.child().url
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
        height: theme.itemSizeLarge

        Row {
            id: toolsrow
            anchors.fill: parent
            IconButton {
                id:backIcon
                icon.source: "image://theme/icon-l-left"
                enabled: false // TODO webContent.back.enabled

                onClicked: {
                    ignoreStoreUrl = true
                }
            }

            Label {
                id: title
                text: "URL" // TODO webContent.status == WebView.Loading ? webContent.statusText : webContent.title
                width: browserPage.width - (backIcon.width + right.width)
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                anchors.verticalCenter: parent.verticalCenter
                truncationMode: TruncationMode.Fade

                MouseArea {
                    anchors.fill: parent
                    onClicked:  {
                        var screenPath = (window.screenRotation == 0) ? BrowserTab.screenCapture(0,0,webContent.width, webContent.height) :
                                                                        BrowserTab.screenCapture(0,0,webContent.height, webContent.width);

                        tabModel.set(currentTab, {"thumbPath" : screenPath, "url" : browserPage.url})
                        var component = Qt.createComponent("ControlPage.qml");
                        if (component.status === Component.Ready) {
                            var sendUrl = (browserPage.url != Parameters.homePage) ? browserPage.url : ""
                            pageStack.push(component, {historyModel: historyModel, url: sendUrl}, true);
                        } else {
                            console.log("Error loading component:", component.errorString());
                        }
                    }
                }
            }

            IconButton {
                id: right
                icon.source: "image://theme/icon-l-right"
                enabled: false // webContent.forward.enabled
                onClicked: {
                    ignoreStoreUrl = true
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
        History.loadModel(historyModel)
    }
}


