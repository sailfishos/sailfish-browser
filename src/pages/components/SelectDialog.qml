/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/


import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: selectDialog

    // input data
    property var options
    property bool multiple
    property QtObject webview

    onOpened: {
        for (var i=0; i < options.length; i++) {
            selectModel.append(options[i])
            if (!selectDialog.multiple && options[i]["selected"]) {
                selectModel.selectedIndex = options[i]["index"]
            }
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

            onClicked: {
                if (selectDialog.multiple) {
                    selectModel.setProperty(index, "selected", !selected)
                } else {
                    if (selectModel.selectedIndex !== index) {
                        selectModel.setProperty(index, "selected", true)
                        selectModel.setProperty(selectModel.selectedIndex, "selected", false)
                        selectModel.selectedIndex = index
                    }
                    selectDialog.accept()
                }
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
