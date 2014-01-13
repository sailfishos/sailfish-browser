/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

import QtQuick 2.1
import Sailfish.Silica 1.0

IconButton {
    id: closeTabButton

    property bool closeActiveTab
    readonly property real padding: Theme.paddingLarge - (closeTabButton.width - Theme.iconSizeMedium) / 2

    anchors {
        bottom: parent.bottom; bottomMargin: padding
        right: parent.right; rightMargin: padding
    }
    icon.source: "image://theme/icon-m-close"
    onClicked: {
        if (closeActiveTab) {
            browserPage.closeActiveTab(true)
        } else {
            browserPage.closeTab(index, false)
        }
    }
}
