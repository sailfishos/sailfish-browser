/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import org.nemomobile.configuration 1.0
import org.sailfishos.browser.settings 1.0

Page {
    id: page

    property var _nameMap: ({})

    function name2index(name) {
        return _nameMap[name] !== undefined ? _nameMap[name] : 0
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: contentColumn.height

        Column {
            id: contentColumn

            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                //% "Browser"
                title: qsTrId("settings_browser-ph-browser")
            }

            TextField {
                id: homePage

                width: parent.width
                //: Label for home page text field
                //% "Home Page"
                label: qsTrId("settings_browser-la-home_page")
                text: homePageConfig.value == "about:blank" ? "" : homePageConfig.value

                //: No home page, type home page
                //% "Type home page"
                placeholderText: qsTrId("settings_browser-ph-type_home_page")
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly

                onTextChanged: homePageConfig.value = text || "about:blank"

                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }

            ComboBox {
                id: searchEngine

                width: parent.width
                //: Label for combobox that sets search engine used in browser
                //% "Search engine"
                label: qsTrId("settings_browser-la-search_engine")
                currentIndex: name2index(searchEngineConfig.value)

                menu: ContextMenu {
                    id: searchEngineMenu

                    Component {
                        id: menuItemComp

                        MenuItem {}
                    }

                    Component.onCompleted: {
                        var index = 0
                        settings.searchEngineList.forEach(function(name) {
                            var map = page._nameMap
                            // FIXME: _contentColumn should not be used to add items dynamicly
                            menuItemComp.createObject(searchEngineMenu._contentColumn, {"text": name})
                            map[name] = index
                            page._nameMap = map
                            index++
                        })
                    }
                }

                onCurrentItemChanged: {
                    if (currentItem.text !== searchEngineConfig.value) {
                        searchEngineConfig.value = currentItem.text
                    }
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                //: Button for opening privacy settings page.
                //% "Privacy"
                text: qsTrId("settings_browser-bt-privacy")
                onClicked: pageStack.push(Qt.resolvedUrl("Privacy.qml"))
            }
        }
    }

    ConfigurationValue {
        id: searchEngineConfig

        key: "/apps/sailfish-browser/settings/search_engine"
        defaultValue: "Google"

        onValueChanged: {
            if (searchEngine.currentItem.text !== value) {
                searchEngine.currentIndex = name2index(value)
            }
        }
    }

    ConfigurationValue {
        id: homePageConfig

        key: "/apps/sailfish-browser/settings/home_page"
        defaultValue: "http://jolla.com/"
    }

    BrowserSettings {
        id: settings
    }
}
