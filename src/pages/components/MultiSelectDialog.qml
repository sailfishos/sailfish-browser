/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: selectDialog

    // input data
    property var options
    property QtObject webview

    onOpened: {
        for (var i=0; i < options.length; i++) {
            selectModel.append(options[i])
        }
    }

    onAccepted: {
        var result = []
        var item

        for (var i = 0; i < selectModel.count; i++) {
            item = selectModel.get(i)
            result.push({
                "selected": item.selected,
                "index": item.index
            })
        }
        webview.sendAsyncMessage("embedui:selectresponse", {"result": result})
    }

    onRejected: {
        webview.sendAsyncMessage("embedui:selectresponse", {"result": -1})
    }

    ListModel {
        id: selectModel

        property int selectedIndex: -1
    }

    SilicaListView {
        anchors.fill: parent
        model: selectModel

        header: DialogHeader {
            dialog: selectDialog
            //% "Select"
            acceptText: qsTrId("sailfish_browser-he-select")
        }

        section {
            property: "group"
            delegate: SectionHeader {
                text: section
            }
        }

        delegate: BackgroundItem {

            enabled: !disabled

            onClicked: selectModel.setProperty(index, "selected", !selected)

            Label {
                x: Theme.paddingLarge
                anchors.verticalCenter: parent.verticalCenter
                text: label
                color: {
                    if (disabled) {
                        return selected ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    } else {
                        return highlighted || selected ? Theme.highlightColor : Theme.primaryColor
                    }
                }
            }
        }
    }
}
