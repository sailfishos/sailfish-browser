/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import Sailfish.Silica 1.0
import "." as Browser

Item {
    id: toolBarRow

    property bool edited
    property bool mayRestoreTitle
    property string title
    property string url
    property alias textField: textField
    property alias text: textField.text
    property bool hasTitle: title === text
    property bool controlsVisible: true
    property bool enteringNewTabUrl
    property bool atTop
    property bool atBottom

    property bool _onlyUrlField

    function reset(url, newTab) {
        enteringNewTabUrl = newTab || false
        textField.text = url
        edited = false
        mayRestoreTitle = false
    }

    function showTitle() {
        if (!enteringNewTabUrl && (!atTop || atBottom && mayRestoreTitle)) {
            textField.text = title
            edited = false
        }
    }

    function showControls() {
        controlsVisible = true
    }

    function hideControls() {
        controlsVisible = false
    }

    signal showTabs
    signal showChrome
    signal showOverlay
    signal load(string text)

    width: parent.width
    height: isPortrait ? Settings.toolbarLarge : Settings.toolbarSmall

    onTitleChanged: showTitle()

    onAtTopChanged: {
        if (atTop) {
            textField.forceActiveFocus()
            mayRestoreTitle = true
        } else {
            textField.focus = false
        }
    }

    onAtBottomChanged: showTitle()

    Browser.IconButton {
        id: backIcon
        visible: active && !_onlyUrlField
        active: webView.canGoBack
        width: visible ? Theme.itemSizeSmall : 0
        height: parent.height
        icon.source: "image://theme/icon-m-back"
        onTapped: webView.goBack()
    }

    TextField {
        id: textField

        // Translation!
        placeholderText: "Search or type URL"
        labelVisible: false
        textLeftMargin: controlsVisible && backIcon.visible ? Theme.paddingSmall : Theme.paddingLarge
        textRightMargin: controlsVisible ? Theme.paddingSmall : Theme.paddingLarge

        anchors {
            left: backIcon.right
            right: reload.left
            verticalCenter: parent.verticalCenter
        }

        onTextChanged: {
            if ((!hasTitle || enteringNewTabUrl) && text !== webView.url && !edited) {
                edited = true
            }
        }

        onActiveFocusChanged: {
            if (activeFocus) {
                showOverlay()
                if (hasTitle && !enteringNewTabUrl) {
                    reset(overlay.webView.url)
                }
            }
        }

        EnterKey.iconSource: "image://theme/icon-m-enter-accept"
        EnterKey.onClicked: toolBarRow.load(text)
    }

    Browser.IconButton {
        id: reload
        visible: !_onlyUrlField
        active: webView.visible && visible
        width: visible ? Theme.itemSizeSmall : 0
        height: parent.height
        anchors.right: tabs.left
        icon.source: webView.loading ? "image://theme/icon-m-reset" : "image://theme/icon-m-refresh"
        onTapped: webView.loading ? webView.stop() : webView.reload()
    }

    Browser.IconButton {
        id: tabs
        visible: !_onlyUrlField
        width: visible ? Theme.itemSizeSmall : 0
        height: parent.height
        anchors.right: parent.right
        icon.source: "image://theme/icon-m-tabs"
        onTapped: {
            if (firstUseOverlay) {
                firstUseOverlay.visible = false
                firstUseOverlay.destroy()
            }
            if (!WebUtils.firstUseDone) WebUtils.firstUseDone = true
            toolBarRow.showTabs()
        }
        Label {
            visible: webView.tabModel.count > 0
            text: webView.tabModel.count
            x: (parent.width - contentWidth) / 2 - 5
            y: (parent.height - contentHeight) / 2 - 5
            font.pixelSize: Theme.fontSizeExtraSmall
            font.bold: true
            color: tabs.down ?  Theme.primaryColor : Theme.highlightDimmerColor
            horizontalAlignment: Text.AlignHCenter
        }
    }

    states: [
        State {
            name: "onlyUrlField"
            when: !controlsVisible
        },
        State {
            name: "controlsVisible"
            when: controlsVisible
        }
    ]

    transitions: [
        Transition {
            from: "controlsVisible"
            to: "onlyUrlField"

            SequentialAnimation {
                Browser.FadeAnimation { target: toolBarRow; to: 0.0; duration: 150 }
                PropertyAction { target: toolBarRow; property: "_onlyUrlField"; value: true }
                ScriptAction {
                    script: {
                        console.log("onlu url field: ", url, textField.text)
                        // Opening to new tab url input.
                        if (!enteringNewTabUrl) {
                            reset(url)
                            textField.selectAll()
                        }
                    }
                }
                Browser.FadeAnimation { target: toolBarRow; to: 1.0; duration: 150 }
            }
        },
        Transition {
            from: "onlyUrlField"
            to: "controlsVisible"
            SequentialAnimation {
                Browser.FadeAnimation { target: toolBarRow; to: 0.0; duration: 150 }
                PropertyAction { target: toolBarRow; property: "_onlyUrlField"; value: false }
                Browser.FadeAnimation { target: toolBarRow; to: 1.0; duration: 150 }
             }
        }
    ]
}
