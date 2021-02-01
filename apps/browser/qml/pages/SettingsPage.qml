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

            TextField {
                id: homePage
                enabled: AccessPolicy.browserEnabled

                //: Label for home page text field
                //% "Home Page"
                label: qsTrId("settings_browser-la-home_page")
                text: homePageConfig.value == "about:blank" ? "" : homePageConfig.value

                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly

                onTextChanged: homePageConfig.value = text || "about:blank"

                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }

            ComboBox {
                id: searchEngine
                enabled: AccessPolicy.browserEnabled
                width: parent.width
                //: Label for combobox that sets search engine used in browser
                //% "Search with"
                label: qsTrId("settings_browser-la-search_with")

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
