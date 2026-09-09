/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Run with node tests/auto/test-hosted-context-menu.cjs.
const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");
const source = fs.readFileSync(path.join(__dirname,
  "../../apps/browser/qml/pages/BrowserPage.qml"), "utf8");
// Popup construction must resolve the model from the originating host. The
// QmlMozView's chromeView id is scoped inside its Component, not this method.
const popupMethod = source.match(/    function presentHostedPopup\([\s\S]*?\n    }/)[0];
for (const privateMode of [false, true]) {
  const hostView = { browserTabModel: { privateMode } };
  const target = {};
  const page = {};
  let delivered = false;
  const opener = { message(name, data) {
    assert.equal(name, "Content:ContextMenu");
    assert.equal(data.types[0], "content-text");
    delivered = true;
    return true;
  } };
  const scope = {
    hostedMessageTabIsSelected: () => true,
    createHostedMessageTarget: () => target,
    browserPage: page,
    window: { pageStack: {} },
    hostedPopupOpenerComponent: { createObject(parent, properties) {
      assert.equal(parent, page);
      assert.equal(properties.tabModel, hostView.browserTabModel);
      assert.equal(properties.contentItem, target);
      return opener;
    } }
  };
  vm.createContext(scope);
  vm.runInContext(popupMethod, scope);
  assert.equal(scope.presentHostedPopup(hostView, "1", "10",
    "Content:ContextMenu", { types: ["content-text"] }, {}), true);
  assert.equal(delivered, true);
  assert.equal(target.opener, opener);
}
console.log("Hosted context menu routing tests passed");
