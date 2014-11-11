/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0

Loader {
    property Item viewItem

    anchors.fill: viewItem
    active: counter.active

    sourceComponent: Item {
        property bool fullscreenMode: viewItem && viewItem.fullscreenMode

        anchors.fill: parent

        onFullscreenModeChanged: {
            if (fullscreenMode) {
                touchInteractionHint.direction = TouchInteraction.Down
            } else {
                touchInteractionHint.direction = TouchInteraction.Up
            }

            counter.increase()
            touchInteractionHint.restart()
        }

        InteractionHintLabel {
            text: fullscreenMode ?
                      //: Flick down to reveal the toolbar
                      //% "Flick down to reveal the toolbar"
                      qsTrId("sailfish_browser-la-toolbar_hint") :

                      //: Flick up to hide the toolbar
                      //% "Flick up to hide the toolbar"
                      qsTrId("sailfish_browser-la-toolbar_hide_hint")
            anchors.bottom: parent.bottom
            opacity: touchInteractionHint.running ? 1.0 : 0.0
            Behavior on opacity { FadeAnimation { duration: 1000 } }
        }

        TouchInteractionHint {
            id: touchInteractionHint

            // Initial state differs from normal state. That's why direction is handled in onFullscreenModeChanged.
            direction: TouchInteraction.Up
            anchors.horizontalCenter: parent.horizontalCenter
            loops: 6

            layer.effect: PressEffect {
                color: Theme.highlightColor
                opacity: touchInteractionHint.opacity
            }
            layer.enabled: !firstUseOverlay

            Component.onCompleted: start()
        }
    }

    FirstTimeUseCounter {
        id: counter
        limit: 4
        key: "/apps/sailfish-browser/hints/fullscreen_mode_count"
    }
}
