// Runs the Atlas page script against a minimal fake DOM and checks the play and recording controls.
// Usage: node launcher/tests/atlas_page.js
'use strict';
const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const html = fs.readFileSync(path.join(__dirname, '../Atlas/index.html'), 'utf8');
const script = html.match(/<script>([\s\S]*)<\/script>/)[1];

function element(id) {
  return {
    id, textContent: '', disabled: html.includes(`id="${id}" disabled`), hidden: false, value: '', open: false,
    dataset: {}, listeners: {},
    classList: {toggle() {}},
    append() {}, replaceChildren() {}, querySelectorAll() { return []; },
    addEventListener(type, handler) { this.listeners[type] = handler; },
    click() { this.listeners.click?.({currentTarget: this, preventDefault() {}}); },
  };
}
const elements = {};
const sent = [];
const document = {
  getElementById(id) { return elements[id] ??= element(id); },
  createElement() { return element(''); },
  querySelectorAll() { return []; },
};
const window = {webkit: {messageHandlers: {atlas: {postMessage: action => sent.push(action)}}}};
vm.runInNewContext(script, {document, window});

const titles = {mm6: false, mm7: true, mm8: false};
function state(overrides) {
  return {
    data: titles, paths: {}, saves: [], running: false, runningBuild: '', mm6Available: false, upstreamAvailable: false,
    recordingFinalizing: false, recordingEnabled: false, recordVideo: false, recordDiagnostics: false,
    recordingPath: '', recordingSession: '', modsEnabled: false, vendorRefreshOnEntry: false,
    portraits: [false, false, false, false], portraitMoods: [{}, {}, {}, {}],
    armor: {saved: false}, ...overrides,
  };
}
const upstream = document.getElementById('play-openenroth');
const note = document.getElementById('openenroth-note');
const mm6 = document.getElementById('play-mm6');

// The upstream button is disabled with an explanation, not hidden, before any state arrives.
assert.strictEqual(upstream.disabled, true);
assert.strictEqual(upstream.hidden, false);

// A missing upstream helper keeps the button disabled and says why.
window.Atlas.receive(state({}));
assert.strictEqual(upstream.disabled, true);
assert.strictEqual(upstream.hidden, false);
assert.match(note.textContent, /no upstream OpenEnroth helper/);
assert.strictEqual(document.getElementById('play-mm7').disabled, false);
assert.strictEqual(mm6.disabled, true);

// MM6 is enabled only when both its title-locked helper and data are available.
window.Atlas.receive(state({data: {...titles, mm6: true}, mm6Available: true}));
assert.strictEqual(mm6.disabled, false);
mm6.click();
assert.strictEqual(sent.at(-1), 'play-mm6');

// With the helper present the button is enabled and sends its own action.
window.Atlas.receive(state({upstreamAvailable: true}));
assert.strictEqual(upstream.disabled, false);
assert.doesNotMatch(note.textContent, /Unavailable/);
upstream.click();
document.getElementById('play-mm7').click();
assert.deepStrictEqual(sent.slice(-2), ['play-openenroth', 'play-mm7']);

// Either running build disables both play buttons and the recording toggles.
window.Atlas.receive(state({upstreamAvailable: true, running: true, runningBuild: 'openenroth'}));
assert.strictEqual(upstream.disabled, true);
assert.strictEqual(upstream.textContent, 'OpenEnroth is running');
assert.strictEqual(document.getElementById('play-mm7').disabled, true);
assert.strictEqual(mm6.disabled, true);
assert.strictEqual(document.getElementById('record-video-toggle').disabled, true);
assert.strictEqual(document.getElementById('record-diagnostics-toggle').disabled, true);

// The two recording toggles are independent.
window.Atlas.receive(state({recordVideo: false, recordDiagnostics: true}));
assert.strictEqual(document.getElementById('record-video-toggle').textContent, 'Turn on');
assert.strictEqual(document.getElementById('record-diagnostics-toggle').textContent, 'Turn off');
document.getElementById('record-video-toggle').click();
document.getElementById('record-diagnostics-toggle').click();
assert.deepStrictEqual(sent.slice(-2), ['record-video-on', 'record-diagnostics-off']);

console.log('Atlas page checks passed');
