/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0

Item {
    id: progressBar

    // From 0 to 1.0
    property real progress: 0.0

    Rectangle {
        id: progressRect
        height: parent.height
        width: progressBar.progress * parent.width
        color: Theme.highlightBackgroundColor
        opacity: 0.3

        Behavior on width {
            enabled: progressBar.opacity == 1.0
            SmoothedAnimation {
                velocity: 480; duration: 200
            }
        }
    }
    Behavior on opacity { FadeAnimation {} }
}
