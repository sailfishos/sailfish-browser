/* SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0 */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const shared = path.join(__dirname, '../../../apps/shared');
const messages = [];
let listener;
function uri(value, base) {
    const url = new URL(value, base && base.spec);
    return { spec: url.href, origin: url.origin, schemeIs: scheme => url.protocol === scheme + ':' };
}
const scope = vm.createContext({
    addEventListener(name, callback, options) {
        assert.equal(name, 'click');
        assert.equal(options.capture, true);
        assert.equal(options.mozSystemGroup, true);
        listener = callback;
    },
    Services: {
        io: { newURI: (value, charset, base) => uri(value, base) },
        scriptSecurityManager: { checkSameOriginURI(a, b, reportError, fromPrivateWindow) {
            assert.equal(arguments.length, 4, 'Gecko requires the private-window argument');
            assert.equal(reportError, false);
            assert.equal(fromPrivateWindow, false, 'Sparse uses ordinary storage');
            if (a.origin !== b.origin) throw new Error('different origin');
        } },
    },
    sendAsyncMessage: (name, data) => messages.push([name, data.uri]),
});
const script = fs.readFileSync(path.join(shared, 'SparseNavigation.js'), 'utf8');
function click(href, page = 'https://login.example/start', tag = 'a', base = page) {
    const doc = { documentURIObject: uri(page), baseURIObject: uri(base) };
    const target = { nodeType: 1, localName: tag, ownerDocument: doc,
        getAttribute: name => name === 'href' ? href : null, getAttributeNS: () => null };
    let blocked = false;
    listener({ target: { nodeType: 1, localName: 'span', parentNode: target,
        getAttribute: () => null, getAttributeNS: () => null },
        preventDefault() { blocked = true; } });
    return blocked;
}
vm.runInContext(script, scope);
assert.equal(click('/next'), false, 'relative same-origin links work');
assert.equal(click('https://login.example:443/next'), false, 'default HTTPS port is same-origin');
for (const url of ['https://other.example/', 'http://login.example/',
    'https://login.example:8443/', 'https://sub.login.example/']) {
    assert.equal(click(url), true, 'outside-origin web link is blocked: ' + url);
    assert.deepEqual(messages.at(-1), ['embed:OpenLink', url]);
}
const count = messages.length;
for (const url of ['account:/done?code=token', 'mailto:user@example.com', 'intent://login/#Intent;end']) {
    assert.equal(click(url), false, 'native protocol keeps existing handoff: ' + url);
}
assert.equal(messages.length, count, 'native handoff is not intercepted');
assert.equal(click(null, 'https://login.example/start', 'form'), false, 'form submission stays internal');
assert.equal(click('/next', 'https://other.example/after-form'), true, 'form navigation does not replace root origin');
vm.runInContext(script, scope);
assert.equal(click(null, 'https://login.example/start', 'form'), false, 'first interaction can be a form');
assert.equal(click('/next', 'https://other.example/after-form'), true, 'first form establishes original origin');
vm.runInContext(script, scope);
assert.equal(click('/next', 'https://login.example/start', 'a', 'https://other.example/'), true,
    'a cross-origin base element cannot redefine the original site');

const source = fs.readFileSync(path.join(shared, 'CaptivePortalView.qml'), 'utf8');
const openLink = source.match(/^    function openPortalLink\([^\n]*\) \{[\s\S]*?^    \}/m)[0];
const events = [];
const host = vm.createContext({
    WebUtils: { sparse: true }, portalService: {}, selectedTabId: '10',
    portalFlows: {
        interceptCallback: (tab, url) => { events.push(['callback', tab, url]); return url.endsWith('/callback'); },
        link: url => { events.push(['owned-link', url]); return true; },
    },
    webView: { linkHandler: { handleLink: url => events.push(['external', url]) } },
});
vm.runInContext(openLink, host);
host.openPortalLink('https://other.example/');
assert.deepEqual(events.splice(0), [['callback', '10', 'https://other.example/']],
    'owned Sparse flow cannot reload an outside link or dispatch it back to Sparse');
host.openPortalLink('https://provider.example/callback');
assert.deepEqual(events.splice(0), [['callback', '10', 'https://provider.example/callback']],
    'owned authentication callback still reaches the caller');
host.portalService = null;
host.openPortalLink('https://other.example/');
assert.equal(events.length, 0, 'ordinary Sparse link remains blocked');
host.WebUtils.sparse = false;
host.portalService = {};
host.openPortalLink('https://other.example/');
assert.deepEqual(events.splice(0), [['owned-link', 'https://other.example/']], 'ordinary account flow retains provider navigation');
host.portalService = null;
host.openPortalLink('https://other.example/');
assert.deepEqual(events.splice(0), [['external', 'https://other.example/']], 'ordinary network portal retains external browser dispatch');
console.log('Sparse same-origin clicks, form exceptions, native handoff and caller routing passed');
