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
import Nemo.Configuration 1.0
import com.jolla.settings.system 1.0
import Sailfish.WebEngine 1.0
import Sailfish.Pickers 1.0
import "components"

Page {
    id: page

    readonly property int _textSwitchIconCenter: Math.round((permissionIcon.width - Theme.itemSizeExtraSmall) / 2.0)

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
            bottomPadding: Theme.paddingLarge

            PageHeader {
                //% "Settings"
                title: qsTrId("sailfish_browser-he-settings")
            }

            BrowserComboBox {
                id: homePage

                readonly property bool _homePageBlank: homePageConfig.value === "about:blank"

                //: Label for home page text field
                //% "Home Page"
                label: qsTrId("settings_browser-la-home_page")
                //% "Start view"
                value: _homePageBlank ? qsTrId("sailfish_browser-la-start_view") : removeProtocolTypeFromUri(homePageConfig.value)

                currentIndex: _homePageBlank
                              ? 0 // For start view (blank)
                              : 1 // For web page

                iconSource: "image://theme/icon-m-home"

                on_HomePageBlankChanged: currentIndex = _homePageBlank ? 0 : 1

                menu: ContextMenu {
                    MenuItem {
                        //% "Start view"
                        text: qsTrId("sailfish_browser-me-start_view")
                        onClicked: homePageConfig.value = "about:blank"
                    }
                    MenuItem {
                        readonly property string site: removeProtocolTypeFromUri(homePageConfig.value)

                        property string title: {
                            if (homePage._homePageBlank || site === "") {
                                //: Shown when site is empty
                                //% "Web page"
                                return qsTrId("sailfish_browser-me-web_page_empty")
                            } else {
                                //: Instead of %1 site address will be displayed
                                //% "Web page %1"
                                qsTrId("sailfish_browser-me-web_page").arg(site)
                            }
                        }

                        textFormat: Text.StyledText
                        color: highlighted ? Theme.highlightColor : Theme.primaryColor
                        text: Theme.highlightText(title, site, Theme.highlightColor)
                        onClicked: {
                            pageStack.animatorPush(Qt.resolvedUrl("components/AddHomePageDialog.qml"),
                                                   {homePageConfig: homePageConfig})
                        }
                    }
                }
            }

            BrowserComboBox {
                id: searchEngine

                //: Label for combobox that sets search engine used in browser
                //% "Search with"
                label: qsTrId("settings_browser-la-search_with")
                iconSource: "image://theme/icon-m-search"

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

            SectionHeader {
                //: Section Header for Privacy settings
                //% "Privacy"
                text: qsTrId("settings_browser-sh-privacy")
            }

            TextSwitch {
                //: Label for text switch that makes all tabs closed upon closing browser application
                //% "Close all tabs on exit"
                text: qsTrId("settings_browser-la-close_all_tabs")
                //% "When Browser is started next time, selected home page will be loaded"
                description: qsTrId("settings_browser-la-close_all_tabs_description")
                checked: closeAllTabsConfig.value
                // Margins adjusted to align with other items on the page
                leftMargin: Theme.horizontalPageMargin + Theme.paddingLarge + _textSwitchIconCenter
                _label.anchors.leftMargin: Theme.paddingMedium + _textSwitchIconCenter

                onCheckedChanged: closeAllTabsConfig.value = checked
            }

            TextSwitch {
                checked: WebEngineSettings.doNotTrack

                //: Tell sites that I do not want to be tracked.
                //% "Do not track"
                text: qsTrId("settings_browser-la-tracking")
                //: Tell sites that I do not want to be tracked.
                //% "Tell sites that I do not want to be tracked"
                description: qsTrId("settings_browser-la-tracking_description")
                // Margins adjusted to align with other items on the page
                leftMargin: Theme.horizontalPageMargin + Theme.paddingLarge + _textSwitchIconCenter
                _label.anchors.leftMargin: Theme.paddingMedium + _textSwitchIconCenter

                onCheckedChanged: WebEngineSettings.doNotTrack = checked
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
                // Margins adjusted to align with other items on the page
                leftMargin: Theme.horizontalPageMargin + Theme.paddingLarge + _textSwitchIconCenter
                _label.anchors.leftMargin: Theme.paddingMedium + _textSwitchIconCenter

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
                        id: loginsIcon
                        source: "image://theme/icon-m-keys"
                    }
                    Label {
                        width: parent.width - parent.spacing - loginsIcon.width
                        //: The label for the button for accessing password management
                        //% "Passwords"
                        text: qsTrId("settings_browser-la-passwords")
                        anchors.verticalCenter: loginsIcon.verticalCenter
                    }
                }
                onClicked: pageStack.push("LoginsPage.qml")
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
                        id: clearBrowsingDataIcon
                        source: "image://theme/icon-m-delete"
                    }
                    Label {
                        width: parent.width - parent.spacing - clearBrowsingDataIcon.width
                        //: The label for the button for accessing clear browsing data page
                        //% "Clear browsing data"
                        text: qsTrId("settings_browser-la-clear-browsing-data")
                        anchors.verticalCenter: clearBrowsingDataIcon.verticalCenter
                    }
                }
                onClicked: pageStack.push("PrivacySettingsPage.qml", { previousPage: page })
            }

            SectionHeader {
                //: Section Header for Downloads settings
                //% "Downloads"
                text: qsTrId("settings_browser-la-downloads")
            }

            BrowserComboBox {
                //% "Save destination"
                label: qsTrId("settings_browser-la-save_destination")
                iconSource: "image://theme/icon-m-downloads"
                value: {
                    if (WebEngineSettings.useDownloadDir) {
                        //% "Download to %1"
                        return qsTrId("sailfish_browser-me-download_to").arg(WebEngineSettings.downloadDir.split("/").pop())
                    } else {
                        //% "Always ask"
                        return qsTrId("sailfish_browser-me-always_ask")
                    }
                }
                currentIndex: WebEngineSettings.useDownloadDir
                              ? 0 // for selection a download folder
                              : 1 // for always ask

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
                            // If the user will reject, choose download path as default
                            WebEngineSettings.downloadDir = StandardPaths.download
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

            SectionHeader {
                //: Section Header for Appearance settings
                //% "Appearance"
                text: qsTrId("settings_browser-la-appearance")
            }

            BrowserComboBox {
                //% "Preferred color scheme"
                label: qsTrId("settings_browser-la-color_scheme")
                iconSource: "image://theme/icon-m-night"
                currentIndex: WebEngineSettings.colorScheme

                //% "The website style to use when available"
                description: qsTrId("sailfish_browser-me-website_color_scheme")

                menu: ContextMenu {
                    MenuItem {
                        //: Option to prefer a website's light color scheme
                        //% "Light"
                        text: qsTrId("sailfish_browser-me-prefers_light_mode")
                        onClicked: WebEngineSettings.colorScheme = WebEngineSettings.PrefersLightMode
                    }
                    MenuItem {
                        //: Option to prefer a website's dark color scheme
                        //% "Dark"
                        text: qsTrId("sailfish_browser-me-prefers_dark_mode")
                        onClicked: WebEngineSettings.colorScheme = WebEngineSettings.PrefersDarkMode
                    }
                    MenuItem {
                        //: Option for the website's color scheme to match the ambience
                        //% "Match ambience"
                        text: qsTrId("sailfish_browser-me-follow_ambience")
                        onClicked: WebEngineSettings.colorScheme = WebEngineSettings.FollowsAmbience
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
