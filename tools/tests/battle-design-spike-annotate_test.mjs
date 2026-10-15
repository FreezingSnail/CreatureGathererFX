// Run: node --test tools/tests/battle-design-spike-annotate_test.mjs
// Verifies mock/annotate.js builds the overlay chips and legend per duel spelling.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import { fileURLToPath } from 'node:url';

const repo = path.dirname(fileURLToPath(import.meta.url)) + '/../..';
const source = fs.readFileSync(path.join(repo, 'mock/annotate.js'), 'utf8');

function boot(dataset) {
  const layer = { hidden: true, innerHTML: '' };
  const legend = { hidden: true, innerHTML: '' };
  const toggle = { checked: true, hidden: false, listeners: {}, addEventListener(type, handler) { this.listeners[type] = handler; } };
  const documentStub = {
    body: { dataset },
    querySelector(selector) {
      if (selector === '[data-annotate-layer]') return layer;
      if (selector === '[data-annotate-legend]') return legend;
      return null;
    },
    getElementById(id) { return id === 'annotate' ? toggle : null; },
  };
  const windowStub = {};
  vm.runInNewContext(source, { window: windowStub, document: documentStub });
  return { layer, legend, toggle, api: windowStub.CGFX_ANNOTATE };
}

test('wheel power overlay: chips, legend, and toggle wiring', () => {
  const { layer, legend, toggle, api } = boot({ mode: 'duel', duel: 'wheel', scoring: 'power' });
  assert.ok(api, 'exposed api');
  assert.ok(layer.innerHTML.includes('class="chip"'));
  assert.ok((layer.innerHTML.match(/class="chip"/g) || []).length >= 10, 'chips rendered');
  assert.ok(legend.innerHTML.includes('FOE sprite'));
  assert.ok(legend.innerHTML.includes('✱'), 'affinity marker explained');
  assert.ok(legend.innerHTML.includes('WHEEL WIN'));
  assert.equal(layer.hidden, false, 'visible by default');
  toggle.checked = false;
  toggle.listeners.change();
  assert.equal(layer.hidden, true);
  assert.equal(legend.hidden, true);
});

test('pure overlay explains that power and affinity are ignored', () => {
  const { legend } = boot({ mode: 'duel', duel: 'wheel', scoring: 'flat' });
  assert.ok(legend.innerHTML.includes('power is ignored'));
  assert.ok(legend.innerHTML.includes('No markers in this spelling'));
  assert.ok(legend.innerHTML.includes('RPS WIN'));
  assert.ok(!legend.innerHTML.includes('✱ on a cell'));
});

test('bonus overlay explains the creature-type marker', () => {
  const { legend } = boot({ mode: 'duel', duel: 'wheel', scoring: 'bonus' });
  assert.ok(legend.innerHTML.includes('beats the foe creature'));
  assert.ok(legend.innerHTML.includes('×1.5'));
  assert.ok(!legend.innerHTML.includes('RPS WIN'));
});

test('stance overlay uses the 4-type cycle legend', () => {
  const { legend, layer } = boot({ mode: 'duel', duel: 'stances' });
  assert.ok(legend.innerHTML.includes('flame &gt; bloom &gt; tide &gt; stone'));
  assert.ok(legend.innerHTML.includes('AFF'));
  assert.ok(legend.innerHTML.includes('CLASH WIN'));
  assert.ok((layer.innerHTML.match(/class="chip"/g) || []).length >= 9);
});

test('non-duel pages are untouched', () => {
  const { api } = boot({ mode: 'trim' });
  assert.equal(api, undefined);
});
