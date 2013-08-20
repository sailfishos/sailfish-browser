/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Dmitry Rozhkov <dmitry.rozhkov@jollamobile.com>
**
****************************************************************************/


import QtQuick 2.0
import Sailfish.Silica 1.0
import "../history.js" as History

ContextMenu {
    id: menu

    property string linkHref
    property string imageSrc

    // NOTE: this code does the same thing as that in embedlite-components/jscomps/HelperAppDialog.js
    //       We can't share the code here since that code is called in the context of web engine and we can't
    //       re-use it as it is due to MPL license thus this regex-less rewrite.
    function getUniqueFileName(fileName) {
        var collisionCount = 0
        var picturesDir = WebUtils.picturesDir
        var sectmp

        while (WebUtils.fileExists(picturesDir + "/" + fileName)) {
            collisionCount++
            if (collisionCount == 1) {
                // append "(2)" before the last dot in (or at the end of) the filename
                sectmp = fileName.split(".")
                if (sectmp.length > 1) {
                    sectmp[sectmp.length-2] = sectmp[sectmp.length-2] + "(2)"
                } else {
                    sectmp[0] = sectmp[0] + "(2)"
                }
                fileName = sectmp.join(".")
            } else {
                // replace the last (n) in the filename with (n+1)
                sectmp = fileName.split("(" + collisionCount + ")")
                var tmp = sectmp.pop()
                fileName = sectmp.join("(" + collisionCount + ")") + "(" + (collisionCount+1) + ")" + tmp
            }
        }

        return picturesDir + "/" + fileName
    }

    MenuItem {
        enabled: false
        truncationMode: TruncationMode.Fade
        text: linkHref ? linkHref : imageSrc
        font.pixelSize: Theme.fontSizeSmall
        color: Theme.primaryColor
    }

    MenuItem {
        visible: linkHref.length > 0
        //% "Open link in a new tab"
        text: qsTrId("sailfish_browser-me-open_link_in_new_tab")

        onClicked: {
            browserPage.newTab(linkHref, false)
        }
    }

    MenuItem {
        visible: imageSrc.length > 0
        //% "Open image in a new tab"
        text: qsTrId("sailfish_browser-me-open_image_in_new_tab")

        onClicked: {
            browserPage.newTab(imageSrc, false)
        }
    }

    MenuItem {
        visible: imageSrc.length > 0
        //: This menu item saves image to Gallery application
        //% "Save to Gallery"
        text: qsTrId("sailfish_browser-me-save_image_to_gallery")

        onClicked: {
            var urlSections = imageSrc.split("/")
            var leafName = urlSections[urlSections.length - 1]

            if (leafName.length === 0) {
                leafName = "unnamed_picture"
            }

            MozContext.sendObserve("embedui:download",
                                   {
                                       "msg": "addDownload",
                                       "from": imageSrc,
                                       "to": "file://" + getUniqueFileName(leafName)
                                   })
        }
    }
}
