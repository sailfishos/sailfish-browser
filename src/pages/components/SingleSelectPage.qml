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

Page {
    id: selectPage

    // input data
    property var options
    property QtObject webview

    Component.onCompleted: {
        for (var i=0; i < options.length; i++) {
            selectModel.append(options[i])
            if (options[i]["selected"]) {
                selectModel.selectedIndex = options[i]["index"]
            }
        }
    }

    function selected() {
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
        pageStack.pop()
    }

    on_NavigationChanged: {
        if (_navigation == PageNavigation.Back) {
            // swiped back
            webview.sendAsyncMessage("embedui:selectresponse", {"result": -1})
        }
    }

    ListModel {
        id: selectModel

        property int selectedIndex: -1
    }

    SilicaListView {
        anchors.fill: parent
        model: selectModel

        header: PageHeader {
            //% "Select"
            title: qsTrId("sailfish_browser-he-select")
        }

        section {
            property: "group"
            delegate: SectionHeader {
                text: section
            }
        }

        delegate: BackgroundItem {

            enabled: !disabled

            onClicked: {
                if (selectModel.selectedIndex !== index) {
                    selectModel.setProperty(index, "selected", true)
                    selectModel.setProperty(selectModel.selectedIndex, "selected", false)
                    selectModel.selectedIndex = index
                }
                selectPage.selected()
            }

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
