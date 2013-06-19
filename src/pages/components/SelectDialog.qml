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
    property variant allItems
    property variant selectedItems
    property bool multiple

    property bool locked: true
    property variant selected: -1

    onOpened: {
        var item
        var currentGroup

        for (var i=0; i < allItems.length; i++) {
            item = allItems[i]
            // TODO: fix SelectHelper.js to set 'group' prop so that the following lines are not needed
            if (item.isGroup) {
                currentGroup = item.label
                continue
            }
            if (currentGroup !== undefined && item.inGroup) {
                item.group = currentGroup
            } else {
                item.group = null
            }
            // TODO: fix SelectHelper.js to make 'selected' item's property so the next line wouldn't be needed
            item.selected = selectedItems[i]
            selectModel.append(item)
        }
    }

    onAccepted: {
        var result
        var counter

        if (multiple) {
            result = []
            // TODO: it would be simpler if response accepted list of selected options only (without optgroups)
            //       In this case the code would be:
            //  for (i=0; i<selectModel.count-1;i++) { result.push(selectModel.get(i).selected) }
            //  selectDialog.selected = result
            counter = 0
            for (var i=0; i < allItems.length; i++) {
                if (allItems[i].isGroup) {
                    result.push(false)
                } else {
                    result.push(selectModel.get(counter).selected)
                    counter++
                }
            }
            selectDialog.selected = result
        }
    }

    onDone: {
        selectDialog.locked = false
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
                selectModel.setProperty(index, "selected", !selected)
                if (!selectDialog.multiple) {
                    selectDialog.selected = model.id
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
