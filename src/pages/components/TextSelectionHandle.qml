/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0

Item {
    id: handle

    width: Theme.itemSizeSmall
    height: width

    property string type
    property Item content
    property alias color: handleRect.color

    // the moving property is updated upon dragActive change to make sure
    // that Browser:SelectionMoveStart is sent out before Browser:SelectionMove
    property bool moving
    property bool dragActive: mouseArea.drag.active

    property int xAnimationLength: Theme.itemSizeExtraLarge

    function getJson() {
        var resolution = content.child.resolution
        var json = {
            "change": handle.type,
        }
        json[handle.type] = {
                "xPos": handle.type === "start" ? (x + width) / resolution : x / resolution,
                "yPos": y / resolution
        }
        return json
    }

    onDragActiveChanged: {
        if (dragActive) {
            content.child.sendAsyncMessage("Browser:SelectionMoveStart", handle.getJson())
            moving = true
        } else {
            content.child.sendAsyncMessage("Browser:SelectionMoveEnd", handle.getJson())
            moving = false
        }
    }

    onVisibleChanged: {
        if (visible) {
            showAnimationId.restart()
        }
    }

    Rectangle {
        id: handleRect

        width: Math.round(Theme.iconSizeSmall / 4) * 2 // ensure even number
        height: width
        radius: width / 2
        smooth: true
        anchors {
            bottom: parent.top
            bottomMargin: -radius
            left: type === "end" ? parent.left : undefined
            right: type === "start" ? parent.right : undefined
            leftMargin: type === "end" ? -1 * (width / 2) : 0
            rightMargin: type === "start" ? -1 * (width / 2) : 0
        }

        Rectangle {
            color: Theme.primaryColor
            width: {
                var _width = parent.width - (Theme.paddingSmall * 2)
                return _width > 0 ? _width : Theme.paddingSmall
            }
            height: width
            radius: width / 2
            smooth: true
            anchors.centerIn: parent
        }
    }

    MouseArea {
        id: mouseArea

        width: parent.width
        height: parent.height

        anchors {
            top: parent.top
            left: type === "end" ? parent.left : undefined
            right: type === "start" ? parent.right : undefined
            leftMargin: type === "end" ? -1 * (Theme.itemSizeSmall / 2) : 0
            rightMargin: type === "start" ? -1 * (Theme.itemSizeSmall / 2) : 0
        }

        drag.target: parent
        drag.axis: Drag.XandYAxis

        onPositionChanged: {
            if (timer.running || !handle.moving) {
                return
            }

            content.child.sendAsyncMessage("Browser:SelectionMove", handle.getJson())
            timer.start()
        }
    }

    Timer {
        id: timer

        interval: 200
    }

    ParallelAnimation {
        id: showAnimationId
        FadeAnimation {
            target: handle
            from: 0
            to: 1
        }
        NumberAnimation {
            target: handle
            property: "x"
            from: handle.type === "start" ? handle.x - xAnimationLength : handle.x + xAnimationLength
            to: handle.x
            duration: 200
            easing.type: Easing.InOutQuad
        }
    }
}
