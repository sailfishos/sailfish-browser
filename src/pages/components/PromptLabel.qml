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

Label {
    property bool largeFont: true

    x: Theme.horizontalPageMargin
    width: parent.width - (2 * Theme.horizontalPageMargin)
    anchors {
        top: parent.top
        topMargin: !largeFont ? Theme.paddingLarge : Theme.itemSizeSmall
    }
    font.pixelSize: largeFont ? Theme.fontSizeExtraLarge : Theme.fontSizeMedium
    color: Theme.highlightColor

    onContentWidthChanged: {
        // We want to get contentWidth text width only once. When wrapping
        // goes enabled we get contentWidth that is less than width.
        // Greater than ~ three liner will be rendered with smaller font.
        if (contentWidth > width * 3 && wrapMode == Text.NoWrap) {
            largeFont = false
            wrapMode = Text.Wrap
        } else if (contentWidth > width && wrapMode == Text.NoWrap) {
            wrapMode = Text.Wrap
        }
    }
}
