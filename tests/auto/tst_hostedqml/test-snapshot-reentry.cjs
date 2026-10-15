/* SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0 */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.join(__dirname, '../../../apps/shared/HostedTabSession.qml'), 'utf8');
const apply = source.match(/^    function applyRuntimeSnapshot\([^\n]*\) \{[\s\S]*?^    \}/m)[0];
const events = [];
let tabs = [];
let pending = true;
const view = { tabModel: { revision: '1', snapshot: () => tabs }, selectedTabId: '' };
const scope = vm.createContext({
    view, _runtimeRestoreSent: true, privateMode: true,
    model: { applyRuntimeSnapshot() {} },
    runtimeSnapshotChanged: () => false,
    pairedRuntimeSnapshot: snapshot => snapshot,
    drainRuntimeNewTabs() {}, resolveRuntimeCloseAfterSnapshot() {},
    flushSelectedRuntimeNavigation() {},
    snapshotApplied: snapshot => events.push(['snapshot', snapshot.length]),
    pairedSnapshotApplied: snapshot => events.push(['paired', snapshot.length]),
    empty: () => events.push(['empty']),
    flushRuntimeCommands() {
        if (!pending) return;
        pending = false;
        tabs = [{ tabId: '1', persistentId: '1' }];
        view.tabModel.revision = '2';
        scope.applyRuntimeSnapshot(false);
    },
});
vm.runInContext(apply, scope);
scope.applyRuntimeSnapshot(false);
assert.deepEqual(events, [['snapshot', 1], ['paired', 1]],
    'A queued first URL must not be followed by the stale empty startup snapshot');
events.length = 0;
tabs = [];
view.tabModel.revision = '3';
scope.applyRuntimeSnapshot(false);
assert.deepEqual(events, [['snapshot', 0], ['paired', 0], ['empty']],
    'Closing the final runtime tab must still emit empty');
console.log('Snapshot reentry and final-tab close tests passed');
