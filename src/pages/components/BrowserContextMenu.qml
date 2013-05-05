/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/


import QtQuick 1.1
import Sailfish.Silica 1.0

import "../history.js" as History

ContextMenu {
    id: menu

    property string linkHref
    property string imageSrc

    MenuItem {
        visible: linkHref.length > 0
        //% "Open link in a new tab"
        text: qsTrId("sailfish_browser-me-open_link_in_new_tab")

        onClicked: {
            console.log("clicked 'open image in tab' (not fully implemeted yet)")
            History.addTab(linkHref, "")
        }
    }

    MenuItem {
        visible: imageSrc.length > 0
        //% "Open image in a new tab"
        text: qsTrId("sailfish_browser-me-open_image_in_new_tab")

        onClicked: {
            console.log("clicked 'open image in tab' (not fully implemeted yet)")
            History.addTab(imageSrc, "")
        }
    }

    MenuItem {
        visible: imageSrc.length > 0
        //% "Save by image URL as..."
        text: qsTrId("sailfish_browser-me-save_image_as")

        onClicked: console.log("clicked 'save image as' (not implemented yet)")
    }
}
