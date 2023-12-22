/****************************************************************************
**
** Copyright (c) 2021 Open Mobile Platform LLC
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0

Dialog {

    property alias historyEnabled: historyItem.visible
    property alias cookieAndSiteDataEnabled: cookieAndSiteDataItem.visible
    property alias passwordsEnabled: passwordsItem.visible
    property alias cacheEnabled: cacheItem.visible
    property alias bookmarksEnabled: bookmarksItem.visible
    property alias sitePermissionsEnabled: sitePermissionsItem.visible

    property int historyPeriod

    property int _siteDataUsage
    property int _cacheUsage

    acceptDestinationAction: PageStackAction.Pop

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height

        Column {
            id: column

            width: parent.width

            spacing: Theme.paddingMedium

            DialogHeader {
                id: header

                //% "Are you sure you want to clear the browser data?"
                title: qsTrId("sailfish_browser-ti-confirm_clearing_browser_data")
                //% "Clear"
                acceptText: qsTrId("sailfish_browser-he-clear")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*Theme.horizontalPageMargin

                //% "The following browser data will be cleared:"
                text: qsTrId("sailfish_browser-la-following_browser_data_will_be_cleared:")
                highlighted: true
                wrapMode: Text.Wrap
            }

            BrowserDataItem {
                id: historyItem

                text: {
                    switch(historyPeriod) {
                    case 1:
                        //% "Browsing history of last 24 hours"
                        return qsTrId("sailfish_browser-la-browsing_history_of_24_hours")
                    case 7:
                        //% "Browsing history of last week"
                        return qsTrId("sailfish_browser-la-browsing_history_of_last_week")
                    case 28:
                        //% "Browsing history of last 4 weeks"
                        return qsTrId("sailfish_browser-la-browsing_history_of_last_four_weeks")
                    default:
                        //% "All browsing history"
                        return qsTrId("sailfish_browser-la-all_browsing_history")
                    }
                }
            }
            BrowserDataItem {
                id: cookieAndSiteDataItem

                text: _siteDataUsage > 0
                        //% "%1 of cookie and site data"
                      ? qsTrId("sailfish_browser-la-usage_of_cookie_and_site_data").arg(Format.formatFileSize(_siteDataUsage))
                        //% "Cookie and site data"
                      : qsTrId("sailfish_browser-la-cookie_and_site_data")
            }

            BrowserDataItem {
                id: passwordsItem

                //% "Saved passwords"
                text: qsTrId("sailfish_browser-la-saved_passwords")
            }

            BrowserDataItem {
                id: cacheItem

                text: _cacheUsage > 0
                        //% "%1 of cache"
                      ? qsTrId("sailfish_browser-la-usage_of_cache").arg(Format.formatFileSize(_cacheUsage))
                        //% "Cache"
                      : qsTrId("sailfish_browser-la-cache")
            }

            BrowserDataItem {
                id: bookmarksItem

                //% "Bookmarks"
                text: qsTrId("sailfish_browser-la-bookmarks")
            }

            BrowserDataItem {
                id: sitePermissionsItem

                //% "Site permissions"
                text: qsTrId("sailfish_browser-la-site_permissions")
            }
        }
    }

    onAccepted: {
        if (historyEnabled) {
            Settings.clearHistory(historyPeriod)
        }
        if (cookieAndSiteDataEnabled) {
            Settings.clearCookiesAndSiteData()
        }
        if (passwordsEnabled) {
            Settings.clearPasswords()
        }
        if (cacheEnabled) {
            Settings.clearCache()
        }
        if (bookmarksEnabled) {
            BookmarkManager.clear()
        }
        if (sitePermissionsEnabled) {
            Settings.clearSitePermissions()
        }
    }

    Component.onCompleted: {
        Settings.calculateSiteDataSize(function(usage) { _siteDataUsage = usage})
        Settings.calculateCacheSize(function(usage) { _cacheUsage = usage})
    }
}
