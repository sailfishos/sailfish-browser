/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import Sailfish.Silica 1.0
import "." as Browser

Item {
    id: tabBar
    // By default url entry tab is active
    property int currentIndex: 1
    width: parent.width
    height: iconRow.height

    signal selectTab(int index)

    Row {
        id: iconRow
        height: childrenRect.height
        anchors.centerIn: parent

        spacing: (parent.width - (4 * Theme.iconSizeMedium)) / 4

        Browser.TabButton {
            selected: currentIndex == 0
            id: backIcon
            icon.source: "image://theme/icon-m-clock"

            label: "History"
            onTapped: selectTab(0)
        }

        Browser.TabButton {
            selected: currentIndex == 1
            icon.source: "image://theme/icon-m-keyboard"
            onTapped: selectTab(1)
            label: "Address"
        }

        Browser.TabButton {
            selected: currentIndex == 2
            icon.source: "image://theme/icon-m-search"
            onTapped: selectTab(2)
            label: "Find"
        }

        Browser.TabButton {
            selected: currentIndex == 3
            icon.source: "image://theme/icon-m-mobile-network"
            onTapped: selectTab(3)
            label: "Downloads"
        }
    }
}
