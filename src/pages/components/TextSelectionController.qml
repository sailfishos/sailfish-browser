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

    property bool selectionVisible: false
    property color color

    property Item _webPage: parent
    // keep selection range we get from engine to move the draggers with
    // selection together when panning or zooming
    property variant _cssRange

    anchors.fill: parent

    function onScrollableOffsetChanged() {
        var newOffset = _webPage.scrollableOffset
        var resolution = _webPage.resolution
        var zoom = resolution / _cssRange.origResolution

        var diffX = newOffset.x - _cssRange.origOffsetX * zoom
        var diffY = newOffset.y - _cssRange.origOffsetY * zoom

        start.x = _cssRange.startX * resolution - diffX - start.width
        start.y = (_cssRange.startY - _cssRange.startHeightShift) * resolution - diffY
        end.x = _cssRange.endX * resolution - diffX
        end.y = (_cssRange.endY - _cssRange.endHeightShift) * resolution - diffY

        timer.restart()
    }

    function onSelectionRangeUpdated(data) {
        if (!data.updateStart) {
            return
        }

        var resolution = _webPage.resolution
        var startHeightShift = data.start.height / 2
        var endHeightShift = data.end.height / 2

        start.x = (data.start.xPos * resolution) - start.width
        start.y = (data.start.yPos - startHeightShift) * resolution
        end.x = data.end.xPos * resolution
        end.y = (data.end.yPos - endHeightShift) * resolution

        _cssRange = {
            "startX": data.start.xPos,
            "startY": data.start.yPos,
            "endX": data.end.xPos,
            "endY": data.end.yPos,
            "startHeightShift": startHeightShift,
            "endHeightShift": endHeightShift,
            "origOffsetX": _webPage.scrollableOffset.x,
            "origOffsetY": _webPage.scrollableOffset.y,
            "origResolution": resolution
        }

        selectionVisible = true
    }

    function onSelectionCopied(data) {
        selectionVisible = false
        _cssRange = null
        _webPage.sendAsyncMessage("Browser:SelectionClose",
                                 {
                                     "clearSelection": true
                                 })
    }

    function onContextMenuRequested(data) {
        if (data.types.indexOf("content-text") !== -1 ||
            (data.types.indexOf("input-text") !== -1 && data.string)) {
            // we want to select some content text
            _webPage.sendAsyncMessage("Browser:SelectionStart", {"xPos": data.xPos, "yPos": data.yPos})
        }
    }

    onSelectionVisibleChanged: {
        if (selectionVisible) {
            _webPage.scrollableOffsetChanged.connect(onScrollableOffsetChanged)
        } else {
            _webPage.scrollableOffsetChanged.disconnect(onScrollableOffsetChanged)
        }
    }

    TextSelectionHandle {
        id: start

        color: parent.color
        type: "start"
        content: _webPage
        visible: selectionVisible
    }

    TextSelectionHandle {
        id: end

        color: parent.color
        type: "end"
        content: _webPage
        visible: selectionVisible
    }

    Component.onCompleted: {
        _webPage.selectionRangeUpdated.connect(onSelectionRangeUpdated)
        _webPage.selectionCopied.connect(onSelectionCopied)
        _webPage.contextMenuRequested.connect(onContextMenuRequested)
    }

    Timer {
        id: timer

        interval: 100

        onTriggered: {
            if (selectionVisible) {
                _webPage.sendAsyncMessage("Browser:SelectionUpdate", {})
            }
        }
    }
}
