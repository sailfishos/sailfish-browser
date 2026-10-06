/* SPDX-FileCopyrightText: 2026 Jolla Mobile Ltd
 * SPDX-License-Identifier: MPL-2.0 */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const read = name => fs.readFileSync(path.join(__dirname, name), 'utf8');
const page = read('../../../apps/browser/qml/pages/BrowserPage.qml');
const suspension = page.match(/^    function updateHostedViewSuspension\([^\n]*\) \{[\s\S]*?^    \}/m)[0];
const viewScope = vm.createContext({ equal: assert.equal });
vm.runInContext(suspension + '\n' + read('view-suspension.js'), viewScope);
const throttle = page.match(/^                throttlePainting: ([\s\S]*?)\n\n/m)[1];
assert.equal(vm.runInNewContext(throttle, {
  browserPage: { visible: true }, privateMode: false,
  webView: { foreground: false, applicationVisible: true,
             resourceController: { videoActive: true } },
}), true, 'A visible app cover must not keep video rendering active');

const controller = read('../../../apps/shared/ResourceController.qml');
const calculate = controller.match(/^    function calculateStatus\([^\n]*\) \{[\s\S]*?^    \}/m)[0];
const state = vm.createContext({
  mediaPlaybackActive: true, _webrtcAudioActive: false, _webrtcVideoActive: false,
  _mediaState: "pause", audioActive: false, videoActive: false,
});
vm.runInContext(calculate + '\ncalculateStatus()', state);
assert.equal(state.audioActive, true);
state.mediaPlaybackActive = false;
vm.runInContext('calculateStatus()', state);
assert.equal(state.audioActive, false);
state._webrtcAudioActive = true;
vm.runInContext('calculateStatus()', state);
assert.equal(state.audioActive, true, 'Existing WebRTC audio remains supported');
assert.match(controller, /preventBlanking: videoActive && foreground/);

const retention = page.match(/^    function hostedViewHasPlayingMedia\([^\n]*\) \{[\s\S]*?^    \}/m)[0];
const retentionScope = vm.createContext({});
vm.runInContext(retention, retentionScope);
const normalHost = { tabModel: { revision: '1', snapshot: () => [
    { tabId: '7', mediaPlaying: false }, { tabId: '8', mediaPlaying: true }] } };
const privateHost = { tabModel: { revision: '2', snapshot: () => [
    { tabId: '9', mediaPlaying: true }] } };
assert.equal(retentionScope.hostedViewHasPlayingMedia(normalHost), true,
             'An unselected playing tab must retain audio resources');
assert.equal(retentionScope.hostedViewHasPlayingMedia(privateHost), true,
             'Private playback must retain audio resources');
normalHost.tabModel.snapshot = () => [{ mediaPlaying: false }];
assert.equal(retentionScope.hostedViewHasPlayingMedia(normalHost), false);
assert.equal(retentionScope.hostedViewHasPlayingMedia(null), false);
assert.doesNotMatch(page, /embed:media-state|embedui:media-state/);
assert.match(page, /backgroundMediaEnabled: true/);
console.log('Per-tab background retention and independent presentation tests passed');
