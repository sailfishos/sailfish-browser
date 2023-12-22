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

    function copyUsername(username) {
        Clipboard.text = username
        //% "Username copied"
        notification.text = qsTrId("sailfish_browser-me-login_copied_username")
        notification.show()
    }

    // Should only be called with SecureAction to avoid leaking passwords
    function copyPassword(password) {
        Clipboard.text = password
        //% "Password copied"
        notification.text = qsTrId("sailfish_browser-me-login_copied_password")
        notification.show()
    }

    SilicaListView {
        id: view

        anchors.fill: parent
        model: loginFilterModel
        currentIndex: -1
        header: Column {
            width: parent.width
            PageHeader {
                //% "Passwords"
                title: qsTrId("sailfish_browser-he-passwords")
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

                FavoriteIcon {
                    id: loginsIcon
                    anchors.verticalCenter: parent.verticalCenter

                    icon: model.favicon

                    sourceSize.width: Theme.iconSizeMedium
                    sourceSize.height: Theme.iconSizeMedium
                    width: Theme.iconSizeMedium
                    height: Theme.iconSizeMedium
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

            menu: Component {
                ContextMenu {
                    id: contextMenu
                    MenuItem {
                        visible: !secureAction.available
                        //% "Copy username"
                        text: qsTrId("sailfish_browser-me-login_copy_username")
                        onClicked: copyUsername(model.username)
                    }
                    MenuItem {
                        visible: secureAction.available
                        //% "Copy"
                        text: qsTrId("sailfish_browser-me-login_copy")
                        onClicked: {
                            // Hide existing menu items
                            var content = contextMenu._contentColumn
                            for (var i = 0; i < content.children.length; i++) {
                                content.children[i].visible = false
                            }
                            // Block menu closing
                            contextMenu.closeOnActivation = false
                            // Reset highlight
                            contextMenu._setHighlightedItem(null)
                            // Add sub-menu menu items
                            copyOptions.createObject(contextMenu, {
                                                         menu: contextMenu,
                                                         username: model.username,
                                                         password: model.password
                                                     })
                        }
                    }
                    MenuItem {
                        //% "Edit"
                        text: qsTrId("sailfish_browser-me-login_edit")
                        onClicked: {
                            var page = pageStack.animatorPush("EditLoginPage.qml", {
                                                                  loginModel: loginModel,
                                                                  secureAction: secureAction,
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
                            remove(model.uid)
                        }
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
        duration: Notice.Short
        verticalOffset: -Theme.itemSizeMedium
    }

    SecureAction {
        id: secureAction
        //% "Unlock access to browser passwords"
        message: qsTrId("sailfish_browser-me-login_unlock_password_access")
    }

    Component {
        id: copyOptions

        Item {
            id: _copyOptions
            property Item menu
            property string username
            property string password

            // Order of items is reversed
            MenuItem {
                visible: secureAction.available
                //% "Copy password"
                text: qsTrId("sailfish_browser-me-login_copy_password")
                parent: menu._contentColumn // context menu touch requires menu items are children of content area
                onClicked: {
                    secureAction.perform(copyPassword.bind(null, _copyOptions.password))
                    menu.close()
                }
            }
            MenuItem {
                // "Copy username" (defined above in contextMenu)
                text: qsTrId("sailfish_browser-me-login_copy_username")
                parent: menu._contentColumn // context menu touch requires menu items are children of content area
                onClicked: {
                    copyUsername(username)
                    menu.close()
                }
            }

            Connections {
                target: menu
                onClosed: _copyOptions.destroy()
            }
        }
    }
}
