/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0

ComboBox {
    property alias iconSource: icon.source

    width: parent.width
    leftMargin: Theme.horizontalPageMargin + icon.width + Theme.paddingMedium

    Icon {
        id: icon

        anchors.verticalCenter: parent.verticalCenter
        x: Theme.horizontalPageMargin
    }
}
