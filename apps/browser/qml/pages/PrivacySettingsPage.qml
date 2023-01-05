/****************************************************************************
**
** Copyright (c) 2014 - 2016 Jolla Ltd.
** Copyright (c) 2020 - 2021 Open Mobile Platform LLC.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0
import Nemo.Configuration 1.0
import Sailfish.Browser 1.0

Page {
    id: page

    property var remorse
    property var previousPage

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: contentColumn.height

        Column {
            id: contentColumn
            width: parent.width

            PageHeader {
                //: Clear private data page header
                //% "Clear private data"
                title: qsTrId("settings_browser-ph-clear_private_data")
            }

            Column {
                width: parent.width
                enabled: !(remorse && remorse.pending)

                TextSwitch {
                    id: clearHistory

                    //% "History"
                    text: qsTrId("settings_browser-la-clear_history")
                    checked: true

                    //: Description for clearing history. This will clear history and tabs.
                    //% "Clears history and open tabs"
                    description: qsTrId("settings_browser-la-clear_history_description")
                }

                ComboBox {
                     id: historyErasingComboBox

                     width: parent.width
                     //% "Clear browser history for"
                     label: qsTrId("settings_browser-la-clear_history_period")

                     menu: ContextMenu {

                         MenuItem {
                             property int period: 1
                             //% "Last 24 hours"
                             text: qsTrId("settings_browser-la-clear_history_day")
                         }

                         MenuItem {
                             property int period: 7
                             //% "Last week"
                             text: qsTrId("settings_browser-la-clear_history_week")
                         }

                         MenuItem {
                             property int period: 28
                             //% "Last 4 weeks"
                             text: qsTrId("settings_browser-la-clear_history_four_weeks")
                         }

                         MenuItem {
                             property int period: 0
                             //% "All time"
                             text: qsTrId("settings_browser-la-clear_history_everything")
                         }
                     }
                 }

                TextSwitch {
                    id: clearCookiesAndSiteData

                    //% "Cookies and site data"
                    text: qsTrId("settings_browser-la-clear_cookies_and_site_data")
                    checked: true
                }

                TextSwitch {
                    id: clearSavedPasswords
                    //% "Saved passwords"
                    text: qsTrId("settings_browser-la-clear_passwords")
                    checked: true
                }

                TextSwitch {
                    id: clearCache
                    //% "Cache"
                    text: qsTrId("settings_browser-la-clear_cache")
                    checked: true
                }

                TextSwitch {
                    id: clearBookmarks
                    //% "Bookmarks"
                    text: qsTrId("settings_browser-la-clear_bookmarks")
                    checked: true
                }

                TextSwitch {
                    id: clearSitePermissions

                    //% "Site permissions"
                    text: qsTrId("settings_browser-la-clear_site_permissions")
                    checked: true
                }

                // Spacer between Button and switches
                Item {
                    width: parent.width
                    height: Theme.paddingLarge
                }

                Button {
                    //: Button for clearing selected private data items.
                    //% "Clear"
                    text: qsTrId("settings_browser-bt-clear")
                    anchors.horizontalCenter: parent.horizontalCenter
                    enabled: clearHistory.checked
                             || clearCookiesAndSiteData.checked
                             || clearSavedPasswords.checked
                             || clearCache.checked
                             || clearBookmarks.checked
                             || clearSitePermissions.checked

                    onClicked: {
                        var page = pageStack.push(Qt.resolvedUrl("components/PrivacySettingsConfirmDialog.qml"), {
                                                      historyEnabled: clearHistory.checked,
                                                      cookieAndSiteDataEnabled: clearCookiesAndSiteData.checked,
                                                      passwordsEnabled: clearSavedPasswords.checked,
                                                      cacheEnabled: clearCache.checked,
                                                      bookmarksEnabled: clearBookmarks.checked,
                                                      sitePermissionsEnabled: clearSitePermissions.checked,
                                                      historyPeriod: historyErasingComboBox.currentItem.period,
                                                      acceptDestination: previousPage
                                                  })
                    }
                }
            }
        }
    }
}
