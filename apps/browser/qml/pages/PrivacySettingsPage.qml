/****************************************************************************
**
** Copyright (c) 2014 - 2016 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0
import org.nemomobile.configuration 1.0
import Sailfish.Browser 1.0

Page {
    id: page

    property var remorse

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

                TextSwitch {
                    id: clearCookies
                    //% "Cookies"
                    text: qsTrId("settings_browser-la-clear_cookies")
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
                             || clearCookies.checked
                             || clearSavedPasswords.checked
                             || clearCache.checked
                             || clearBookmarks.checked
                             || clearSitePermissions.checked

                    onClicked: {
                        //: Remorse item for clearing private data
                        //% "Cleared"
                        remorse = Remorse.popupAction(page, qsTrId("settings_browser-la-cleared_private_data"),
                                                 function() {
                                                     if (clearHistory.checked) {
                                                         Settings.clearHistory()
                                                         Settings.removeAllTabs()
                                                     }
                                                     if (clearCookies.checked) {
                                                         Settings.clearCookies()
                                                     }
                                                     if (clearSavedPasswords.checked) {
                                                         Settings.clearPasswords()
                                                     }
                                                     if (clearCache.checked) {
                                                         Settings.clearCache()
                                                     }
                                                     if (clearBookmarks.checked) {
                                                         BookmarkManager.clear()
                                                     }
                                                     if (clearSitePermissions.checked) {
                                                         Settings.clearSitePermissions()
                                                     }
                                                 }
                        );
                    }
                }
            }
        }
    }
}
