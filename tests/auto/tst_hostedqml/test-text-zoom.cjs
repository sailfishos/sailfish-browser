/* SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0 */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const page = fs.readFileSync(path.join(__dirname,
  '../../../apps/browser/qml/pages/BrowserPage.qml'), 'utf8');
const update = page.match(/^    function updateHostedTextZoom\([^\n]*\) \{[\s\S]*?^    \}/m)[0];
const scope = vm.createContext({});
vm.runInContext(update, scope);
function host(ids) {
  const calls = [];
  return { calls, systemTextZoom: 1.5,
    tabModel: { snapshot: () => ids.map(tabId => ({ tabId })) },
    sendAsyncMessageToTab(id, name, data) { calls.push([id, name, data.zoom]); },
  };
}
const normal = host(['11', '12']), privateHost = host(['21']);
for (const view of [normal, privateHost, null, {}]) scope.updateHostedTextZoom(view);
assert.deepEqual(normal.calls, [
  ['11', 'embedui:textZoom', 1.5], ['12', 'embedui:textZoom', 1.5],
]);
assert.deepEqual(privateHost.calls, [['21', 'embedui:textZoom', 1.5]]);
normal.calls.length = 0;
normal.systemTextZoom = 1;
normal.tabModel.snapshot = () => [{ tabId: '12' }, { tabId: '13' }];
scope.updateHostedTextZoom(normal);
assert.deepEqual(normal.calls, [
  ['12', 'embedui:textZoom', 1], ['13', 'embedui:textZoom', 1],
], 'Reset to default reaches surviving and newly restored tabs, not closed tabs');
assert.match(page, /onSystemTextZoomChanged: browserPage.updateHostedTextZoom\(chromeView\)/);
assert.match(page, /onViewInitialized: browserPage.updateHostedTextZoom\(chromeView\)/);
assert.match(page, /onRevisionChanged: \{\s*tabSession.applyRuntimeSnapshot\(false, chromeView\)\s*browserPage.updateHostedTextZoom\(chromeView\)\s*if/);
console.log('Hosted text zoom normal/private/new/restored tab tests passed');
