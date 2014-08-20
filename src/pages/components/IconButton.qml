/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0

IconButton {
    // Keep icon enabled so that drag area can filter mouse events.
    // TODO: Are we disabling somewhere IconButton? Can active be removed?
    property bool active: true

    signal tapped

    height: parent.height
    width: height
    icon.opacity: active ? 1.0 : 0.4
    icon.highlighted: active && down

    onClicked: {
        if (active) {
            tapped()
        }
    }
}
