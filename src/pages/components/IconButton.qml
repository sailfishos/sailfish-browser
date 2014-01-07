/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jollamobile.com>
**
****************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0

MouseArea {
    id: root

    property string source
    readonly property bool down: (pressed && containsMouse) || pressTimer.running

    onPressedChanged: {
        if (pressed) {
            pressTimer.start()
        }
    }
    onCanceled: pressTimer.stop()
    onSourceChanged: {
        image.source = Qt.binding(function() { return source + "?" + Theme.highlightDimmerColor })
    }

    width: Theme.itemSizeSmall; height: Theme.itemSizeSmall
    anchors.verticalCenter: parent.verticalCenter

    Image {
        id: image

        property string _highlightSource

        opacity: parent.enabled ? 1.0 : 0.4
        anchors.centerIn: parent

        function updateHighlightSource() {
            if (state === "") {
                if (source != "") {
                    _highlightSource = Qt.binding(function() { return root.source + "?" + Theme.highlightColor })
                } else {
                    _highlightSource = ""
                }
            }
        }

        onSourceChanged: updateHighlightSource()
        Component.onCompleted: updateHighlightSource()

        states: State {
            when: root.down && image._highlightSource != ""
            PropertyChanges {
                target: image
                source: image._highlightSource
            }
        }
    }
    Timer {
        id: pressTimer
        interval: 50
    }
}
