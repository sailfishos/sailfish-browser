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
            placeholderText: "Search in page"
            width: parent.width


            onTextChanged: {
                searchBar.search = text
                webView.sendAsyncMessage("embedui:find", { text: text, backwards: false, again: false })
            }
        }

        Label {
            visible: searchInPage.text
            text: webView.findInPageHasResult? "Show found matches" : "No results"
            color: webView.findInPageHasResult? Theme.primaryColor : Theme.highlightColor
            anchors.horizontalCenter: parent.horizontalCenter

            MouseArea {
                anchors.fill: parent
                enabled: visible && webView.findInPageHasResult

                onClicked: {
                    searchBar.visible = true
                    webView.focus = true
                    overlayAnimator.showChrome()
                }
            }
        }

        Label {
            visible: searchInPage.text
            text: "Search again"
            color: Theme.primaryColor
            anchors.horizontalCenter: parent.horizontalCenter

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    searchBar.search = searchInPage.text
                    webView.sendAsyncMessage("embedui:find", { text: text, backwards: false, again: false })
                }
            }
        }
    }
}
