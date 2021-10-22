/****************************************************************************
**
** Copyright (c) 2014 Jolla Ltd.
** Copyright (c) 2021 Open Mobile Platform LLC.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import QtGraphicalEffects 1.0
import Sailfish.Silica 1.0

BackgroundItem {
    id: root

    // Expose GridView for all items
    property Item view: GridView.view
    property bool destroying
    property color highlightColor: Theme.colorScheme == Theme.LightOnDark
                                   ? Theme.highlightColor
                                   : Theme.highlightFromColor(Theme.highlightColor, Theme.LightOnDark)
    // In direction so that we can break this binding when closing a tab
    implicitWidth: width
    implicitHeight: height

    enabled: !destroying

    layer.enabled: true
    layer.effect: OpacityMask {
        maskSource: Rectangle {
            width: root.width
            height: root.height
            radius: 12 * Theme.pixelRatio
            visible: false
        }
    }

    // Background item that is also a placeholder for a tab not having
    // thumbnail image.
    contentItem.width: root.implicitWidth
    contentItem.height: root.implicitHeight

    onClicked: view.activateTab(index)

    // contentItem is hidden so this cannot be children of the contentItem.
    // So, making them as siblings of the contentItem.
    data: [
        Item {
            width: root.implicitWidth
            height: root.implicitHeight
            layer.effect: PressEffect {}
            layer.enabled: _showPress

            Rectangle {
                anchors.fill: parent
                color: Theme.colorScheme === Theme.LightOnDark ? "black" : "white"

                ColorOverlay {
                    anchors.fill: parent
                    source: parent
                    color: Theme.primaryColor
                    opacity: Theme.colorScheme === Theme.LightOnDark ? Theme.opacityFaint : 0.01
                }
                ColorOverlay {
                    anchors.fill: parent
                    source: parent
                    color: root.highlightColor
                    opacity: activeTab ? 0.1 : 0.0
                }
            }

            Item {
                id: header
                width: root.implicitWidth
                height: iconHeader.height + Theme.paddingMedium * 2

                Image {
                    id: iconHeader
                    anchors {
                        left: parent.left
                        leftMargin: Theme.paddingMedium
                        verticalCenter: parent.verticalCenter
                    }

                    height: Theme.iconSizeSmall
                    width: height
                    // TODO: Add favicon
                    // source: favicon
                    cache: false
                    asynchronous: true
                }

                Label {
                    id: titleLabel

                    anchors {
                        left: iconHeader.right
                        leftMargin: Theme.paddingMedium
                        right: close.left
                        rightMargin: Theme.paddingMedium
                        verticalCenter: iconHeader.verticalCenter
                    }

                    text: title || WebUtils.displayableUrl(url)
                    verticalAlignment: Qt.AlignVCenter
                    truncationMode: TruncationMode.Fade
                    color: down || activeTab ? root.highlightColor : Theme.primaryColor
                }

                IconButton {
                    id: close

                    anchors {
                        right: parent.right
                        top: parent.top
                        verticalCenter: iconHeader.verticalCenter
                    }
                    icon.color: Theme.primaryColor
                    icon.highlightColor: root.highlightColor
                    icon.highlighted: down
                    icon.anchors.horizontalCenterOffset: Theme.paddingMedium

                    icon.source: "image://theme/icon-s-clear-opaque-cross"
                    onClicked: {
                        // Break binding, so that texture size would not change when
                        // closing tab (animating height).
                        root.implicitHeight = root.height
                        root.implicitWidth = root.width

                        destroying = true
                        removeTimer.running = true
                    }
                }
            }
            Image {
                id: image

                source: thumbnailPath
                y: header.height
                width: root.implicitWidth
                height: root.implicitHeight

                cache: false
                asynchronous: true
                opacity: status !== Image.Ready && source !== "" ? 0.0 : 1.0
                fillMode: Image.PreserveAspectCrop
                verticalAlignment: Image.AlignTop
                Behavior on opacity { FadeAnimation {} }

            }
        },
        Timer {
            id: removeTimer
            interval: 16
            onTriggered: view.closeTab(index)
        }
    ]
}
