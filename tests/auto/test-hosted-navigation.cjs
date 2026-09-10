/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Run with node tests/auto/test-hosted-navigation.cjs.
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");

const source = fs.readFileSync(path.join(__dirname,
  "../../apps/browser/qml/pages/BrowserPage.qml"), "utf8");
for (const direction of ["Back", "Forward"]) {
  const method = source.match(new RegExp(
    "    function go" + direction + "\\(\\) \\{[\\s\\S]*?\\n    }"));
  assert.ok(method, "Production navigation method must be present");
  for (const persistentId of ["17", ""]) {
    for (const accepted of [false, true]) {
      const calls = [];
      const scope = {
        selectedPersistentId() { return persistentId; },
        chromeHostView: { ["go" + direction]() { calls.push("native"); } },
        webView: { tabModel: {
          ["runtimeGo" + direction](id) {
            assert.equal(id, persistentId);
            calls.push("bookkeeping");
            return accepted;
          }
        } }
      };
      vm.createContext(scope);
      vm.runInContext(method[0] + "\ngo" + direction + "();", scope);
      assert.deepEqual(calls, persistentId ? ["bookkeeping", "native"] : ["native"],
        "A stale database cursor must not suppress Gecko navigation");
    }
  }
  let legacyCalls = 0;
  const scope = {
    chromeHostView: null,
    webView: { ["go" + direction]() { legacyCalls++; } }
  };
  vm.createContext(scope);
  vm.runInContext(method[0] + "\ngo" + direction + "();", scope);
  assert.equal(legacyCalls, 0, "No native fallback while the QML view initializes");
}
console.log("Hosted Back/Forward tests passed");
