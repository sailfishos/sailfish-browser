/****************************************************************************
**
** Copyright (c) 2014 Jolla Ltd.
** Copyright (c) 2021 Open Mobile Platform LLC.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0
import Sailfish.WebView.Popups 1.0

SilicaListView {
    id: view

    property alias viewPlaceholder: viewPlaceholder
    property string search
    property bool showDeleteButton
    property bool menuClosed

    signal load(string url, string title, bool newTab)
    signal saveBookmark(string url, string title, string favicon)

    onLoad: if (newTab) webView.privateMode = !webView.privateMode

    // To prevent model to steal focus
    currentIndex: -1

    footer: Item {
        width: 1
        height: Theme.itemSizeLarge
    }

    delegate: HistoryItem {
        id: historyDelegate

        search: view.search
        showDeleteButton: view.showDeleteButton
        onMenuOpenChanged: view.menuClosed = !menuOpen
    }

    WebShareAction {
        id: webShareAction
    }

    ViewPlaceholder {
        id: viewPlaceholder

        //% "Websites you visit show up here"
        text: qsTrId("sailfish_browser-la-websites-show-up-here")
    }

    VerticalScrollDecorator {}
}
