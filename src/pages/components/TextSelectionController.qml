/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

import QtQuick 1.1
import Sailfish.Silica 1.0

// TODO: also this text selection controller acts as a relay of touch events to
//       the web view -> refactor to separate these two logical entities.
MouseArea {

    property bool selectionVisible: false

    property Item _webContent: parent
    property variant _engine: _webContent.child
    property variant _point

    anchors.fill: parent

    function onBrowserSingleTap(point) {
        if (selectionVisible) {
            // This is a workaround for https://bugreports.qt-project.org/browse/QTBUG-25194
            _point = point
            _engine.sendAsyncMessage("Browser:SelectionCopy",
                                     {
                                         "xPos": _point.x,
                                         "yPos": _point.y
                                     })
        }
    }

    function onViewAreaChanged() {
        _engine.sendAsyncMessage("Browser:SelectionUpdate", {})
    }

    function onSelectionRangeUpdated(data) {
        if (!data.updateStart) {
            return
        }

        var resolution = _engine.resolution
        startSelectionHandle.x = (data.start.xPos * resolution) - startSelectionHandle.width
        startSelectionHandle.y = data.start.yPos * resolution
        endSelectionHandle.x = data.end.xPos * resolution
        endSelectionHandle.y = data.end.yPos * resolution
        selectionVisible = true
    }

    function onSelectionCopied(data) {
        selectionVisible = false
        _engine.sendAsyncMessage("Browser:SelectionClose",
                                 {
                                     "clearSelection": true
                                 })
    }

    function onContextMenuRequested(data) {
        if (data.types.indexOf("content-text") !== -1) {
            // we want to select some content text
            _engine.sendAsyncMessage("Browser:SelectionStart", {"xPos": data.xPos, "yPos": data.yPos})
        }
    }

    onPressed: {
        _engine.recvMousePress(mouseX, mouseY)
    }
    onReleased: {
        _engine.recvMouseRelease(mouseX, mouseY)
    }
    onPositionChanged: {
        _engine.recvMouseMove(mouseX, mouseY)
    }

    onSelectionVisibleChanged: {
        if (selectionVisible) {
            _engine.viewAreaChanged.connect(onViewAreaChanged)
        } else {
            _engine.viewAreaChanged.disconnect(onViewAreaChanged)
        }
    }

    TextSelectionHandle {
        id: startSelectionHandle

        type: "start"
        content: _webContent
        visible: selectionVisible
    }

    TextSelectionHandle {
        id: endSelectionHandle

        type: "end"
        content: _webContent
        visible: selectionVisible
    }

    Component.onCompleted: {
        _engine.handleSingleTap.connect(onBrowserSingleTap)
        _webContent.selectionRangeUpdated.connect(onSelectionRangeUpdated)
        _webContent.selectionCopied.connect(onSelectionCopied)
        _webContent.contextMenuRequested.connect(onContextMenuRequested)
    }
}
