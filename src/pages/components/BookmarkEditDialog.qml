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
import Sailfish.Browser 1.0

UserPrompt {
    property string url
    property string title
    property int index

    property alias editedUrl: urlField.text
    property alias editedTitle: titleField.text

    canAccept: urlField.text !== "" && (urlField.text !== url || titleField.text !== title)

    //% "Edit favorite"
    title: qsTrId("sailfish_browser-he-edit_favorite")

    //: Save the bookmark/favorite
    //% "Save"
    acceptText: qsTrId("sailfish_browser-la-accept_edit")

    Column {
        width: parent.width
        spacing: Theme.paddingMedium

        TextField {
            id: titleField
            text: title
            width: parent.width
            focus: true
            //% "Enter title"
            placeholderText: qsTrId("sailfish_browser-ph-title_edit")

            //: Label for bookmark/favorite's title edit field
            //% "Title"
            label: qsTrId("sailfish_browser-la-title_editor")

            EnterKey.iconSource: "image://theme/icon-m-enter-next"
            EnterKey.onClicked: urlField.focus = true
        }

        TextField {
            id: urlField
            text: url
            width: parent.width

            //: Placeholder for textfield to edit bookmark's URL
            //% "Enter URL"
            placeholderText: qsTrId("sailfish_browser-ph-url_edit")

            //: Label for textfield to edit bookmark's URL
            //% "URL"
            label: qsTrId("sailfish_browser-la-url_editor")
            inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhUrlCharactersOnly

            EnterKey.iconSource: canAccept ? "image://theme/icon-m-enter-accept"
                                           : "image://theme/icon-m-enter-next"
            EnterKey.onClicked: {
                if (canAccept) {
                    accept()
                } else {
                    titleField.focus = true
                }
            }
        }
    }
}
