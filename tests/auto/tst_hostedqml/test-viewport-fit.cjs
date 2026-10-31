/* SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0 */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const page = fs.readFileSync(path.join(__dirname,
    '../../../apps/browser/qml/pages/BrowserPage.qml'), 'utf8');
const apply = page.match(/^    function applyHostedViewportFitState\([^\n]*\) \{[\s\S]*?^    \}/m)[0];
const tab = { persistentId: '7', locationRevision: '12' };
const state = { ...tab, viewportFit: 'cover', safeAreaInsetUsage: 1,
    themeColor: '#191919' };
const host = { selectedTabId: '3' };
const values = { _hostedViewportFit: 'cover', _hostedSafeAreaInsetUsage: 1,
    _hostedThemeColor: '#191919' };
const changes = [];
const scope = vm.createContext({
    _hostedViewportFitStates: { '3': state },
    hostedRuntimeTabByRuntimeId: () => tab,
});
for (const key of Object.keys(values)) {
    Object.defineProperty(scope, key, {
        get() { return values[key]; },
        set(value) {
            if (value !== values[key]) changes.push([key, value]);
            values[key] = value;
        },
    });
}
vm.runInContext(apply, scope);
for (let i = 0; i < 100; ++i) scope.applyHostedViewportFitState(host);
assert.equal(changes.length, 0,
    'Repeated metadata must not temporarily remove the cutout allowance or theme');

state.themeColor = '#ffffff';
scope.applyHostedViewportFitState(host);
assert.deepEqual(changes, [['_hostedThemeColor', '#ffffff']],
    'A theme update must not invalidate unchanged viewport geometry');

for (const invalid of [null, { selectedTabId: '' }, { selectedTabId: 'missing' },
    { persistentId: 'different' }, { locationRevision: 'old' }]) {
    Object.assign(values, { _hostedViewportFit: 'cover', _hostedSafeAreaInsetUsage: 1,
        _hostedThemeColor: '#ffffff' });
    changes.length = 0;
    Object.assign(state, tab);
    const invalidHost = invalid && !('selectedTabId' in invalid) ? host : invalid;
    if (invalidHost === host) Object.assign(state, invalid);
    scope.applyHostedViewportFitState(invalidHost);
    assert.deepEqual(values, { _hostedViewportFit: '', _hostedSafeAreaInsetUsage: 0,
        _hostedThemeColor: '' }, 'Missing or stale tab metadata must still clear');
}
console.log('Viewport-fit refresh, theme update and stale-state tests passed');
