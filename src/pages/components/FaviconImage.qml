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

Image {
    property string favicon
    property string link

    source: favicon != "" ? favicon : WebUtils.getFaviconForUrl(link)
    height: Theme.iconSizeSmall
    width: Theme.iconSizeSmall
    asynchronous: true
    smooth: true
    fillMode: Image.PreserveAspectCrop
    sourceSize.width: Theme.iconSizeMedium
    clip: true

    onStatusChanged: {
        if (status == Image.Error) {
            // Invalidate source size. Image provider doesn't handle sourceSize properly.
            sourceSize.width = -1
            source = "image://theme/icon-m-region"
        }
    }
}
