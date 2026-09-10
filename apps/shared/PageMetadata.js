/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

(function() {
    var PAGE_METADATA_MESSAGE = "embed:pageMetadata";
    var INTERNAL_PAGE_MESSAGE = "embed:internalPage";
    var RICH_ICON_MIN_WIDTH = 96;
    var lastMetadataState = "";
    var pendingIconTimer = 0;

    function topLevelContentEvent(event) {
        var target = event.originalTarget;
        return target === content.document || (target && target.ownerGlobal === content);
    }

    function iconWidth(link) {
        var width = 0;
        var sizes = link.sizes;
        if (!sizes) {
            return width;
        }

        for (var i = 0; i < sizes.length; ++i) {
            var match = /^(\d+)x(\d+)$/i.exec(sizes[i]);
            if (match && match[1] === match[2]) {
                width = Math.max(width, parseInt(match[1], 10));
            }
        }

        return width;
    }

    function iconForLink(link, rels) {
        var width = iconWidth(link);
        var type = (link.type || "").toLowerCase();
        var href = link.href || "";
        var richIcon = rels["apple-touch-icon"] || rels["apple-touch-icon-precomposed"]
                || rels["fluid-icon"] || width >= RICH_ICON_MIN_WIDTH;

        return {
            url: href,
            width: width,
            richIcon: richIcon,
            svg: type.indexOf("svg") >= 0 || /\.svg(?:$|[?#])/i.test(href)
        };
    }

    function betterIcon(icon, previous) {
        if (!previous) {
            return true;
        }
        if (icon.richIcon !== previous.richIcon) {
            return icon.richIcon;
        }
        if (icon.svg !== previous.svg) {
            return icon.svg;
        }
        if (icon.width !== previous.width) {
            return icon.width > previous.width;
        }

        return true;
    }

    function pageOriginFavicon() {
        try {
            var location = content.document.location;
            if (location.protocol === "http:" || location.protocol === "https:") {
                return location.protocol + "//" + location.host + "/favicon.ico";
            }
        } catch (e) {
        }

        return "";
    }

    function currentIcon() {
        var bestIcon = null;
        var links = content.document.querySelectorAll("link[rel]");
        for (var i = 0; i < links.length; ++i) {
            var link = links[i];
            if (!link.href || link.hasAttribute("mask")) {
                continue;
            }

            var rels = {};
            var rel = (link.rel || "").toLowerCase().split(/\s+/);
            for (var j = 0; j < rel.length; ++j) {
                rels[rel[j]] = true;
            }

            if (!rels.icon && !rels["apple-touch-icon"]
                    && !rels["apple-touch-icon-precomposed"] && !rels["fluid-icon"]) {
                continue;
            }

            var icon = iconForLink(link, rels);
            if (betterIcon(icon, bestIcon)) {
                bestIcon = icon;
            }
        }

        if (bestIcon) {
            return bestIcon;
        }

        return {
            url: pageOriginFavicon(),
            width: 0,
            richIcon: false,
            svg: false
        };
    }

    function notifyMetadata(force) {
        var icon = currentIcon();
        var metadata = {
            url: content.document.documentURI || "",
            title: content.document.title || "",
            favicon: icon.url || "",
            isRichIcon: !!icon.richIcon
        };
        var state = JSON.stringify(metadata);
        if (!force && state === lastMetadataState) {
            return;
        }

        lastMetadataState = state;
        sendAsyncMessage(PAGE_METADATA_MESSAGE, metadata);
    }

    function scheduleIconMetadata(event) {
        if (!topLevelContentEvent(event)) {
            return;
        }

        if (pendingIconTimer) {
            content.clearTimeout(pendingIconTimer);
        }
        pendingIconTimer = content.setTimeout(function() {
            pendingIconTimer = 0;
            notifyMetadata(false);
        }, 100);
    }

    function internalPageForClick(event) {
        if (!event.isTrusted || event.defaultPrevented || event.button !== 0
                || event.altKey || event.ctrlKey || event.metaKey || event.shiftKey) {
            return "";
        }

        var node = event.originalTarget;
        while (node && node !== content.document) {
            if (node.nodeType === content.Node.ELEMENT_NODE
                    && (node.localName === "a" || node.localName === "area")) {
                var href = node.href || "";
                if (href === "about:config" || href === "about:settings") {
                    return href;
                }
                return "";
            }
            node = node.parentNode;
        }

        return "";
    }

    addEventListener("DOMContentLoaded", function(event) {
        if (topLevelContentEvent(event)) {
            notifyMetadata(true);
        }
    }, true);

    addEventListener("pageshow", function(event) {
        if (topLevelContentEvent(event)) {
            notifyMetadata(true);
        }
    }, true);

    addEventListener("DOMTitleChanged", function(event) {
        if (topLevelContentEvent(event)) {
            notifyMetadata(false);
        }
    }, true);

    addEventListener("DOMLinkAdded", scheduleIconMetadata, true);
    addEventListener("DOMLinkChanged", scheduleIconMetadata, true);

    addEventListener("click", function(event) {
        var internalPage = internalPageForClick(event);
        if (!internalPage) {
            return;
        }

        event.preventDefault();
        sendAsyncMessage(INTERNAL_PAGE_MESSAGE, { url: internalPage });
    }, true);

    notifyMetadata(true);
})();
