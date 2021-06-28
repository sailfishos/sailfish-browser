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

        animating: menuAnimation.running

        active: false
        source: menuComponent

        onAnimate: {
            if (item) {
                item.percentageClosed = 1
                menuAnimation.target = item
                menuAnimation.from = item.percentageClosed
                menuAnimation.to = 0
                menuAnimation.restart()
            } else if (replacedItem) {
                menuAnimation.target = replacedItem
                menuAnimation.from = 0
                menuAnimation.to = 1
                menuAnimation.restart()
            }
        }
    }

    NumberAnimation {
        id: menuAnimation

        running: false
        duration: 200
        easing.type: Easing.InOutQuad
        property: "percentageClosed"
    }

    Component {
        id: menuComponent

        Rectangle {
            id: menuItem

            property real percentageClosed
            readonly property real menuTop: Math.max(0, headerItem.y - menuFlickable.contentY)

            width: popUpMenu.width
            height: popUpMenu.height

            color: Theme.rgba("black", Theme.opacityLow * (1 - percentageClosed))

            SilicaFlickable {
                id: menuFlickable

                x: popUpMenu.width - width - popUpMenu.margin
                y: popUpMenu.height
                        - height
                        - popUpMenu.margin
                        + (menuItem.percentageClosed * (height - menuItem.menuTop + popUpMenu.margin))

                width: Math.max(
                            Theme.paddingLarge * widthRatio,
                            footerLoader.item ? footerLoader.item.implicitWidth : 0)
                height: Math.min(
                            popUpMenu.height - (2 * popUpMenu.margin),
                            headerItem.height + contentLoader.height + footerLoader.height)

                contentHeight: contentLoader.y + contentLoader.height + footerLoader.height

                interactive: popUpMenu.active   // Don't handle mouse events during fade out.

                Item {
                    id: headerItem

                    y: Math.max(0, footerLoader.y - height - (Screen.sizeCategory > Screen.Medium
                                ? contentLoader.height
                                : Math.min(contentLoader.height, Theme.paddingLarge * popUpMenu.heightRatio)))

                    width: menuFlickable.width
                    height: Theme.paddingLarge
                }

                Private.AnimatedLoader {
                    id: contentLoader

                    y: headerItem.y + headerItem.height

                    width: menuFlickable.width
                    height: item ? item.height : 0
                    source: popUpMenu.menuItem

                    onInitializeItem: {
                        item.width = Qt.binding(function() { return menuFlickable.width })
                    }
                }

                children: [
                    Rectangle {
                        id: background

                        y: Math.max(0, headerItem.y - menuFlickable.contentY)
                        z: -1
                        width: footerLoader.width
                        height: footerLoader.y - y

                        color: Qt.tint(
                                    popUpMenu.palette.colorScheme === Theme.LightOnDark ? "black" : "white",
                                    Theme.rgba(popUpMenu.palette.primaryColor, Theme.opacityFaint))
                    },
                    Item {
                        id: decoratorParent

                        y: background.y + headerItem.height
                        width: footerLoader.width
                        height: footerLoader.y - y

                        VerticalScrollDecorator {
                            _forcedParent: parent
                            flickable: menuFlickable

                            _sizeRatio: decoratorParent.height / (menuFlickable.contentHeight - contentLoader.y - footerLoader.height)
                            y: menuFlickable.contentHeight > menuFlickable.height + headerItem.y
                                    ? ((decoratorParent.height - height)
                                        * Math.max(0, menuFlickable.contentY - headerItem.y)
                                        / (menuFlickable.contentHeight - menuFlickable.height - headerItem.y))
                                    : 0
                        }
                    },
                    Rectangle {
                        y: Math.max(0, headerItem.y - menuFlickable.contentY)
                        width: headerItem.width
                        height: headerItem.height + Theme.paddingMedium

                        color: background.color

                        Rectangle {
                            x: (headerItem.width - width) / 2
                            y: (headerItem.height - height)

                            width: Theme.itemSizeLarge
                            height: Theme.paddingSmall

                            radius: height / 2

                            color: popUpMenu.palette.primaryColor

                            opacity: menuFlickable.contentY < headerItem.y ? 1 : Theme.opacityLow
                        }
                    },
                    MouseArea {
                        anchors.fill: footerLoader
                    },
                    Private.AnimatedLoader {
                        id: footerLoader

                        y: menuFlickable.height - height

                        width: menuFlickable.width
                        height: item ? item.height: 0

                        source: popUpMenu.footer

                        onInitializeItem: {
                            item.width = Qt.binding(function() { return menuFlickable.width })
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

                width: menuFlickable.width
                height: menuFlickable.height

                sourceItem: menuFlickable
                hideSource: true
                visible: false
            }

            Background {
                id: menuShaderItem

                // The ShaderEffectSourceItem has its size fixed to the maximum open size of the
                // menu, this is so each frame of animation doesn't have to allocate a new texture
                // of a different size when the menu expands. This matrix transforms the normalized
                // item rectangle coordinates to the visible sub rectangle of the source item.
                readonly property matrix4x4 menuSourceMatrix: Qt.matrix4x4(
                        1, 0, 0, 0,
                        0, height / menuFlickable.height, 0, 0,
                        0, 0, 1, 0,
                        0, menuItem.menuTop / menuFlickable.height, 0, 1)

                x: menuFlickable.x
                y: menuFlickable.y + menuItem.menuTop
                width: menuFlickable.width
                height: menuFlickable.height - menuItem.menuTop

                radius: popUpMenu.cornerRadius
                sourceItem: menuShaderSource
                fillMode: Background.Stretch

                material: Material {
                    vertexShader: "
                        attribute highp vec4 position;
                        attribute highp vec2 normalizedPosition;

                        uniform highp mat4 positionMatrix;

                        uniform highp mat4 menuSourceMatrix;

                        varying highp vec2 sourceCoord;

                        void backgroundMain() {
                            gl_Position = positionMatrix * position;
                            sourceCoord = (menuSourceMatrix * vec4(normalizedPosition, 0, 1)).xy;
                        }"
                    fragmentShader: "
                        uniform lowp sampler2D sourceTexture;

                        varying highp vec2 sourceCoord;

                        void backgroundMain() {
                            gl_FragColor = texture2D(sourceTexture, sourceCoord);
                        }"
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
