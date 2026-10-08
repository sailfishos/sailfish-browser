/* SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0 */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.join(__dirname,
    '../../../apps/browser/qml/BrowserMediaPlayer.qml'), 'utf8');
function handler(name) {
    const match = source.match(new RegExp('            on' + name + ': \\{([\\s\\S]*?)\\n            \\}'));
    assert.ok(match, name);
    return 'function run() {' + match[1] + '\n} run()';
}
let sought;
const controller = { canSeek: true, position: 10.25, duration: 120, trackId: '/track/t1',
    seek(track, seconds) { sought = {track, seconds}; return true; } };
const mediaPlayer = { publishable: true, controller };
vm.runInNewContext(handler('SeekRequested'), {mediaPlayer, offset: 1875});
assert.deepEqual(sought, {track: '/track/t1', seconds: 12.125});
vm.runInNewContext(handler('SetPositionRequested'), {mediaPlayer, trackId: '/track/t1', position: 23456});
assert.deepEqual(sought, {track: '/track/t1', seconds: 23.456});
sought = undefined;
mediaPlayer.publishable = false;
vm.runInNewContext(handler('SetPositionRequested'), {mediaPlayer, trackId: '/track/private', position: 5000});
vm.runInNewContext(handler('SeekRequested'), {mediaPlayer, offset: 5000});
assert.equal(sought, undefined, 'Private transitions disable commands before player teardown');
for (const field of ['trackId', 'title', 'contributingArtist', 'albumTitle', 'duration']) {
    assert.match(source, new RegExp('metaData\\.' + field + ': mediaPlayer\\.publishable'));
}
const lifecycle = source.match(/    function updatePlayer\(\) \{([\s\S]*?)\n    \}/)[1];
const retired = {serviceName: 'sailfish_browser'};
const changes = [];
const context = {publishable: false, item: retired};
Object.defineProperty(context, 'active', {set(value) {
    changes.push(value);
    assert.equal(retired.serviceName, '', 'Old player unregisters before deferred Loader deletion');
}});
vm.runInNewContext(lifecycle, context);
assert.deepEqual(changes, [false]);
context.publishable = true;
context.item = null;
vm.runInNewContext(lifecycle, context);
assert.deepEqual(changes, [false, true]);
assert.equal(retired.serviceName, '', 'Retired player cannot reclaim the new player connection');
assert.match(source, /serviceName: "sailfish_browser"/);
assert.match(source, /hasLoopStatus: false/);
assert.match(source, /hasShuffle: false/);
console.log('MPRIS seek conversion and private publication guards passed');
