/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0
import org.nemomobile.configuration 1.0
import org.sailfishos.browser.settings 1.0

Page {
    property bool clearPrivateData: clearHistory.checked &&
                                    clearCookies.checked &&
                                    clearSavedPasswords.checked &&
                                    clearCache.checked

    RemorsePopup {
        id: clearDataRemorse
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: contentColumn.height

        Column {
            id: contentColumn
            width: parent.width

            PageHeader {
                //: Privacy settings page header
                //% "Privacy"
                title: qsTrId("settings_browser-ph-privacy")
            }

            // Add do not track switch here.

            SectionHeader {
                //: Clear private data section header
                //% "Clear private data"
                text: qsTrId("settings_browser-ph-clear_private_data")
            }

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

                onClicked: {
                    //: Remorse item for clearing private date
                    //% "Clearing"
                    clearDataRemorse.execute(qsTrId("settings_browser-la-clearing_private_data"),
                                             function() {
                                                 if (clearPrivateData) {
                                                     clearPrivateDataConfig.value = true
                                                 } else {
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
                                                 }
                                             }
                    );
                }
            }
        }
    }

    ConfigurationValue {
        id: clearPrivateDataConfig

        key: "/apps/sailfish-browser/actions/clear_private_data"
        defaultValue: false
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
}
