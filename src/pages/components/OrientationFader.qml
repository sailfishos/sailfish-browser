/****************************************************************************
**
** Copyright (C) 2015 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import Sailfish.Silica 1.0

Rectangle {
    id: orientationFader

    property alias orientationTransition: transition
    property alias page: propertyAction.target
    property alias fadeTarget: fadeOut.target
    readonly property alias running: transition.running
    property bool waitForWebContentOrientationChanged

    signal applyContentOrientation

    anchors.fill: parent
    opacity: running || waitForWebContentOrientationChanged ? 1.0 : 0.0

    Behavior on opacity {
        FadeAnimation {
            alwaysRunToEnd: true
            duration: 100
        }
    }

    Transition {
        id: transition

        to: 'Portrait,Landscape,PortraitInverted,LandscapeInverted'
        from: 'Portrait,Landscape,PortraitInverted,LandscapeInverted'

        SequentialAnimation {
            PropertyAction {
                id: propertyAction
                property: 'orientationTransitionRunning'
                value: true
            }

            FadeAnimation {
                id: fadeOut
                to: 0
                duration: 100
            }

            PropertyAction {
                target: page
                properties: 'width,height,rotation,orientation'
            }

            ScriptAction {
                script: {
                    // Restores the Bindings to width, height and rotation
                    page._defaultTransition = false
                    page._defaultTransition = true
                    orientationFader.applyContentOrientation()
                }
            }

            FadeAnimation {
                target: fadeTarget
                to: 1
                duration: 150
            }

            PropertyAction {
                target: page
                property: 'orientationTransitionRunning'
                value: false
            }
        }
    }
}
