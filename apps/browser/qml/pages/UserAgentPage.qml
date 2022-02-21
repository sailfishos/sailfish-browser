/****************************************************************************
**
** Copyright (c) 2022 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.5
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import "useragenthelper.js" as UserAgentHelper
import "components"

Page {
    UserAgentFilterModel {
        id: userAgentFilterModel
        sourceModel: UserAgentModel {
            id: userAgentModel
        }
    }

    SilicaListView {
        id: view

        anchors.fill: parent
        model: userAgentFilterModel
        currentIndex: -1
        header: Column {
            width: parent.width
            PageHeader {
                //% "User agent overrides"
                title: qsTrId("sailfish_browser-he-user_agent_overrides")
            }
            SearchField {
                width: parent.width
                //% "Search"
                placeholderText: qsTrId("sailfish_browser-ph-usera_gent_override_search")
                EnterKey.onClicked: focus = false
                onTextChanged: userAgentFilterModel.search = text
                inputMethodHints: Qt.ImhPreferLowercase | Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                visible: userAgentModel.count > 0
            }
        }

        delegate: ListItem {
            id: listItem

            width: parent.width
            contentHeight: Theme.itemSizeMedium
            ListView.onAdd: AddAnimation { target: listItem }
            ListView.onRemove: animateRemoval()

            function remove(host) {
                remorseDelete(function() {
                    userAgentModel.unsetUserAgentOverride(host)
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
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - parent.spacing - loginsIcon.width
                    Label {
                        width: parent.width
                        text: Theme.highlightText(model.host,
                                                  userAgentFilterModel.search,
                                                  Theme.highlightColor)
                        textFormat: Text.StyledText
                    }

                    Label {
                        width: parent.width
                        text: UserAgentHelper.getUserAgentString(model.userAgent, model.isKey)
                        font.pixelSize: Theme.fontSizeExtraSmall
                        elide: Text.ElideRight
                        color: Theme.secondaryColor
                    }
                }
            }

            menu: Component {
                ContextMenu {
                    MenuItem {
                        //% "Edit"
                        text: qsTrId("sailfish_browser-me-user_agent_override_edit")
                        onClicked: {
                            userAgentModel.currentHost = model.host
                            pageStack.animatorPush("SiteUserAgentPage.qml",
                                                   {
                                                       userAgentModel: userAgentModel
                                                   })
                        }
                    }
                    MenuItem {
                        //% "Delete"
                        text: qsTrId("sailfish_browser-me-user_agent_override_delete")
                        onClicked: listItem.remove(host)
                    }
                }
            }
        }

        ViewPlaceholder {
            //% "Your saved user agent overrides show up here"
            text: qsTrId("sailfish_browser-la-user_agents_none")
            enabled: userAgentModel.count === 0
        }

        VerticalScrollDecorator {
            parent: view
            flickable: view
        }
    }
}
