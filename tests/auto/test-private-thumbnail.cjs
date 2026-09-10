/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const qml = fs.readFileSync(path.join(__dirname, '../../apps/browser/qml/pages/BrowserPage.qml'), 'utf8');
const methods = [...qml.matchAll(/^    function \w+\([^]*?^    }/gm)].map(m => m[0]).join('\n');
for (const nativePresentation of [true, false]) {
  const callbacks = [];
  const capturePaths = [];
  const updated = [];
  const completed = [];
  const view = { privateMode: true, selectedTabId: '23',
    grabNativeImage(callback) { capturePaths.push(true); callbacks.push(callback); return true; },
    grabToImage(callback) { capturePaths.push(false); callbacks.push(callback); return true; } };
  const scope = {
    chromeHostView: view, width: 100, height: 200,
    _privateCoverPending: false, _privateCaptureGeneration: 0, _privateTabGrabs: {},
    webView: { privateMode: true, foreground: true, nativeWindow: nativePresentation ? {} : null,
      privateTabModel: { updateThumbnailPath(...args) { updated.push(args); } } },
    Qt: { size(width, height) { return { width, height }; } },
    hostedThumbnailGrabbed(...args) { completed.push(args); }
  };
  scope.browserPage = scope;
  vm.createContext(scope);
  vm.runInContext(methods, scope);
  scope.selectedPersistentId = () => '17';
  const capture = scope.beginHostedTabViewThumbnailCapture();
  assert.ok(capture, 'Private tab grid must wait for a capture');
  assert.equal(completed.length, 0, 'Do not release the view before the callback');
  const result = { url: 'itemgrabber:private-preview' };
  callbacks.shift()(result);
  assert.deepEqual(updated, [[17, result.url]]);
  assert.equal(scope._privateTabGrabs['17'], result, 'Retain the in-memory image');
  assert.equal(completed[0][3], capture.generation);
  // A late capture must never be attached to another selected tab.
  scope.beginHostedTabViewThumbnailCapture();
  view.selectedTabId = '24';
  callbacks.shift()({ url: 'itemgrabber:stale' });
  assert.equal(updated.length, 1);
  scope.prunePrivateTabGrabs([]);
  assert.equal(Object.keys(scope._privateTabGrabs).length, 0);
  assert.equal(scope.privateCoverGrab, null);
  assert.deepEqual(capturePaths, [nativePresentation, nativePresentation]);
}
console.log('Private thumbnail capture tests passed');
