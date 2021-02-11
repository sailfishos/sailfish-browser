/*
 * Copyright (c) 2021 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.2
import QtGraphicalEffects 1.0
import Sailfish.Silica 1.0

Item {
    id: popUpMenu

    property var menuItem
    property var footer
    property bool active
    readonly property int cornerRadius: 12
    readonly property int widthRatio: 18
    readonly property int heightRatio: 28
    readonly property bool isTablet: Screen.sizeCategory > Screen.Medium
    readonly property bool _open: active
    readonly property int _menuItemHeight: isTablet
                                          ? container.contentHeight + footer.height
                                          : isPortrait ? Theme.paddingLarge * heightRatio : Screen.width - Theme.paddingMedium * 2

    signal closed

    width: Math.max(Theme.paddingLarge * widthRatio, footer.implicitWidth)
    height: Math.min(parent.height - Theme.paddingLarge * 2, _menuItemHeight)

    opacity: _open ? 1.0 : 0.0
    visible: opacity > 0.0

    Behavior on opacity { FadeAnimation {} }

    InverseMouseArea {
        anchors.fill: parent
        enabled: active
        stealPress: true
        onPressedOutside: closed()
    }

    layer.enabled: true
    layer.effect: OpacityMask {
        source: popUpMenu
        maskSource: Rectangle {
            anchors.centerIn: parent
            width: popUpMenu.width
            height: popUpMenu.height
            radius: popUpMenu.cornerRadius
            visible: false
        }
    }

    Rectangle {
        width: popUpMenu.width
        height: popUpMenu.height
        color: Theme.colorScheme === Theme.LightOnDark ? "black" : "white"
        radius: cornerRadius

        ColorOverlay {
            anchors.fill: parent
            source: parent
            color: Theme.primaryColor
            opacity: Theme.opacityFaint
        }

        SilicaFlickable {
            id: container
            width: parent.width
            height: parent.height - footer.height
            contentHeight: loader.height
            clip: true
            visible: popUpMenu.visible
            onVisibleChanged: contentY = originY

            Loader {
                id: loader
                width: parent.width
                active: popUpMenu._open
                sourceComponent: popUpMenu.menuItem
            }
            VerticalScrollDecorator {}
        }

        Loader {
            id: footer
            width: parent.width
            active: popUpMenu._open
            sourceComponent: popUpMenu.footer
            anchors.bottom: parent.bottom
        }
    }
}
