/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0

ApplicationWindow {
    id: window

    property alias webView: webView
    property alias tabModel: webView.tabModel
    property alias historyModel: historyModel

    allowedOrientations: Orientation.Portrait
    _defaultPageOrientations: allowedOrientations

    initialPage: Page {
        WebView {
            id: webView

            active: true
            toolbarHeight: 50
            portrait: true
            width: parent.width
            height: parent.height

            onLoadProgressChanged: console.log("progress:", loadProgress, url, tabId)
            onLoadingChanged: console.log(loading, url, tabId)
            onUrlChanged: console.log(url, tabId)
            onContentItemChanged: console.log(contentItem, "url:", (contentItem ? contentItem.url : "no url" ), "tabId:", (contentItem ? contentItem.tabId : -1 ))

            HistoryModel {
                id: historyModel
            }
        }
    }

    cover: undefined
}

