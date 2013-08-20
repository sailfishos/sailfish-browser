/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0

Label {
    property bool largeFont: true

    x: Theme.paddingLarge
    width: parent.width - (2 * Theme.paddingLarge)
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
