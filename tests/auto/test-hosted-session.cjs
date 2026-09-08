/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const qml = fs.readFileSync(path.join(__dirname, '../../apps/shared/HostedTabSession.qml'), 'utf8');
const methods = [...qml.matchAll(/^    function \w+\([^]*?^    }/gm)].map(m => m[0]).join('\n');
function session(name) {
  const calls = [];
  const scope = {
    _runtimeRestoreSent: false, _runtimeSnapshotInitialized: false,
    _pendingRuntimeCommands: [], _pendingRuntimeNavigation: null,
    runtimeNavigationTimer: { stop() {}, restart() {} },
    model: {
      loaded: true,
      runtimeRestoreBatch() { return { tabs: [], selectedIndex: -1 }; },
      runtimeIdForPersistentId(id) { assert.equal(id, '17'); return '23'; },
      cancelRuntimeTabReservation(id) { calls.push(['cancel', id]); }
    },
    view: {
      selectedTabId: '23',
      restoreTabs() { calls.push(['restore', name]); return true; },
      load(url) { calls.push(['load', url]); },
      selectTab(id) { calls.push(['select', id]); return true; },
      reload() { calls.push(['reload']); },
      newTab(url, id) { calls.push(['new', url, id]); return true; },
      tabModel: { snapshot() { return [{ tabId: '23', persistentId: '17' }]; } }
    }
  };
  vm.createContext(scope);
  vm.runInContext(methods, scope);
  return { scope, calls };
}
// Deliberately use identical IDs: ownership must come from the session,
// independently of which mode the UI currently displays.
const normal = session('normal');
const privateSession = session('private');
for (const s of [normal, privateSession]) {
  s.scope.dispatchRuntimeCommand({ type: 'navigate', persistentId: '17', url: s === normal ? 'normal' : 'private' });
  assert.equal(s.calls.length, 0, 'Queue commands until that session has restored');
}
privateSession.scope.restoreRuntimeTabs(privateSession.scope.view);
privateSession.scope._runtimeSnapshotInitialized = true;
privateSession.scope.flushRuntimeCommands();
assert.deepEqual(privateSession.calls, [['restore', 'private'], ['load', 'private']]);
assert.deepEqual(normal.calls, [], 'Private restore must not drain normal commands');
normal.scope.restoreRuntimeTabs(normal.scope.view);
normal.scope._runtimeSnapshotInitialized = true;
normal.scope.flushRuntimeCommands();
assert.deepEqual(normal.calls, [['restore', 'normal'], ['load', 'normal']]);
privateSession.scope.dispatchRuntimeCommand({ type: 'activate', persistentId: '17', reload: true });
assert.deepEqual(privateSession.calls.slice(-2), [['select', '23'], ['reload']]);
assert.equal(normal.calls.length, 2, 'Hidden-session activation must stay in its own renderer');
console.log('Hosted session ownership tests passed');
