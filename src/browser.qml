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
import "pages/components" as Browser

ApplicationWindow {
    id: window

    property bool opaqueBackground
    property var rootPage

    property var backgrounds: []

    function setBrowserCover(model) {
        if (!model || model.count === 0) {
            cover = Qt.resolvedUrl("cover/NoTabsCover.qml")
        } else {
            cover = null
        }
    }

    function findBackgroundIndexByPage(page) {
        for (var i = 0; i < backgrounds.length; ++i) {
            if (backgrounds[i].page === page) {
                return i
            }
        }
        return -1
    }

    allowedOrientations: defaultAllowedOrientations
    _defaultPageOrientations: Orientation.All
    _defaultLabelFormat: Text.PlainText
    _clippingItem.opacity: 1.0
    _resizeContent: !window.rootPage.active
    cover: null
    initialPage: Component {
        BrowserPage {
            id: browserPage

            Component.onCompleted: {
                window.webView = webView
                window.rootPage = browserPage
            }
        }
    }

    pageStack.onCurrentPageChanged: {
        var currentContainer = pageStack._currentContainer
        if (currentContainer && pageStack.currentPage !== window.rootPage) {
            var index = findBackgroundIndexByPage(pageStack.currentPage)
            if (index === -1) {
                var background = backgroundComponent.createObject(window._wallpaperItem, {
                                                                      "pageContainer": currentContainer,
                                                                      "page": pageStack.currentPage
                                                                  })
                backgrounds.push({
                                     "page": pageStack.currentPage,
                                     "background": background
                                 })
            } else {
                // Update container that is the one that moves.
                backgrounds[index].background.pageContainer = currentContainer
            }
        }
    }

    property QtObject webView
    Binding {
        when: !!_coverWindow
        target: _coverWindow
        property: "mainWindow"
        value: webView
    }

    Component {
        id: backgroundComponent
        Browser.Background {
            id: bg
            property Item pageContainer
            property Page page

            // Page and attached background is destroyed when the page gets destroyed.
            onPageChanged: {
                if (!page) {
                    var index = 0
                    for (; index < backgrounds.length; ++index) {
                        if (backgrounds[index].background === bg) {
                            break
                        }
                    }

                    if (index >= 0 && backgrounds.length > 0) {
                        backgrounds.splice(index, 1)
                    } else {
                        backgrounds = []
                    }

                    destroy()
                }
            }

            width: height
            height: Math.max(Screen.height, Screen.width)
            x: pageContainer && page && page.isPortrait ? pageContainer.x : 0
            y: pageContainer && page && page.isLandscape ? pageContainer.y : 0
            // Page can have longer life-cycle than page container in case
            // page pushed to the pagestack as an item.
            visible: pageContainer ? pageContainer.visible : 0
        }
    }
}
