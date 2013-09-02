
import QtQuick 2.0
import Sailfish.Silica 1.0

MouseArea {
    id: root

    property string source
    property bool _showPress: (pressed && containsMouse) || pressTimer.running

    onPressedChanged: {
        if (pressed) {
            pressTimer.start()
        }
    }
    onCanceled: pressTimer.stop()
    onSourceChanged: {
        image.source = source + "?" + Theme.highlightDimmerColor
    }

    width: Theme.itemSizeSmall; height: Theme.itemSizeSmall

    Image {
        id: image

        property string _highlightSource

        opacity: parent.enabled ? 1.0 : 0.4
        anchors.centerIn: parent

        function updateHighlightSource() {
            if (state === "") {
                if (source != "") {
                    _highlightSource = root.source + "?" + Theme.highlightColor
                } else {
                    _highlightSource = ""
                }
            }
        }

        onSourceChanged: updateHighlightSource()
        Component.onCompleted: updateHighlightSource()

        states: State {
            when: root._showPress && image._highlightSource != ""
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
