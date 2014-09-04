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
import "." as Browser

Browser.IconButton {
    id: backIcon

    property int buttonWidth

    clip: true
    width: opacity * buttonWidth
    opacity: active ? 1.0 : 0.0
    icon.source: "image://theme/icon-m-back"

    // Don't pass touch events through in the middle FadeAnimation
    enabled: opacity === 1.0

    Behavior on opacity { FadeAnimation {} }
}
