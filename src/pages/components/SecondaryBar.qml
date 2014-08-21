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
import Sailfish.Browser 1.0
import "." as Browser

Item {
    id: toolBarRow

    property bool bookmarked

    signal bookmarkActivePage
    signal removeActivePageFromBookmarks

    width: parent.width
    height: isPortrait ? Settings.toolbarLarge : Settings.toolbarSmall

    Row {
        anchors.centerIn: parent
        height: parent.height

        Browser.IconButton {
            width: Theme.iconSizeMedium +  Theme.paddingMedium * 2
            icon.source: bookmarked ? "image://theme/icon-m-favorite-selected" : "image://theme/icon-m-favorite"

            onClicked: {
                if (bookmarked) {
                    removeActivePageFromBookmarks()
                } else {
                    bookmarkActivePage()
                }
            }
        }

        Browser.IconButton {
            width: Theme.iconSizeMedium +  Theme.paddingMedium
            icon.source: "image://theme/icon-m-forward"
        }

        Browser.IconButton {
            width: Theme.iconSizeMedium +  Theme.paddingMedium * 2

            icon.source: "image://theme/icon-m-search"
        }

        Browser.IconButton {
            width: Theme.iconSizeMedium +  Theme.paddingMedium * 2

            icon.source: "image://theme/icon-m-mobile-network"
        }

        Browser.IconButton {
            width: Theme.iconSizeMedium + Theme.paddingMedium * 2
            icon.source: "image://theme/icon-m-add"
        }

        Browser.IconButton {
            icon.source: "image://theme/icon-m-share"
            width: Theme.iconSizeMedium + Theme.paddingMedium
        }
    }
}
