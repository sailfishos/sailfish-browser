/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0

MenuItem {
    id: root

    property string targetDirectory
    property string linkUrl
    property string contentType
    property int viewId

    readonly property string downloadFileTargetUrl: {
        // Hack: Evaluate binding whenever menu is shown.
        var updater = root.parent.visible

        // drop query string from URL and split to sections
        var urlSections = linkUrl.split("?")[0].split("/")
        var leafName = urlSections[urlSections.length - 1]

        if (leafName.length === 0) {
            leafName = "unnamed_file"
        }

        return WebUtils.createUniqueFileUrl(leafName, targetDirectory)
    }

    onClicked: {
        if (downloadFileTargetUrl) {
            MozContext.sendObserve("embedui:download",
                                   {
                                       "msg": "addDownload",
                                       "from": root.linkUrl,
                                       "to": downloadFileTargetUrl,
                                       "contentType": root.contentType,
                                       "viewId": root.viewId
                                   })
        }
    }
}
