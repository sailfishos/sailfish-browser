/****************************************************************************
**
** Copyright (c) 2013 - 2019 Jolla Ltd.
** Copyright (c) 2020 Open Mobile Platform LLC.
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

ApplicationWindow {
    id: window

    readonly property bool largeScreen: Screen.sizeCategory > Screen.Medium
    readonly property int browserVisibility: _mainWindow ? _mainWindow.visibility : QuickWindow.Window.Hidden
    property bool coverMode: browserVisibility === QuickWindow.Window.Hidden ||
                             browserVisibility === QuickWindow.Window.Minimized ||
                             browserVisibility === QuickWindow.Window.Windowed

    property var rootPage
    property QtObject webView
    property alias activityDisabledByMdm: mdmView.activity

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

    pageStack.pageBackground: Component { Background {} }

    DisabledByMdmView {
        id: mdmView
        enabled: !AccessPolicy.browserEnabled
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

    Component.onCompleted: {
        if (window.hasOwnProperty("displayMode")) {
            displayMode = ApplicationDisplayMode.FullPortrait
        }
    }
}
