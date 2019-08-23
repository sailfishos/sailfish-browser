/*
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.2
import Sailfish.Silica 1.0

Item {
    property alias key: keyLabel.text
    property alias value: valueLabel.text
    property alias valueColor: valueLabel.color
    property alias keyLabel: keyLabel
    property int keyWidth
    property bool _wrap: keyLabel.implicitWidth > keyWidth

    property bool insecure
    visible: value && value.length > 0
    width: parent.width - 2 * Theme.horizontalPageMargin
    x: Theme.horizontalPageMargin
    height: _wrap ? keyLabel.height + valueLabel.height : Math.max(keyLabel.height, valueLabel.height)

    Label {
        id: keyLabel
        color: Theme.highlightColor
        width: keyWidth
    }

    Label {
        id: valueLabel
        anchors {
            left: _wrap ? keyLabel.left : keyLabel.right
            leftMargin: _wrap ? 0 : Theme.paddingMedium
            top: _wrap ? keyLabel.bottom : keyLabel.top
        }

        width: _wrap ? parent.width : parent.width - keyLabel.width - Theme.paddingMedium
        wrapMode: Text.Wrap
        color: insecure ? Theme.errorColor : Theme.secondaryHighlightColor
        opacity: insecure ? Theme.opacityHigh : 1.0
    }
}
