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

Dialog {
    id: dialog
    property alias acceptText: header.acceptText
    property alias title: header.title
    default property alias defaultContent: promptContent.children

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        contentHeight: contentColumn.height

        Column {
            id: contentColumn
            width: parent.width

            DialogHeader {
                id: header
            }

            Item {
                id: promptContent
                width: dialog.width

                // So that anchoring works in user prompt dialog implementations
                // and dialog looks visually good. This leaves annoying binding loop which happens
                // when keyboard is opened as keyboard opening reduces dialog height.
                height: Math.max(childrenRect.height, dialog.height - header.height) - Theme.paddingLarge * 2
            }
        }
    }
}
