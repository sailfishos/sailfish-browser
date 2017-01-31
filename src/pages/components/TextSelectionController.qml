/****************************************************************************
**
** Copyright (C) 2013-2016 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0
import org.nemomobile.notifications 1.0
import org.freedesktop.contextkit 1.0

MouseArea {
    id: root

    property bool selectionVisible
    readonly property bool active: selectionVisible

    property alias contentWidth: start.contentWidth
    property alias contentHeight: start.contentHeight

    property alias contentItem: start.contentItem

    property string text
    property string searchUri
    property bool isPhoneNumber

    readonly property bool _canCall: !cellular1Status.disabled || !cellular2Status.disabled

    // keep selection range we get from engine to move the draggers with
    // selection together when panning or zooming
    property var _cssRange
    property var _selectionData

    property bool _phoneNumberSelected

    function selectionRangeUpdated(data) {
        var resolution = contentItem.resolution
        start.lineHeight = data.start.height * resolution
        end.lineHeight = data.end.height * resolution

        var startHeightShift = data.start.height / 2
        var endHeightShift = data.end.height / 2

        // Don't update root state yet.
        var state = data.src

        // Start marker
        start.fixedX = (data.start.xPos * resolution) - start.width
        start.fixedY = data.start.yPos * resolution
        if (!selectionVisible) {
            start.x = start.fixedX
            start.y = start.fixedY
        } else if ((state === "end"  || state === "reflow") && !start.dragActive) {
            start.targetPositionAnimation.start()
        }

        // End marker
        end.fixedX = data.end.xPos * resolution
        end.fixedY = data.end.yPos * resolution

        if (!selectionVisible) {
            end.x = end.fixedX
            end.y = end.fixedY
        }
        else if ((state === "end" || state === "reflow") && !end.dragActive) {
            end.targetPositionAnimation.start()
        }

        _cssRange = {
            "startX": data.start.xPos,
            "startY": data.start.yPos,
            "endX": data.end.xPos,
            "endY": data.end.yPos,
            "startHeightShift": startHeightShift,
            "endHeightShift": endHeightShift,
            "origOffsetX": contentItem.scrollableOffset.x,
            "origOffsetY": contentItem.scrollableOffset.y,
            "origResolution": resolution
        }

        _selectionData = data
        selectionVisible = true

        text = data.text
        searchUri = data.searchUri

        _phoneNumberSelected = data.isPhoneNumber
        isPhoneNumber = _canCall && _phoneNumberSelected

        root.state = state
    }

    function copy() {
        if (selectionVisible) {
            // Send a message that hits the selection.
            contentItem.sendAsyncMessage("Browser:SelectionCopy",
                                         {
                                             "xPos": _cssRange.startX + 1,
                                             "yPos": _cssRange.startY - 1,
                                         })
        }
    }

    function showNotification() {
        notification.show()
    }

    function swap() {
        // Should we implement this?
        // Feels rather good this way as well.
    }

    function clearSelection() {
        selectionVisible = false
        _cssRange = null
        contentItem.sendAsyncMessage("Browser:SelectionClose",
                                 {
                                     "clearSelection": true
                                 })

        notification.hide()
        root.destroy()
    }


    function getMarkerBaseMessage(markerTag) {
        var resolution = contentItem.resolution
        return {
            change: markerTag,
            start: {
                xPos: (start.x + start.width) / resolution,
                yPos: start.y / resolution
            },
            end: {
                xPos: end.x / resolution,
                yPos: end.y / resolution
            },
            caret: {
                xPos: 0,
                yPos: 0
            }
        }
    }

    // Selection is copied upon state change.
    onClicked: clearSelection()

    onStateChanged: {
        // Copy when selection starts and ends.
        if (state === "end" || state === "start") {
            copy()
        }
    }

    on_CanCallChanged: {
        isPhoneNumber = _canCall && _phoneNumberSelected
    }

    TextSelectionHandle {
        id: start
        markerTag: "start"

        // contentItem, contentWidth, and contentHeight are aliased
        // from root
        visible: selectionVisible
        selectionController: root
    }

    TextSelectionHandle {
        id: end

        markerTag: "end"
        contentItem: root.contentItem
        contentWidth: root.contentWidth
        contentHeight: root.contentHeight
        visible: selectionVisible
        selectionController: root
    }

    Notification {
        id: notification
        property bool published

        function show() {
            if (published) {
                close()
            } else {
                publish()
                published = true
            }
        }

        function hide() {
            if (published) {
                close()
            }
            published = false
        }

        expireTimeout: 3000
        icon: "icon-s-clipboard"

        //% "Copied to clipboard"
        previewSummary: qsTrId("sailfish_browser-la-selection_copied")
    }

    ContextProperty {
        id: cellular1Status
        property bool disabled: value == "disabled" || value == undefined
        key: "Cellular.Status"
    }
    ContextProperty {
        id: cellular2Status
        property bool disabled: value == "disabled" || value == undefined
        key: "Cellular_1.Status"
    }
}
