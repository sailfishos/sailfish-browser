import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.WebView.Controls 1.0

Page {
    id: page

    property string filterType
    property string title
    property PermissionModel model

    ListModel {
        id: capabilityModel
        ListElement {
            //% "Allowed for"
            title: qsTrId("sailfish_browser-ti-allowed_for")
            capability: PermissionManager.Allow
        }
        ListElement {
            //% "Blocked for"
            title: qsTrId("sailfish_browser-ti-blocked_for")
            capability: PermissionManager.Deny
        }
    }

    PermissionFilterProxyModel {
        id: proxyModel
        sourceModel: model
        permissionType: filterType
    }

    SilicaFlickable {
        anchors.fill: parent

        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column

            width: parent.width

            PageHeader {
                title: page.title
            }

            ExpandingSectionGroup {
                Repeater {
                    model: capabilityModel

                    ExpandingSection {
                        id: section

                        title: model.title
                        onExpandedChanged: if (expanded) proxyModel.permission = model.capability

                        content.sourceComponent: ColumnView {
                            width: section.width
                            itemHeight: Theme.itemSizeSmall
                            model: proxyModel

                            delegate: ListItem {
                                function remove() {
                                    remorseDelete(function() {
                                        proxyModel.remove(model.index)
                                    })
                                }

                                function setCapability(capability) {
                                    proxyModel.setCapability(model.index, capability)
                                }

                                width: parent.width

                                Label {
                                    text: model.uri
                                    anchors {
                                        verticalCenter: parent.verticalCenter
                                        left: parent.left
                                        right: parent.right
                                        margins: Theme.horizontalPageMargin
                                    }
                                    color: highlighted ? Theme.highlightColor : Theme.primaryColor
                                    truncationMode: TruncationMode.Fade
                                }
                                onClicked: openMenu()

                                menu: ContextMenu {
                                    MenuItem {
                                        //% "Block"
                                        text: qsTrId("sailfish_browser-me-block")
                                        visible: model.capability !== PermissionManager.Deny
                                        onClicked: setCapability(PermissionManager.Deny)
                                    }
                                    MenuItem {
                                        //% "Allow"
                                        text: qsTrId("sailfish_browser-me-allow")
                                        visible: model.capability !== PermissionManager.Allow
                                        onClicked: setCapability(PermissionManager.Allow)
                                    }
                                    MenuItem {
                                        //% "Delete"
                                        text: qsTrId("sailfish_browser-me-delete")
                                        onClicked: remove()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
