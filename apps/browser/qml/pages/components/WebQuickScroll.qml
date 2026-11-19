/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Silica.private 1.0 as Private

Item {
    id: root

    property Item view

    property color surfaceColor
    property real startY
    property double startTime
    property real previousY
    property double previousTime
    property bool tracking
    property bool fastFlick

    // Gecko presents the page in a separate window below the Browser chrome.
    Rectangle {
        anchors.fill: parent
        color: root.surfaceColor
        opacity: 1 - scroll.opacity
        visible: opacity > 0
    }

    SilicaFlickable {
        id: scroll

        property string animationTab
        anchors.fill: parent
        interactive: false
        quickScroll: false
        contentHeight: root.view ? height + Math.max(0, root.view.scrollableSize.height
                       - root.view.contentRect.height * root.view.resolution) : height

        onQuickScrollAnimatingChanged: {
            if (quickScrollAnimating && root.view) {
                animationTab = root.view.selectedTabId
                root.tracking = false
            }
        }
        onContentYChanged: {
            if (quickScrollAnimating && root.view && animationTab === String(root.view.selectedTabId)) {
                root.view.scrollTo(root.view.scrollableOffset.x, contentY)
            }
        }
        Binding {
            target: scroll
            property: "contentY"
            when: !scroll.quickScrollAnimating
            value: root.view ? root.view.scrollableOffset.y : 0
        }

        Private.QuickScrollArea {
            parent: scroll
            flickable: scroll
            inBounds: true
            flicking: root.tracking && root.fastFlick
        }
    }

    Connections {
        target: root.view
        onDraggingChanged: {
            if (root.view.dragging) {
                root.previousY = root.view.scrollableOffset.y
                root.previousTime = Date.now()
                if (!root.tracking) {
                    root.startY = root.previousY
                    root.startTime = root.previousTime
                    root.fastFlick = false
                    root.tracking = true
                }
            }
        }
        onSelectedTabChanged: root.tracking = false
        onScrollableOffsetChanged: {
            if (!root.tracking) return
            var now = Date.now()
            var elapsed = now - root.previousTime
            var y = root.view.scrollableOffset.y
            if (!root.view.dragging && !root.view.pinching && elapsed > 0
                    && now - root.startTime >= 600
                    && Math.abs(y - root.startY) >= 3 * root.height
                    && Math.abs(y - root.previousY) * 1000 > Screen.height * elapsed) {
                root.fastFlick = true
            }
            root.previousTime = now
            root.previousY = y
            idleTimer.restart()
        }
    }

    Timer {
        id: idleTimer
        interval: 200
        onTriggered: root.tracking = false
    }
}
