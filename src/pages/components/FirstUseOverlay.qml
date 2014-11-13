/****************************************************************************
**
** Copyright (C) 2013, 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0


SilicaFlickable {
    id: root

    property double startY
    property double gestureThreshold

    // These properties allows us to fake the FirstUseOverlay
    // for OverlayAnimator to be similar to WebView itself.
    property real fullscreenHeight
    property bool fullscreenMode

    function done() {
        if (!WebUtils.firstUseDone) {
            WebUtils.firstUseDone = true
        }
        dismiss()
    }

    function dismiss() {
        visible = false
        firstUseOverlay = null
        webView.visible = true
        destroy()
    }

    contentHeight: contentColumn.height
    z: -1
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    flickableDirection: Flickable.VerticalFlick

    onFlickStarted: startY = contentY
    onFlickEnded: startY = contentY

    onContentYChanged: {
        var offset = contentY
        var currentDelta = offset - startY

        if (currentDelta > gestureThreshold) {
            startY = contentY
            currentDelta = 0
            fullscreenMode = true
        } else if (currentDelta < -gestureThreshold) {
            startY = contentY
            currentDelta = 0
            fullscreenMode = false
        }
    }

    Column {
        id: contentColumn
        anchors {
            margins: Theme.paddingLarge
            top: parent.top
            left: parent.left
            right: parent.right
        }
        spacing: Theme.paddingLarge
        height: window.height + gestureThreshold * 8

        Label {
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeHuge
            font.family: Theme.fontFamilyHeading
            width: parent.width - Theme.paddingLarge * 2

            //: Ahoy
            //% "Ahoy"
            text: qsTrId("sailfish_browser-he-first_use_ahoy")
        }

        Label {
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeExtraLarge
            font.family: Theme.fontFamilyHeading
            width: parent.width - Theme.paddingLarge * 2
            wrapMode: Text.WordWrap

            //: Welcome to the new Sailfish Browser experience
            //% "Welcome to the new Sailfish Browser experience"
            text: qsTrId("sailfish_browser-he-first_use_welcome")
        }
    }

    children: FullscreenModeHint {
        viewItem: root
    }
}
