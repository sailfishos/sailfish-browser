/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Run with node tests/auto/test-hosted-select-cancel.cjs.
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");
const source = fs.readFileSync(path.join(__dirname,
  "../../apps/browser/qml/pages/BrowserPage.qml"), "utf8");
const method = source.match(/    function cancelHostedSelect\([\s\S]*?\n    }/)[0];
const host = {};
const otherHost = {};
const request = (overrides = {}) => Object.assign({
  message: "embed:selectasync", hostView: host, tabId: "1",
  persistentId: "10", data: { id: "123:1" }
}, overrides);
const matching = request();
const preserved = [request({ hostView: otherHost }), request({ tabId: "2" }),
  request({ persistentId: "11" }), request({ data: { id: "124:1" } }),
  request({ message: "embed:confirm" })];
for (const active of [matching, ...preserved]) {
  const calls = [];
  const scope = {
    _pendingHostedModalRequests: [matching, ...preserved],
    _activeHostedModalTarget: {
      modalRequest: active, opener: { message: (...args) => calls.push(args) }
    }
  };
  vm.createContext(scope);
  vm.runInContext(method, scope);
  scope.cancelHostedSelect(host, "1", "10", { id: "123:1" });
  assert.deepEqual(Array.from(scope._pendingHostedModalRequests), preserved);
  assert.equal(calls.length, active === matching ? 1 : 0);
  if (calls.length) assert.equal(calls[0][0], "embed:selectabort");
}
console.log("Hosted select cancellation tests passed");
