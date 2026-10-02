/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

var browserPage
var chromeHostView
var webView
var PageStatus = { "Inactive": 0, "Activating": 1, "Active": 2, "Deactivating": 3 };

(function() {
    var resumeCount = 0
    var suspendCount = 0
    var view = {
        "privateMode": false,
        "visible": true,
        "resumeView": function() { ++resumeCount },
        "suspendView": function() { ++suspendCount }
    }

    browserPage = { "status": PageStatus.Inactive, "visible": false }
    chromeHostView = view
    webView = { "foreground": true, "privateMode": false }

    updateHostedViewSuspension(view)
    equal(resumeCount, 0)
    equal(suspendCount, 1,
          "The tab overview must keep the hidden Gecko session suspended")

    // Silica shows the underlying page during a back-swipe preview without
    // changing its Inactive status until the gesture is committed.
    browserPage.visible = true
    updateHostedViewSuspension(view)
    equal(resumeCount, 1,
          "An inactive page exposed by a back-swipe preview must resume")

    browserPage.visible = false
    updateHostedViewSuspension(view)
    equal(suspendCount, 2,
          "Cancelling the preview must suspend the hidden page again")

    browserPage.visible = true
    browserPage.status = PageStatus.Activating
    updateHostedViewSuspension(view)
    equal(resumeCount, 2,
          "Returning to BrowserPage must resume before the slide starts")

    browserPage.status = PageStatus.Active
    updateHostedViewSuspension(view)
    equal(resumeCount, 3)

    browserPage.status = PageStatus.Deactivating
    updateHostedViewSuspension(view)
    equal(resumeCount, 4,
          "The outgoing web page must remain presented during the slide")
    equal(suspendCount, 2)

    // Reversing a back gesture must also preserve the partially visible page.
    browserPage.status = PageStatus.Activating
    updateHostedViewSuspension(view)
    equal(resumeCount, 5)
    equal(suspendCount, 2)

    webView.foreground = false
    updateHostedViewSuspension(view)
    equal(suspendCount, 3,
          "Backgrounding during a transition must suspend the Gecko session")

    webView.foreground = true
    view.visible = false
    updateHostedViewSuspension(view)
    equal(suspendCount, 4,
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

    browserPage.status = PageStatus.Inactive
    browserPage.visible = false
    updateHostedViewSuspension(view)
    equal(suspendCount, 5,
          "Completing the transition must suspend the now-hidden session")

    return true
})()
