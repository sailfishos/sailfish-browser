/* SPDX-License-Identifier: MPL-2.0 */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.join(__dirname, '../../../apps/shared/ExternalUrlFlow.qml'), 'utf8');

function dispatch({ delivered = false, automatic = true, current = true,
                    fallback = 'https://example.com/sign-in' } = {}) {
    const events = [];
    const request = { id: 'request', tabId: '1', locationRevision: '2', winId: 7, fallback };
    const scope = vm.createContext({
        request, delivered, automatic,
        requests: { request },
        contentItem: {
            tabModel: { snapshot: () => [{ tabId: '1', locationRevision: current ? '2' : '3' }] },
            sendAsyncMessageToTab: (tab, topic, data) => events.push(['response', data.outcome, data.fallback]),
        },
        handler: { cancel() {} }, expiryTimer: { stop() {} },
        pageStack: { push: () => events.push(['dialog']) }, unsupportedComponent: {},
        closeFlow: () => events.push(['close']),
    });
    for (const name of ['current', 'respond', 'unsupported']) {
        const match = source.match(new RegExp('^    function ' + name + '\\([^\\n]*\\) \\{[\\s\\S]*?^    \\}', 'm'));
        assert(match, 'Production function ' + name + ' must exist');
        vm.runInContext(match[0], scope);
    }
    const handler = source.match(/^        onDispatched: \{([\s\S]*?)^        \}/m);
    assert(handler, 'Exercise the production dispatch handler');
    vm.runInContext('(function() {' + handler[1] + '})()', scope);
    return events;
}

for (const automatic of [false, true]) {
    assert.deepEqual(dispatch({ automatic }), [['response', 'fallback', 'https://example.com/sign-in']],
        'A failed dispatch follows its validated fallback and keeps the originating flow open');
    assert.deepEqual(dispatch({ automatic, fallback: '' }), [['response', 'unsupported', ''], ['dialog']],
        'A failed dispatch without fallback explains the failure');
    assert.deepEqual(dispatch({ automatic, current: false }), [['response', 'cancelled', '']],
        'Navigation cancels a stale failure without fallback or error UI');
    assert.deepEqual(dispatch({ automatic, current: false, delivered: true }), [['response', 'cancelled', '']],
        'A stale success cannot close another document');
}
assert.deepEqual(dispatch({ delivered: true }), [['response', 'dispatched', ''], ['close']],
    'Successful Sparse dispatch still closes the originating flow');
assert.deepEqual(dispatch({ delivered: true, automatic: false }), [['response', 'dispatched', '']],
    'Successful full Browser dispatch keeps its tab open');
console.log('External dispatch fallback, error UI, stale requests and successful handoff passed');
