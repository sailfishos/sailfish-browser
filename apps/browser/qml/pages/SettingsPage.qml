/****************************************************************************
**
** Copyright (c) 2013 - 2021 Jolla Ltd.
** Copyright (c) 2020 - 2021 Open Mobile Platform LLC.
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import org.nemomobile.configuration 1.0
import com.jolla.settings.system 1.0
import Sailfish.Policy 1.0
import Sailfish.WebEngine 1.0
import Sailfish.Pickers 1.0
import "components"

Page {
    id: page

    function removeProtocolTypeFromUri(uri) {
        if (uri.length === 0)
            return uri
       return uri.replace(/(^\w+:|^)\/\//, '')
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: contentColumn.height

        Column {
            id: contentColumn

            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                //% "Settings"
                title: qsTrId("sailfish_browser-he-settings")
            }

            DisabledByMdmBanner {
                active: !AccessPolicy.browserEnabled
            }

            ComboBox {
                id: homePage
                enabled: AccessPolicy.browserEnabled

                width: parent.width
                //: Label for home page text field
                //% "Home Page"
                label: qsTrId("settings_browser-la-home_page")
                //% "Default"
                value: homePageConfig.value === "about:blank" ? qsTrId("sailfish_browser-la-home_page_default") : removeProtocolTypeFromUri(homePageConfig.value)
                leftMargin: 2*Theme.horizontalPageMargin + homePageIcon.width
                contentHeight: Theme.itemSizeMedium

                Icon {
                    id: homePageIcon
                    source: "image://theme/icon-m-home"
                    anchors.verticalCenter: parent.verticalCenter
                    x: Theme.horizontalPageMargin
                }

                menu: ContextMenu {
                    MenuItem {
                        //% "Default home page"
                        text: qsTrId("sailfish_browser-me-home_page_default")
                        onClicked: homePageConfig.value = "about:blank"
                    }
                    MenuItem {
                        readonly property string site: removeProtocolTypeFromUri(homePageConfig.value)
                        //: Instead of %1 site address will be displayed
                        //% "Custom website %1"
                        property string title: qsTrId("sailfish_browser-me-home_page_custom").arg(homePageConfig.value === "about:blank" ? "" : site)

                        textFormat: Text.StyledText
                        color: highlighted ? Theme.highlightColor : Theme.primaryColor
                        text: Theme.highlightText(title, site, Theme.highlightColor)
                        onClicked: pageStack.animatorPush(Qt.resolvedUrl("components/AddHomePageDialog.qml"))
                    }
                }
            }

            ComboBox {
                id: searchEngine
                enabled: AccessPolicy.browserEnabled
                width: parent.width
                //: Label for combobox that sets search engine used in browser
                //% "Search with"
                label: qsTrId("settings_browser-la-search_with")
                leftMargin: 2*Theme.horizontalPageMargin + searchIcon.width
                contentHeight: Theme.itemSizeMedium

                Icon {
                    id: searchIcon
                    source: "image://theme/icon-m-search"
                    anchors.verticalCenter: parent.verticalCenter
                    x: Theme.horizontalPageMargin
                }

                menu: ContextMenu {
                    Repeater {
                        model: SearchEngineModel
                        delegate: SearchEngineMenuItem {
                            text: title
                            //: Shown on Settings -> Search engine for user installable search services
                            //% "Tap to install"
                            description: status == SearchEngineModel.Available ? qsTrId("settings_browser-la-tap_to_install") : ""
                            onClicked: {
                                if (title !== searchEngineConfig.value) {
                                    if (status == SearchEngineModel.Available) {
                                        SearchEngineModel.install(title)
                                    } else {
                                        searchEngineConfig.value = title
                                    }
                                }
                            }

                            Component.onCompleted: {
                                if (text && (text === searchEngineConfig.value)) {
                                    searchEngine.currentIndex = index
                                }
                            }
                        }
                    }
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                //: Button for opening privacy settings page.
                //% "Privacy"
                text: qsTrId("settings_browser-bt-privacy")
                enabled: AccessPolicy.browserEnabled
                onClicked: pageStack.animatorPush(Qt.resolvedUrl("PrivacySettingsPage.qml"))
            }

            TextSwitch {
                //: Label for text switch that makes all tabs closed upon closing browser application
                //% "Close all tabs on exit"
                text: qsTrId("settings_browser-la-close_all_tabs")
                //% "Upon exiting Sailfish Browser all open tabs will be closed"
                description: qsTrId("settings_browser-la-close_all_tabs_description")
                checked: closeAllTabsConfig.value
                enabled: AccessPolicy.browserEnabled

                onCheckedChanged: closeAllTabsConfig.value = checked
            }

            TextSwitch {
                //: Label for text switch that enables JavaScript globally for all tabs
                //% "Enable JavaScript"
                text: qsTrId("settings_browser-la-enable_javascript")
                description: WebEngineSettings.javascriptEnabled ?
                                 //% "Allowed (recommended)"
                                 qsTrId("settings_browser-la-enabled_javascript_description") :
                                 //% "Blocked, some sites may not work correctly"
                                 qsTrId("settings_browser-la-disable_javascript_description")
                checked: WebEngineSettings.javascriptEnabled
                enabled: AccessPolicy.browserEnabled
                onCheckedChanged: WebEngineSettings.javascriptEnabled = checked;
            }

            BackgroundItem {
                width: parent.width
                contentHeight: Theme.itemSizeMedium
                Row {
                    width: parent.width - 2*Theme.horizontalPageMargin
                    x: Theme.horizontalPageMargin
                    spacing: Theme.paddingMedium
                    anchors.verticalCenter: parent.verticalCenter

                    Icon {
                        id: permissionIcon
                        source: "image://theme/icon-m-browser-permissions"
                    }
                    Label {
                        width: parent.width - parent.spacing - permissionIcon.width
                        //% "Permissions"
                        text: qsTrId("settings_browser-la-permissions")
                        anchors.verticalCenter: permissionIcon.verticalCenter
                    }
                }
                onClicked: pageStack.push("PermissionPage.qml")
            }

            BackgroundItem {
                width: parent.width
                contentHeight: Theme.itemSizeMedium
                Row {
                    width: parent.width - 2*Theme.horizontalPageMargin
                    x: Theme.horizontalPageMargin
                    spacing: Theme.paddingMedium
                    anchors.verticalCenter: parent.verticalCenter

                    Icon {
                        id: loginsIcon
                        source: "image://theme/icon-m-contact"
                    }
                    Label {
                        width: parent.width - parent.spacing - permissionIcon.width
                        //% "Logins and passwords"
                        text: qsTrId("settings_browser-la-logins-and-passwords")
                        anchors.verticalCenter: loginsIcon.verticalCenter
                    }
                }
                onClicked: pageStack.push("LoginsPage.qml")
            }

            BrowserListItem {
                //% "Save destination"
                label: qsTrId("settings_browser-la-save_destination")
                iconSource: "image://theme/icon-m-download"
                value: {
                    if (WebEngineSettings.useDownloadDir) {
                        //% "Download to %1"
                        return qsTrId("sailfish_browser-me-download_to").arg(WebEngineSettings.downloadDir.split("/").pop())
                    } else {
                        //% "Always ask"
                        return qsTrId("sailfish_browser-me-always_ask")
                    }
                }

                description: {
                    if (WebEngineSettings.useDownloadDir) {
                        //% "Downloaded files will be saved to %1 folder"
                        return qsTrId("sailfish_browser-me-will_be_saved_to_download").arg(WebEngineSettings.downloadDir)
                    } else {
                        //% "You will be asked where to save files"
                        return qsTrId("sailfish_browser-me-you_will_be_asked_where_to_save_files")
                    }
                }

                menu: ContextMenu {
                    MenuItem {
                        //% "Select a download folder"
                        text: qsTrId("sailfish_browser-me-select_download_folder")
                        onClicked: {
                            WebEngineSettings.useDownloadDir = true
                            pageStack.animatorPush(folderPickerPage)
                        }
                    }
                    MenuItem {
                        //% "Always ask"
                        text: qsTrId("sailfish_browser-me-always_ask")
                        onClicked: WebEngineSettings.useDownloadDir = false
                    }
                }
            }
        }
    }

    ConfigurationValue {
        id: closeAllTabsConfig
        key: "/apps/sailfish-browser/settings/close_all_tabs"
        defaultValue: false
    }

    ConfigurationValue {
        id: searchEngineConfig

        key: "/apps/sailfish-browser/settings/search_engine"
        defaultValue: "Google"
    }

    ConfigurationValue {
        id: homePageConfig

        key: "/apps/sailfish-browser/settings/home_page"
        defaultValue: "http://jolla.com/"
    }

    Notice {
        id: searchInstalledNotice
        duration: 3000
        verticalOffset: -Theme.paddingLarge
    }

    Connections {
        target: SearchEngineModel

        onInstalled: {
            //% "%1 search installed"
            searchInstalledNotice.text = qsTrId("sailfish_browser-la-search_installed").arg(title)
            searchInstalledNotice.show()
        }
    }

    Component {
        id: folderPickerPage

        FolderPickerPage {
            showSystemFiles: false
            //% "Download to"
            dialogTitle: qsTrId("sailfish_browser-ti-download-to")

            onSelectedPathChanged: WebEngineSettings.downloadDir = selectedPath
        }
    }
}
