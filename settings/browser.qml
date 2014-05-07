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

    function name2index(name) {
        switch (name) {
            case "Google": return 0
            case "Bing": return 1
            case "Yahoo": return 2
            default: return 0
        }
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

            ComboBox {
                id: searchEngine

                width: parent.width
                //: Label for combobox that sets search engine used in browser
                //% "Search engine"
                label: qsTrId("settings_browser-la-search_engine")
                currentIndex: name2index(searchEngineConfig.value)

                menu: ContextMenu {
                    MenuItem {
                        text: "Google"
                    }
                    MenuItem {
                        text: "Bing"
                    }
                    MenuItem {
                        text: "Yahoo"
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
}
