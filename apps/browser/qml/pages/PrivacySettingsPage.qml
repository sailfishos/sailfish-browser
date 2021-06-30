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
                    enabled: clearHistory.checked || clearCookies.checked || clearSavedPasswords.checked || clearCache.checked || clearBookmarks.checked

                    onClicked: {
                        //: Remorse item for clearing private data
                        //% "Cleared"
                        remorse = Remorse.popupAction(page, qsTrId("settings_browser-la-cleared_private_data"),
                                                 function() {
                                                     if (clearHistory.checked) {
                                                         clearHistoryConfig.value = true
                                                     }
                                                     if (clearCookies.checked) {
                                                         clearCookiesConfig.value = true
                                                     }
                                                     if (clearSavedPasswords.checked) {
                                                         clearSavedPasswordsConfig.value = true
                                                     }
                                                     if (clearCache.checked) {
                                                         clearCacheConfig.value = true
                                                     }
                                                     if (clearBookmarks.checked) {
                                                         clearBookmarksConfig.value = true
                                                     }
                                                 }
                        );
                    }
                }
            }
        }
    }

    ConfigurationValue {
        id: clearHistoryConfig

        key: "/apps/sailfish-browser/actions/clear_history"
        defaultValue: false
    }

    ConfigurationValue {
        id: clearCookiesConfig

        key: "/apps/sailfish-browser/actions/clear_cookies"
        defaultValue: false
    }

    ConfigurationValue {
        id: clearSavedPasswordsConfig

        key: "/apps/sailfish-browser/actions/clear_passwords"
        defaultValue: false
    }

    ConfigurationValue {
        id: clearCacheConfig

        key: "/apps/sailfish-browser/actions/clear_cache"
        defaultValue: false
    }

    ConfigurationValue {
        id: clearBookmarksConfig

        key: "/apps/sailfish-browser/actions/clear_bookmarks"
        defaultValue: false
    }
}
