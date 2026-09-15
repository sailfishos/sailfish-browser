/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

(function() {
    for (var privateModeIndex = 0; privateModeIndex < 2; ++privateModeIndex) {
        var privateMode = privateModeIndex !== 0
        var hostView = { "browserTabModel": { "privateMode": privateMode } }
        var target = {}
        var page = {}
        var delivered = false
        var opener = {
            "message": function(name, data) {
                equal(name, "Content:ContextMenu")
                equal(data.types[0], "content-text")
                delivered = true
                return true
            }
        }
        hostedMessageTabIsSelected = function() { return true }
        createHostedMessageTarget = function() { return target }
        browserPage = page
        window = { "pageStack": {} }
        hostedPopupOpenerComponent = {
            "createObject": function(parent, properties) {
                equal(parent, page)
                equal(properties.tabModel, hostView.browserTabModel)
                equal(properties.contentItem, target)
                return opener
            }
        }
        _activeHostedModalTarget = null

        check(presentHostedPopup(hostView, "1", "10", "Content:ContextMenu",
                                 { "types": ["content-text"] }, {}))
        check(delivered)
        equal(target.opener, opener)
    }
    return true
})()
