/*
 * Copyright (c) 2019-2020 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.2
import Sailfish.Silica 1.0

Item {
    id: root

    property real rootWidth
    property alias key: keyLabel.text
    property alias value: valueLabel.text
    property alias keyLabel: keyLabel
    property bool _wrap: valueLabel.implicitWidth > (rootWidth - keyLabel.width - Theme.paddingMedium)

    visible: value && value.length > 0
    width: _wrap
           ? rootWidth
           : keyLabel.width + valueLabel.implicitWidth + Theme.paddingMedium
    x: Theme.horizontalPageMargin
    height: Math.max(keyLabel.height, valueLabel.implicitHeight)

    Label {
        id: keyLabel

        color: Theme.secondaryHighlightColor
    }

    Label {
        id: valueLabel

        anchors {
            left: keyLabel.right
            leftMargin: Theme.paddingMedium
        }

        width: _wrap ? rootWidth - keyLabel.width - Theme.paddingMedium : implicitWidth
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        color: Theme.highlightColor
    }
}
