/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

import QtQuick 1.1
import Sailfish.Silica 1.0

Rectangle {
    id: handle

    width: 40
    height: 40
    color: "red"
    visible: false

    property string type
    property Item content

    // the moving property is updated upon dragActive change to make sure
    // that Browser:SelectionMoveStart is sent out before Browser:SelectionMove
    property bool moving
    property bool dragActive: mouseArea.drag.active

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

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        drag.target: parent
        drag.axis: Drag.XandYAxis
        drag.minimumX: 0
        drag.maximumX: content.width - handle.width
        drag.minimumY: 0
        drag.maximumY: content.height - handle.height

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
}
