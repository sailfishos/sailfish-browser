/*
 * Copyright (c) 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

var browserPage
var chromeHostView
var webView

(function() {
    var resumeCount = 0
    var suspendCount = 0
    var view = {
        "privateMode": false,
        "visible": true,
        "resumeView": function() { ++resumeCount },
        "suspendView": function() { ++suspendCount }
    }

    browserPage = { "active": false }
    chromeHostView = view
    webView = { "foreground": true, "privateMode": false }

    updateHostedViewSuspension(view)
    equal(resumeCount, 0)
    equal(suspendCount, 1,
          "The tab overview must keep the hidden Gecko session suspended")

    browserPage.active = true
    updateHostedViewSuspension(view)
    equal(resumeCount, 1,
          "Returning to BrowserPage must resume the selected session")

    webView.foreground = false
    updateHostedViewSuspension(view)
    equal(suspendCount, 2,
          "A background Browser window must suspend the Gecko session")

    webView.foreground = true
    view.visible = false
    updateHostedViewSuspension(view)
    equal(suspendCount, 3,
          "A hidden selected session must remain suspended")
    view.visible = true

    var otherViewSuspendCount = 0
    var otherView = {
        "privateMode": true,
        "visible": true,
        "resumeView": function() {
            throw new Error("A non-selected session must not resume")
        },
        "suspendView": function() { ++otherViewSuspendCount }
    }
    updateHostedViewSuspension(otherView)
    equal(otherViewSuspendCount, 1)

    return true
})()
