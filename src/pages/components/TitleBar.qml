/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0

BackgroundItem {
    id: backgroundItem

    property string title
    property alias url: urlLabel.text
    property bool showEmpty

    implicitHeight: texts.height
    contentItem.color: "transparent"

    Column {
        id: texts
        x: Theme.paddingSmall
        width: parent.width - Theme.paddingSmall * 2
        anchors.verticalCenter: parent.verticalCenter

        Label {
            id: titleLabel
            width: parent.width
            color: backgroundItem.highlighted ? Theme.highlightColor : Theme.primaryColor
            font.pixelSize: showEmpty ? Theme.fontSizeSmall : Theme.fontSizeExtraSmall
            font.weight: Font.Normal

            //: Placeholder text for url typing and searching
            //% "Type URL or search"
            text: showEmpty ? qsTrId("sailfish_browser-ph-type_url_or_search") : backgroundItem.title
            truncationMode: TruncationMode.Fade
        }
        Label {
            id: urlLabel
            visible: !showEmpty
            width: parent.width
            color: backgroundItem.highlighted ? Theme.highlightColor : Theme.primaryColor
            font.pixelSize: Theme.fontSizeTiny
            font.weight: Font.Normal
            truncationMode: TruncationMode.Fade
        }
    }
}
