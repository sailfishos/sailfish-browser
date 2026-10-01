/* SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0 */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const pages = path.join(__dirname, '../../../apps/browser/qml/pages');
const page = fs.readFileSync(path.join(pages, 'BrowserPage.qml'), 'utf8');
const functions = ['stop', 'reload'].map(name => {
  const match = page.match(new RegExp('^    function ' + name + '\\([^\\n]*\\) \\{[\\s\\S]*?^    \\}', 'm'));
  assert.ok(match, name);
  return match[0];
}).join('\n');
const calls = [];
const host = {
  loading: true,
  stop() { assert.deepEqual(calls.at(-1), ['cancel', '7']); calls.push('stop'); },
  reload() { assert.deepEqual(calls.at(-1), ['cancel', '7']); calls.push('reload'); },
};
const scope = vm.createContext({
  chromeHostView: host, hostedView: host,
  selectedPersistentId: () => '7',
  webView: { tabModel: { cancelRuntimeTraversal: id => calls.push(['cancel', id]) } },
  toolBarRow: { hosted: true, showChrome() {} }, root: { hosted: true },
  overlay: { animator: { showChrome() {} } },
});
vm.runInContext(functions, scope);
scope.browserPage = scope;
for (const file of ['ToolBar.qml', 'PopUpMenuFooter.qml']) {
  const source = fs.readFileSync(path.join(pages, 'components', file), 'utf8');
  const marker = file === 'ToolBar.qml' ? 'id: stopButton' : '? "image://theme/icon-m-reset"';
  const tail = source.slice(source.indexOf(marker));
  const match = tail.match(/onTapped: \{([\s\S]*?)^            \}/m);
  assert.ok(match, file);
  for (const loading of [true, false]) {
    host.loading = loading;
    calls.length = 0;
    vm.runInContext(match[1], scope);
    assert.deepEqual(calls, [['cancel', '7'],
      file === 'ToolBar.qml' || loading ? 'stop' : 'reload']);
  }
}
console.log('Toolbar and menu Stop/reload cancel queued traversal before dispatch');
