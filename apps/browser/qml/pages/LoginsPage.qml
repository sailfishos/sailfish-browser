/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import "components"

Page {
    LoginFilterModel {
        id: loginFilterModel
        sourceModel: LoginModel {
            id: loginModel
        }
    }

    function stripPrefix(hostname) {
        if (hostname.length < 7) {
            return hostname
        }
        if (hostname.substring(0, 8) === "https://") {
            return hostname.substring(8)
        } else if (hostname.substring(0, 7) === "http://") {
            return hostname.substring(7)
        }
        return hostname
    }

    SilicaListView {
        id: view

        anchors.fill: parent
        model: loginFilterModel
        currentIndex: -1
        header: Column {
            width: parent.width
            PageHeader {
                //% "Logins and passwords"
                title: qsTrId("sailfish_browser-he-logins")
            }
            SearchField {
                width: parent.width
                //% "Search"
                placeholderText: qsTrId("sailfish_browser-ph-logins_search")
                EnterKey.onClicked: focus = false
                onTextChanged: loginFilterModel.search = text
                visible: loginModel.count > 0
            }
        }

        delegate: ListItem {
            id: listItem

            width: parent.width
            contentHeight: Theme.itemSizeMedium
            ListView.onAdd: AddAnimation { target: listItem }
            ListView.onRemove: animateRemoval()

            function remove(uid) {
                remorseDelete(function() {
                    loginModel.remove(uid)
                })
            }

            onClicked: openMenu()

            Row {
                width: parent.width - 2 * Theme.horizontalPageMargin
                x: Theme.horizontalPageMargin
                spacing: Theme.paddingMedium
                anchors.verticalCenter: parent.verticalCenter

                Icon {
                    anchors.verticalCenter: parent.verticalCenter
                    id: loginsIcon
                    source: "image://theme/icon-m-contact"
                }

                Column {
                    id: column
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - parent.spacing - loginsIcon.width
                    Label {
                        width: parent.width
                        text: Theme.highlightText(stripPrefix(model.hostname),
                                                  loginFilterModel.search,
                                                  Theme.highlightColor)
                        textFormat: Text.StyledText
                    }
                    Label {
                        width: parent.width
                        text: model.username
                        font.pixelSize: Theme.fontSizeExtraSmall
                        color: Theme.secondaryColor
                    }
                }
            }

            menu: ContextMenu {
                MenuItem {
                    //% "Copy password"
                    text: qsTrId("sailfish_browser-me-login_copy_password")
                    onClicked: {
                        Clipboard.text = model.password
                        notification.show()
                    }
                }
                MenuItem {
                    //% "Edit"
                    text: qsTrId("sailfish_browser-me-login_edit")
                    onClicked: {
                        var page = pageStack.animatorPush("EditLoginPage.qml", {
                                                              loginModel: loginModel,
                                                              uid: model.uid,
                                                              hostname: model.hostname,
                                                              username: model.username,
                                                              password: model.password
                                                          })
                    }
                }
                MenuItem {
                    //% "Delete"
                    text: qsTrId("sailfish_browser-me-login_delete")
                    onClicked: {
                        remove(model.uid);
                    }
                }
            }
        }

        PageBusyIndicator {
            running: !loginModel.populated
        }

        ViewPlaceholder {
            //% "Your saved logins and passwords show up here"
            text: qsTrId("sailfish_browser-la-logins-none")
            enabled: loginModel.count === 0 && loginModel.populated
        }

        VerticalScrollDecorator {
            parent: view
            flickable: view
        }
    }

    Notice {
        id: notification
        property bool published
        duration: 3000
        //% "Password copied"
        text: qsTrId("sailfish_browser-me-login_copied_password")
        verticalOffset: -Theme.itemSizeMedium
    }
}
