/****************************************************************************
**
** Copyright (c) 2013 - 2019 Jolla Ltd.
** Copyright (c) 2019 - 2020 Open Mobile Platform LLC.
** Contact: Vesa-Matti Hartikainen <vesa-matti.hartikainen@jollamobile.com>
** Contact: Raine Makelainen <raine.makelainen@jolla.com>
**
****************************************************************************/

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import QtQuick 2.2
import QtQuick.Window 2.2 as QuickWindow
import Sailfish.Silica 1.0
import Sailfish.Browser 1.0
import Sailfish.Policy 1.0
import com.jolla.settings.system 1.0
import "pages"
import "pages/components" as Browser

ApplicationWindow {
    id: window

    readonly property bool largeScreen: Screen.sizeCategory > Screen.Medium
    readonly property int browserVisibility: _mainWindow ? _mainWindow.visibility : QuickWindow.Window.Hidden
    readonly property bool coverMode: browserVisibility === QuickWindow.Window.Hidden ||
                                      browserVisibility === QuickWindow.Window.Minimized ||
                                      browserVisibility === QuickWindow.Window.Windowed

    property bool opaqueBackground
    property var rootPage
    property QtObject webView

    function setBrowserCover(model) {
        if (!model || model.count === 0) {
            cover = Qt.resolvedUrl("cover/NoTabsCover.qml")
        } else {
            if (cover != null && window.webView) {
              window.webView.clearSurface();
            }
            cover = null
        }
    }

    allowedOrientations: defaultAllowedOrientations
    // For non large screen fix cover to portrait.
    _defaultPageOrientations: !largeScreen && coverMode ? Orientation.Portrait : Orientation.LandscapeMask | Orientation.Portrait
    _defaultLabelFormat: Text.PlainText
    _clippingItem.opacity: 1.0
    _resizeContent: !window.rootPage.active
    _mainWindow: webView
    _backgroundVisible: false
    _opaque: false

    cover: null
    initialPage: Component {
        BrowserPage {
            id: browserPage

            Component.onCompleted: {
                window.webView = webView
                window.rootPage = browserPage
            }

            Component.onDestruction: {
                window.webView = null
            }
        }
    }

    Browser.Background {
        id: pushBackground
        parent: null
        anchors.fill: parent
        z: -1
    }

    pageStack.onCurrentPageChanged: {
        var currentContainer = pageStack._currentContainer

        // synchronous x binding does not work with the new animator-based page pushes
        // the push is performed using placeholder page, the background is handled with pushBackground above
        var isPlaceholderPage = pageStack.currentPage && pageStack.currentPage.hasOwnProperty("__placeholder")
        var newBackground = pageStack.currentPage && !pageStack.currentPage.hasOwnProperty("__hasBackground")
        if (isPlaceholderPage) {
            pushBackground.parent = pageStack.currentPage
        } else if (newBackground) {
            var background = pushBackgroundComponent.createObject(pageStack.currentPage)
            background.parent = pageStack.currentPage
        }
    }

    Component {
        id: pushBackgroundComponent
        Browser.Background {
            parent: null
            anchors.fill: parent
            z: -1
        }
    }
    
    DisabledByMdmView {
        enabled: !AccessPolicy.browserEnabled
        //% "Web browsing"
        activity: qsTrId("sailfish_browser-la-web_browsing");
        onEnabledChanged: {
            if (enabled) {
                webView.tabModel.clear()
                webView.privateMode = true
                pageStack.pop(null, PageStackAction.Immediate)
                rootPage.overlay.toolBar.showOverlay()
            } else {
                webView.privateMode = false
            }
        }
    }
}
