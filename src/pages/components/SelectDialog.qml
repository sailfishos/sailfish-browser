/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/


import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Silica.theme 1.0

Dialog {
    id: selectDialog

    // input data
    property variant options
    property bool multiple
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

        delegate: ListItem {

            enabled: !disabled

            onClicked: {
                if (selectDialog.multiple) {
                    selectModel.setProperty(index, "selected", !selected)
                } else {
                    selectModel.setProperty(index, "selected", true)
                    for (var i = 0; i < selectModel.count; i++) {
                        if (i !== index) {
                            selectModel.setProperty(i, "selected", false)
                        }
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
