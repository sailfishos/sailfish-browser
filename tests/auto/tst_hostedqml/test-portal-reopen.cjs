/* SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0 */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const apps = path.join(__dirname, '../../../apps');
function body(source, marker) {
    const start = source.indexOf(marker);
    assert(start >= 0, marker);
    const open = source.indexOf('{', start);
    let depth = 1, end = open + 1;
    for (; depth; ++end) {
        if (source[end] === '{') ++depth;
        if (source[end] === '}') --depth;
    }
    return source.slice(open + 1, end - 1);
}
const controller = fs.readFileSync(path.join(apps, 'shared/PortalFlowController.qml'), 'utf8');
const page = fs.readFileSync(path.join(apps, 'captiveportal/qml/pages/CaptivePortalPage.qml'), 'utf8');
const events = [];
const scope = vm.createContext({
    browserEnabled: false, flows: {},
    service: { failed: (id, why) => events.push(['failed', id, why]),
        closed: (id, why) => events.push(['closed', id, why]) },
    webView: { reopen: () => events.push('reopen'),
        tabModel: { newTab: () => { events.push('new'); return 1; } } },
    touch() {}, present: () => events.push('present'),
});
vm.runInContext('function open(id,url,purpose,heading) {' + body(controller, 'function open(') + '}', scope);
scope.open('blocked', 'https://example.com', 'authentication', 'Sign in');
assert.deepEqual(events.splice(0), [['failed', 'blocked', 'load'], ['closed', 'blocked', 'failed']]);
assert.equal(Object.keys(scope.flows).length, 0);
scope.browserEnabled = true;
scope.open('allowed', 'https://example.com', 'authentication', 'Sign in');
assert.deepEqual(events.splice(0), ['reopen', 'new', 'present']);
for (const sparse of [false, true]) {
    let closing = true, nativeVisible = false, chromeVisible = false;
    const state = vm.createContext({
        WebUtils: { sparse }, AccessPolicy: { browserEnabled: true },
        Qt: { application: { active: false } }, url: 'https://example.com',
        webView: {
            reopen() { closing = false; }, clearSelection() {},
            tabModel: { count: 1, activateTab: () => false, newTab: () => 1 },
            nativeWindow: { showFullScreen() { nativeVisible = true; } },
            chromeWindow: { showFullScreen() { chromeVisible = true; } },
        }, overlay: { dismiss() {} }, bringToForeground() {}, window: { activate() {} },
    });
    for (const handler of ['onOpenUrlRequested:', 'onShowChrome:']) {
        closing = true; nativeVisible = chromeVisible = false;
        vm.runInContext('(function(){' + body(page, handler) + '})()', state);
        assert.equal(closing, false, handler + ' resets retained-process shutdown');
        assert(nativeVisible && chromeVisible, handler + ' restores both windows');
    }
}
console.log('Portal policy, retained-process reopening and window restoration passed');
