/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0

Item {
    property alias findInPageField: searchInPage
    Column {
        width: parent.width
        spacing: Theme.paddingMedium

        SearchField {
            id: searchInPage
            placeholderText: "Find within webpage"
            width: parent.width


            onTextChanged: {
                searchBar.search = text
                webView.sendAsyncMessage("embedui:find", { text: text, backwards: false, again: false })
            }
        }

        Button {
            visible: searchInPage.text && webView.findInPageHasResult
            text: "Show found matches"
            anchors.horizontalCenter: parent.horizontalCenter

            onClicked: {
                searchBar.visible = true
                webView.focus = true
                overlayAnimator.showChrome()
            }
        }

        Label {
            id: noResults
            text: "No results"
            color: Theme.highlightColor
            visible: searchInPage.text && !webView.findInPageHasResult
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Button {
            visible: noResults.visible
            anchors.horizontalCenter: parent.horizontalCenter

            text: "Search again"
            onClicked: {
                searchInPage.text = ""
            }
        }
    }
}
