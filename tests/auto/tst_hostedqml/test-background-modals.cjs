/* SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0 */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.join(__dirname, '../../../apps/browser/qml/pages/BrowserPage.qml'), 'utf8');
const functions = ['queueHostedModalRequest', 'processPendingHostedModalRequests', 'expirePendingHostedModalRequests']
  .map(name => {
    const match = source.match(new RegExp('^    function ' + name + '\\([^\\n]*\\) \\{[\\s\\S]*?^    \\}', 'm'));
    assert.ok(match, name);
    return match[0];
  }).join('\n');
const shown = [], rejected = [];
const host = { selected: '1', selectTab() { assert.fail('Background modal stole selection'); } };
const context = vm.createContext({
  _activeHostedModalTarget: null, _pendingHostedModalRequests: [],
  hostedModalRequestTimer: { restart() {}, stop() {} },
  hostedModalRequestIsLive: request => !request.dead,
  hostedMessageTabIsSelected: (view, tab) => view.selected === tab,
  presentHostedModalRequest: request => { shown.push(request.tabId); return true; },
  rejectHostedModalRequest: request => rejected.push(request.tabId),
});
vm.runInContext(functions, context);
context.queueHostedModalRequest('popup', host, '2', '20', 'embed:alert', {});
assert.equal(host.selected, '1');
assert.equal(shown.length, 0);
context.processPendingHostedModalRequests(host);
assert.equal(shown.length, 0);
// Background queue entries must not starve a foreground request.
context.queueHostedModalRequest('popup', host, '1', '10', 'embed:alert', {});
context.processPendingHostedModalRequests(host);
assert.deepEqual(shown, ['1']);
host.selected = '2';
context.processPendingHostedModalRequests(host);
assert.deepEqual(shown, ['1', '2']);
context.queueHostedModalRequest('popup', host, '3', '30', 'embed:alert', {});
context.expirePendingHostedModalRequests();
assert.deepEqual(rejected, ['3']);
assert.equal(context._pendingHostedModalRequests.length, 0);
console.log('Background modal deferral, foreground routing and expiry passed');
