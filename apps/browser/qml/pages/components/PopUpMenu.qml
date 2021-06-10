/*
 * Copyright (c) 2021 Open Mobile Platform LLC.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

import QtQuick 2.2
import Sailfish.Silica 1.0
import Sailfish.Silica.Background 1.0
import Sailfish.Silica.private 1.0 as Private

SilicaControl {
    id: popUpMenu

    property var menuItem
    property var footer
    property alias active: menuLoader.active
    property int margin: Theme.paddingMedium
    readonly property int cornerRadius: 12
    readonly property int widthRatio: 18
    readonly property int heightRatio: 28

    signal closed

    Private.AnimatedLoader {
        id: menuLoader

        width: popUpMenu.width
        height: popUpMenu.height

        animating: menuFadeAnimation.running

        source: menuComponent

        onAnimate: {
            if (item) {
                item.opacity = 0
                menuFadeAnimation.target = item
                menuFadeAnimation.from = 0
                menuFadeAnimation.to = 1
                menuFadeAnimation.restart()
            } else if (replacedItem) {
                menuFadeAnimation.target = replacedItem
                menuFadeAnimation.from = 1
                menuFadeAnimation.to = 0
                menuFadeAnimation.restart()
            }
        }
    }

    FadeAnimation {
        id: menuFadeAnimation

        running: false
    }

    Component {
        id: menuComponent

        Item {
            width: popUpMenu.width
            height: popUpMenu.height

            SilicaFlickable {
                id: menuFlickable

                width: popUpMenu.width
                height: popUpMenu.height

                contentHeight: contentLoader.y + contentLoader.height + footerLoader.height

                interactive: popUpMenu.active   // Don't handle mouse events during fade out.

                Private.AnimatedLoader {
                    id: contentLoader

                    x: menuFlickable.width - width - popUpMenu.margin
                    y: Math.max(popUpMenu.margin, footerLoader.y - popUpMenu.margin - (Screen.sizeCategory > Screen.Medium
                                ? contentLoader.height
                                : Math.min(contentLoader.height, isPortrait
                                    ? Theme.paddingLarge * popUpMenu.heightRatio
                                    : Screen.width - Theme.paddingMedium * 2)))

                    width: footerLoader.width
                    height: item ? item.height : 0
                    source: popUpMenu.menuItem

                    onInitializeItem: {
                        item.width = Qt.binding(function() { return footerLoader.width })
                    }
                }

                children: [
                    Rectangle {
                        id: background

                        x: contentLoader.x
                        y: Math.max(popUpMenu.margin, contentLoader.y - menuFlickable.contentY)
                        z: -1
                        width: footerLoader.width
                        height: footerLoader.y - y

                        color: Qt.tint(
                                    popUpMenu.palette.colorScheme === Theme.LightOnDark ? "black" : "white",
                                    Theme.rgba(popUpMenu.palette.primaryColor, Theme.opacityFaint))
                    },
                    Item {
                        x: contentLoader.x
                        y: Math.max(popUpMenu.margin, contentLoader.y - menuFlickable.contentY)
                        width: footerLoader.width
                        height: footerLoader.y - y

                        VerticalScrollDecorator {
                            _forcedParent: parent
                            flickable: menuFlickable
                        }
                    },
                    MouseArea {
                        anchors.fill: footerLoader
                    },
                    Private.AnimatedLoader {
                        id: footerLoader

                        x: menuFlickable.width - width - popUpMenu.margin
                        y: menuFlickable.height - height - popUpMenu.margin

                        width: Math.max(Theme.paddingLarge * widthRatio, item ? item.implicitWidth : 0)
                        height: item ? item.height: 0

                        source: popUpMenu.footer

                        onInitializeItem: {
                            item.width = Qt.binding(function() { return footerLoader.width })
                        }
                    }
                ]
            }

            // To round the corners the menu is being rendered to a ShaderEffectSource which is
            // used as the source item for a Background item which composites the menu with the
            // rest of the UI with rounded corners. Using the ShaderEffectSource also means
            // overlapping items in the menu won't blend together when the menu fades in and out.
            ShaderEffectSource {
                id: menuShaderSource

                width: popUpMenu.width
                height: popUpMenu.height

                sourceItem: menuFlickable
                hideSource: true
                visible: false
            }

            Background {
                id: menuShaderItem

                readonly property color backgroundColor: Qt.tint(
                        popUpMenu.palette.colorScheme === Theme.LightOnDark ? "black" : "white",
                        Theme.rgba(popUpMenu.palette.primaryColor, Theme.opacityFaint))

                x: contentLoader.x
                y: Math.max(popUpMenu.margin, contentLoader.y - menuFlickable.contentY)
                width: footerLoader.width
                height: footerLoader.y + footerLoader.height - y

                radius: popUpMenu.cornerRadius
                sourceItem: menuShaderSource
                transformItem: __silica_applicationwindow_instance._rotatingItem
                fillMode: Background.Stretch

                material: Material {
                }
            }

            InverseMouseArea {
                anchors.fill: menuShaderItem
                enabled: popUpMenu.active
                stealPress: true
                onPressedOutside: closed()
            }
        }
    }
}
