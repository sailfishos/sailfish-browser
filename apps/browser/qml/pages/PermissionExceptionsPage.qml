/****************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.WebView.Controls 1.0
import "components"

Page {
    id: page

    property alias model: proxyModel.sourceModel
    property alias permissionType: proxyModel.permissionType
    property string title
    property string iconSource

    PermissionFilterProxyModel {
        id: proxyModel
        onlyPermanent: true
    }

    Component {
        id: permissionCreateDialog

        PermissionCreateDialog {
            label: page.title
            iconSource: page.iconSource
            onAccepted: proxyModel.add(uri, page.permissionType, capability)
        }
    }

    SilicaListView {
        id: view

        anchors.fill: parent
        header: PageHeader {
            title: page.title
        }
        model: proxyModel

        delegate: ListItem {
            id: listItem

            width: parent.width

            contentHeight: Theme.itemSizeMedium

            function remove(index) {
                remorseDelete(function() {
                    proxyModel.remove(index)
                })
            }

            onClicked: openMenu()

            ValueButton {
                label: model.uri
                labelColor: listItem.highlighted ? Theme.highlightColor : Theme.primaryColor
                valueColor: Theme.highlightColor
                enabled: false
                opacity: 1.0

                value: {
                    switch(model.capability) {
                    case PermissionManager.Allow:
                        //% "Allow"
                        return qsTrId("sailfish_browser-me-allow")
                    case PermissionManager.Deny:
                        //% "Block"
                        return qsTrId("sailfish_browser-me-block")
                    }
                }
            }
            menu: ContextMenu {
                MenuItem {
                    //% "Allow"
                    text: qsTrId("sailfish_browser-me-allow")
                    onClicked: proxyModel.setCapability(model.index, PermissionManager.Allow)
                }
                MenuItem {
                    //% "Block"
                    text: qsTrId("sailfish_browser-me-block")
                    onClicked: proxyModel.setCapability(model.index, PermissionManager.Deny)
                }
                MenuItem {
                    //% "Delete"
                    text: qsTrId("sailfish_browser-me-delete")
                    onClicked: remove(model.index)
                }
            }
        }

        PullDownMenu {
            MenuItem {
                //% "Add new"
                text: qsTrId("sailfish_browser-me-add-new")
                onClicked: pageStack.push(permissionCreateDialog)
            }
        }

        ViewPlaceholder {
            enabled: view.count === 0

            //% "You have no exceptions"
            text: qsTrId("sailfish_browser-la-have-no-exceptions")
        }
    }
}
