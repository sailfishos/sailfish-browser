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

    property PermissionModel model
    property alias permissionType: proxyModel.permissionType
    property string title
    property string iconSource

    property var remorse
    readonly property bool pendingRemorse: remorse ? remorse.pending : false

    PermissionFilterProxyModel {
        id: proxyModel
        onlyPermanent: true
        sourceModel: page.model
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
        model: pendingRemorse ? null : proxyModel

        delegate: ListItem {
            id: listItem

            width: parent.width

            contentHeight: Theme.itemSizeMedium

            function remove(uri, type, capability) {
                remorseDelete(function() {
                    proxyModel.remove(uri, type, capability)
                })
            }

            onClicked: openMenu()

            ValueButton {
                anchors.verticalCenter: parent.verticalCenter
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
                    onClicked: remove(model.uri, model.type, model.capability)
                }
            }
        }

        PullDownMenu {
            MenuItem {
                //% "Delete all exceptions"
                text: qsTrId("sailfish_browser-me-delete-all-exceptions")
                visible: model && (view.count > 0)
                onClicked: {
                    remorse = Remorse.popupAction(
                                page,
                                //% "Deleted exceptions"
                                qsTrId("sailfish_browser-deleted-exceptions"),
                                function() {
                                    page.model.removeAllForPermissionType(proxyModel.permissionType)
                                })
                }
            }
            MenuItem {
                //% "Add new"
                text: qsTrId("sailfish_browser-me-add-new")
                onClicked: pageStack.push(permissionCreateDialog)
            }
        }

        ViewPlaceholder {
            enabled: page.pendingRemorse || (view.count === 0)

            //% "You have no exceptions"
            text: qsTrId("sailfish_browser-la-have-no-exceptions")
        }
    }
}
