/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import QtQuick.Window 2.1
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import "pages"

ApplicationWindow {
    id: window

    property bool opaqueBackground

    function setBrowserCover(model) {
        if (!model || model.count === 0) {
            cover = Qt.resolvedUrl("cover/NoTabsCover.qml")
        } else {
            cover = null
        }
    }

    allowedOrientations: defaultAllowedOrientations
    _defaultPageOrientations: Orientation.All
    _defaultLabelFormat: Text.PlainText
    _clippingItem.opacity: 1.0
    _resizeContent: false
    cover: null
    initialPage: Component {
        BrowserPage {
            Component.onCompleted: window.webView = webView
        }
    }

    property QtObject webView
    Binding {
        when: !!_coverWindow
        target: _coverWindow
        property: "mainWindow"
        value: webView
    }
}
