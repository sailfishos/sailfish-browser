/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.1
import Sailfish.Silica 1.0
import org.nemomobile.dbus 2.0

Item {

    /*Label {
        text: "No current downloads"
        anchors.centerIn: parent
        font.pixelSize: Theme.fontSizeExtraLarge
        color: Theme.highlightColor
    }*/

    ListModel {
        id: tranferList
        ListElement { fileName: "test.txt"; status: "active"; progress: 0.9 }
        ListElement { fileName: "xxx.mov"; status: "active"; progress: 0.7 }

        ListElement { fileName: "jotain.jpg"; status: "completed"; progress: 1.0 }

        ListElement { fileName: "cat_pic.jpg"; status: "completed"; progress: 1.0 }
    }

    SilicaListView {
        anchors.fill: parent

        model: tranferList

        section {
            property: 'status'
            delegate: SectionHeader {
                text: section == "active" ? "Active downloads" : "Finished downloads"
                height: Theme.itemSizeExtraSmall
            }
        }

        delegate: ListItem {
            width: parent.width
            height: Theme.itemSizeMedium

            Slider {
                id: progressBar
                handleVisible: false
                label: fileName
                value: progress
                visible: status == "active"
                width: parent.width
            }

            Label {
                visible: !progressBar.visible
                anchors.centerIn: parent
                text: fileName
            }
        }

        footer: Item {
            width: parent.width
            height: Theme.itemSizeMedium

            DBusInterface {
                id: settingsApp
                service: "com.jolla.settings"
                iface: "com.jolla.settings.ui"
                path: "/com/jolla/settings/ui"
            }

            Button {
                anchors.centerIn: parent
                text: "Show all downloads"
                onClicked: settingsApp.call("showTransfers", [])
            }
        }
    }
}
