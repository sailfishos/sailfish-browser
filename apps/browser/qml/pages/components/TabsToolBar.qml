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

    signal back
    signal enterNewTabUrl

    width: parent.width
    height: tabPage.isPortrait ? scaledPortraitHeight : scaledLandscapeHeight

    Rectangle {
        anchors.fill: parent
        anchors {
            verticalCenter: parent.verticalCenter
        }

        color: Theme.colorScheme == Theme.LightOnDark ? "black" : "white"
    }

    Shared.IconButton {
        anchors {
            left: parent.left
            leftMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }
        width: Theme.itemSizeSmall
        icon.source: "image://theme/icon-m-tab-return"
        onTapped: root.back()
    }
    Browser.TabButton {
        anchors {
            centerIn: parent
            verticalCenter: parent.verticalCenter
        }
        width: Theme.itemSizeSmall
        icon.source: webView.privateMode ? "image://theme/icon-m-incognito-new" : "image://theme/icon-m-tab-new"
        onTapped: root.enterNewTabUrl()
    }

    Shared.IconButton {
        anchors {
            right: parent.right
            rightMargin: Theme.horizontalPageMargin
            verticalCenter: parent.verticalCenter
        }

        visible: false // TODO: Make visible after adding popup menu
        width: Theme.itemSizeSmall
        icon.source: "image://theme/icon-m-menu"
    }
}
