/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

(function(global) {
    var rootOrigin;

    global.addEventListener("click", function(event) {
        var node = event.target;
        while (node && node.nodeType === 1) {
            // As in captive portal, allow login forms to submit across origins.
            if (node.localName === "form") {
                if (!rootOrigin) rootOrigin = node.ownerDocument.documentURIObject;
                return;
            }
            var href = node.getAttribute("href")
                    || node.getAttributeNS("http://www.w3.org/1999/xlink", "href");
            if (href && (node.localName === "a" || node.localName === "area"
                    || node.localName === "link"
                    || node.namespaceURI === "http://www.w3.org/1998/Math/MathML")) {
                var target;
                try {
                    target = Services.io.newURI(href, null, node.ownerDocument.baseURIObject);
                } catch (error) {
                    return;
                }
                // Native-app links must reach the existing external-URL flow,
                // which checks support and closes Sparse after a successful handoff.
                if (!target.schemeIs("http") && !target.schemeIs("https")) return;
                if (!rootOrigin) rootOrigin = node.ownerDocument.documentURIObject;
                try {
                    Services.scriptSecurityManager.checkSameOriginURI(rootOrigin, target, false, false);
                    return;
                } catch (error) {
                    event.preventDefault();
                    // The host may recognize a caller-owned authentication callback;
                    // other outside-site web links remain blocked.
                    sendAsyncMessage("embed:OpenLink", { "uri": target.spec });
                    return;
                }
            }
            node = node.parentNode;
        }
    }, { capture: true, mozSystemGroup: true });
})(this);
