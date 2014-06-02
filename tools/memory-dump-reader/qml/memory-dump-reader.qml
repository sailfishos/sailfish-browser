/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0

Rectangle {
    width: 600
    height: 800

    function sortFunc(a,b) {
        if (a.path < b.path)
            return -1
        if (a.path > b.path)
            return 1
        return 0
    }

    function calculatePathAmount(jsonObject, allocs, type) {
        var found = false
        for (var i = 0; i < allocs.length; ++i) {
            var alloc = allocs[i]
            if (jsonObject.path.indexOf(alloc.path) === 0) {
                alloc.amount += jsonObject.amount
                alloc["type"] = type
                found = true
            }
        }
        return found
    }

    function writeSummary(allocs, excludeKeys) {
        var sum = 0
        for (var i = 0; i < allocs.length; ++i) {
            var alloc = allocs[i]
            if (excludeKeys) {
                if (excludeKeys.indexOf(alloc.path) === -1)
                    sum += alloc.amount
            } else {
                sum += alloc.amount
            }
        }

        for (i = 0; i < allocs.length; ++i) {
            alloc = allocs[i]
            alloc.type = alloc.type + "," + sum
        }
    }

    function calculateMemory(json) {
        var explicitAllocationsSummary = [
                    {"path":"explicit/js-non-window", "amount": 0},
                    {"path":"explicit/window-objects", "amount": 0},
                    {"path":"explicit/heap-overhead", "amount": 0},
                    {"path":"explicit/storage", "amount": 0}
                ]

        var otherMeasurements = [
                    {"path":"decommitted", "amount": 0},
                    {"path":"js-main-runtime", "amount": 0},
                    {"path":"js-main-runtime-compartments", "amount": 0},
                    {"path":"js-main-runtime-gc-heap-committed", "amount": 0}
                ]

        var keys = ["canvas-2d-pixels", "gfx-surface-image", "gfx-surface-xlib",
                    "gfx-textures", "ghost-windows", "heap-allocated", "heap-committed",
                    "host-object-urls", "imagelib-surface-cache", "js-main-runtime-temporary-peak", "page-faults-hard",
                    "page-faults-soft", "resident", "resident-unique"]

        var excludeKeys = ["gfx-textures", "resident-unique", "heap-allocated"]

        var summary = []

        for (var i = 0; i < json.reports.length; ++i) {
            if (keys.indexOf(json.reports[i].path) > 0) {
                var alloc = json.reports[i]
                alloc["type"] = "Summary"
                summary.push(alloc)
            } else if (!calculatePathAmount(json.reports[i], explicitAllocationsSummary, "Explicit")) {
                calculatePathAmount(json.reports[i], otherMeasurements, "Other measurement")
            }
        }


        explicitAllocationsSummary.sort(sortFunc)
        otherMeasurements.sort(sortFunc)
        summary.sort(sortFunc)

        // Calculate and write summeries for explicit allocs and other measurements.
        writeSummary(explicitAllocationsSummary)
        writeSummary(otherMeasurements)

        explicitAllocationsSummary = explicitAllocationsSummary.concat(otherMeasurements)
        var tmpList = explicitAllocationsSummary.concat(summary)

        for (i = 0; i < tmpList.length; ++i) {
            listModel.append(tmpList[i])
        }
    }

    ListView {
        anchors.fill: parent
        header: Rectangle {
            width: parent.width
            height: 50
            color: "white"
            Text {
                anchors {
                    left: parent.left; leftMargin: 10
                    right: parent.right; rightMargin: 10
                }
                color: "black"
                font.pixelSize: 36
                text: "Browser Memory Summary"
            }
        }

        section.property : "type"
        section.delegate : Rectangle {
            property var sectionItem: section.split(",")
            property int size: sectionItem.length > 1 ? sectionItem[1] : 0

            color: "blue"
            height: 40
            width: parent.width
            Text {
                color: "white"
                font.bold: true
                font.pixelSize: 20
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                text: size ? (size / 1024 / 1024).toFixed(1) + " MB" : ""
            }

            Text {
                color: "white"
                font.bold: true
                font.pixelSize: 20
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                text: sectionItem[0]
            }
        }


        delegate: Rectangle {
            width: parent.width
            border.color: "black"
            border.width: 2
            height: 50
            color: "white"

            Text {
                id: amountText
                width: 100
                x: 20
                color: "black"
                font.pixelSize: 20
                text: (amount / 1024 / 1024).toFixed(1)
            }

            Text {
                id: unit
                anchors {
                    left: amountText.right; leftMargin: 10
                    right: pathText.left; rightMargin: 10
                }
                color: "black"
                font.pixelSize: 20
                text: "MB"
            }

            Text {
                id: pathText
                anchors {
                    right: parent.right
                    rightMargin: 20
                }
                text: path
                font.pixelSize: 20
                color: "black"
            }
        }

        model: ListModel {
            id: listModel
        }
    }

    Component.onCompleted: {
        var json = JSON.parse(jsonFile)
        calculateMemory(json)
    }
}
