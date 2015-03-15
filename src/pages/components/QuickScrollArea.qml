/****************************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Joona Petrell <joona.petrell@jollamobile.com>
** All rights reserved.
**
** This file is part of Sailfish Silica UI component package.
**
** You may use this file under the terms of BSD license as follows:
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the Jolla Ltd nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
** ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************************/

import QtQuick 2.1
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0

Column {
    id: root

    property WebPage flickable
    property bool flicking
    //property bool inBounds: (!flickable.pullDownMenu || !flickable.pullDownMenu.active) && (!flickable.pushUpMenu || !flickable.pushUpMenu.active)
    property bool inBounds: true
    property bool pressed: scrollUpButton.pressed || scrollDownButton.pressed
    property bool active: true //visibilityTimer.running || !clicked && (pressed || (flicking && inBounds))
    property bool clicked
    onVisibleChanged: console.log("QuickScrollArea visible:" + visible)

    Component.onCompleted: {
        console.log("height is " + height)
        console.log("width is " + width)
    }

    onFlickingChanged: {
        if (flicking) {
            clicked = false
        } else {
            if (inBounds) {
                visibilityTimer.restart()
            }
        }
    }

    height: flickable.height
    width: Theme.itemSizeExtraLarge
    enabled: opacity > 0.0
    anchors.right: parent.right

    opacity: active ? 1.0 : 0.0
    Behavior on opacity { FadeAnimation { duration: 400 } }

    Timer {
        id: visibilityTimer
        interval: 200
    }
    Connections {
        target: flickable
        onFlickingVerticallyChanged: flicking = flickable.flickingVertically && Math.abs(flickable.verticalVelocity) > Screen.height
    }
    QuickScrollButton {
        id: scrollUpButton

        invert: false
        active: true //!flickable.atYBeginning
        source: "image://theme/icon-m-page-up"
        flickable: root.flickable
        onClicked: {
            flickable.scrollToTop()
            root.clicked = true
        }

        onVisibleChanged: console.log("QuickScrollButton visible:" + visible)
    }
    QuickScrollButton {
        id: scrollDownButton

        invert: true
        active: true //!flickable.atYEnd
        source: "image://theme/icon-m-page-down"
        flickable: root.flickable
        onClicked: {
            flickable.scrollToBottom()
            root.clicked = true
        }

        onVisibleChanged: console.log("QuickScrollButton visible:" + visible)
    }
}
