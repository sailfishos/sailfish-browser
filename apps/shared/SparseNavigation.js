/*
 * SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0
 */

(function(global) {
    var rootOrigin;

    // The host retains the origin across frame-script/process recreation.
    addMessageListener("Sparse:NavigationOrigin", function(message) {
        if (message.data.uri) rootOrigin = Services.io.newURI(message.data.uri, null, null);
    });

    function requestOrigin(document) {
        if (document !== content.document) return;
        var uri = document.documentURIObject;
        var topLevelWeb = content === content.top && (uri.schemeIs("http") || uri.schemeIs("https"));
        sendAsyncMessage("Sparse:NavigationOrigin", { "uri": topLevelWeb ? uri.spec : "" });
    }

    global.addEventListener("DOMWindowCreated", function(event) {
        requestOrigin(event.target);
    }, true);
    requestOrigin(content.document);

    global.addEventListener("click", function(event) {
        var node = event.target;
        while (node && node.nodeType === 1) {
            // As in captive portal, allow login forms to submit across origins.
            if (node.localName === "form") {
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
                try {
                    if (rootOrigin) {
                        Services.scriptSecurityManager.checkSameOriginURI(rootOrigin, target, false, false);
                        return;
                    }
                } catch (error) { }
                event.preventDefault();
                // The host may recognize a caller-owned authentication callback;
                // other outside-site web links remain blocked.
                sendAsyncMessage("embed:OpenLink", { "uri": target.spec });
                return;
            }
            node = node.parentNode;
        }
    }, { capture: true, mozSystemGroup: true });
})(this);
