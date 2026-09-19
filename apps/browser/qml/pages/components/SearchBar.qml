/****************************************************************************
**
** Copyright (c) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import "." as Browser
import "../../" as Shared
import QtQuick 2.0

Item {
    id: searchBar

    width: parent.width
    height: isPortrait ? Settings.toolbarLarge : Settings.toolbarSmall

    property string search

    Row {
        anchors.centerIn: parent
        height: parent.height
        spacing: Theme.paddingMedium

        Shared.IconButton {
            width: Theme.iconSizeMedium + 2 * Theme.paddingMedium
            height: searchBar.height
            icon.source: "image://theme/icon-m-left"

            onTapped: {
                browserPage.findInPage(search, true, true)
            }
        }

        MouseArea {
            id: touchArea

            height: parent.height
            width: textLabel.width
            property bool down: pressed && containsMouse

            Label {
                id: textLabel
                anchors.verticalCenter: parent.verticalCenter
                text: "Find: \"" + search + "\""
                color: touchArea.down ? Theme.highlightColor : Theme.primaryColor
            }

            onClicked: overlayAnimator.showOverlay()
        }

        Shared.IconButton {
            width: Theme.iconSizeMedium + 2 * Theme.paddingMedium
            height: searchBar.height
            icon.source: "image://theme/icon-m-right"

            onTapped: {
                browserPage.findInPage(search, false, true)
            }
        }
    }

    Shared.IconButton {
        anchors {
            right: parent.right
            rightMargin: Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }
        width: Theme.iconSizeMedium
        height: searchBar.height
        icon.source: "image://theme/icon-m-reset"

        onTapped: {
            searchBar.visible = false
        }
    }
}
