/*
 * Copyright (c) 2021 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.2
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import "." as Browser
import "../../shared" as Shared

Item {
    id: root

    property int scaledPortraitHeight
    property int scaledLandscapeHeight

    readonly property int horizontalOffset: largeScreen ? Theme.paddingLarge : Theme.paddingSmall
    readonly property int buttonPadding: largeScreen || orientation === Orientation.Landscape || orientation === Orientation.LandscapeInverted
                                         ? Theme.paddingMedium : Theme.paddingSmall
    readonly property int iconWidth: largeScreen ? (Theme.iconSizeLarge + 3 * buttonPadding) : (Theme.iconSizeMedium + 2 * buttonPadding)

    signal back
    signal enterNewTabUrl
    signal openMenu

    width: parent.width
    height: tabPage.isPortrait ? scaledPortraitHeight : scaledLandscapeHeight

    Rectangle {
        anchors.fill: parent

        color: Theme.colorScheme == Theme.LightOnDark ? "black" : "white"
    }


    Row {
        width: parent.width
        height: parent.height

        Shared.IconButton {
            id: returnIcon
            width: iconWidth
            icon.anchors.horizontalCenterOffset: root.horizontalOffset
            icon.source: "image://theme/icon-m-tab-return"
            onTapped: root.back()
        }

        Item {
            // Space between buttons
            height: parent.height
            width: (parent.width - (returnIcon.width + newTabIcon.width + menuIcon.width)) / 2
        }

        Browser.TabButton {
            id: newTabIcon

            width: iconWidth
            icon.source: webView.privateMode ? "image://theme/icon-m-incognito-new" : "image://theme/icon-m-tab-new"
            onTapped: root.enterNewTabUrl()
        }

        Item {
            // Space between buttons
            height: parent.height
            width: (parent.width - (returnIcon.width + newTabIcon.width + menuIcon.width)) / 2
        }

        Shared.IconButton {
            id: menuIcon

            width: iconWidth
            icon.source: "image://theme/icon-m-menu"
            icon.anchors.horizontalCenterOffset: - root.horizontalOffset
            onTapped: root.openMenu()
        }
    }
}
