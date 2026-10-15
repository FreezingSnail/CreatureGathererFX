// Run: node --test tools/tests/battle-design-spike-matrix_test.mjs
// Verifies mock/matrix.js renders x/y tables from mock/data.js in a stub DOM.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

const repo = path.dirname(fileURLToPath(import.meta.url)) + '/../..';
const require = createRequire(import.meta.url);
const DATA = require(path.join(repo, 'mock/data.js'));
const source = fs.readFileSync(path.join(repo, 'mock/matrix.js'), 'utf8');

function bootMatrix(container, battleMock) {
  const documentStub = {
    readyState: 'complete',
    querySelectorAll: () => [container],
    addEventListener() {},
  };
  const windowStub = { CGFX_SPIKE_DATA: DATA, __battleMock: battleMock };
  vm.runInNewContext(source, {
    window: windowStub,
    document: documentStub,
    setInterval: () => 0,
    clearInterval: () => {},
  });
  return windowStub;
}

test('wheel table renders all eight headers with win/lose marks', () => {
  const container = { dataset: { matrix: 'wheel' }, innerHTML: '' };
  bootMatrix(container);
  for (const type of DATA.wheel.order) assert.ok(container.innerHTML.includes(type), type);
  assert.ok(container.innerHTML.includes('m-win'), 'has ▲ cells');
  assert.ok(container.innerHTML.includes('m-lose'), 'has ▼ cells');
  assert.ok(container.innerHTML.includes('m-flat'), 'has · cells');
  assert.ok(container.innerHTML.includes('atk \\ def') || container.innerHTML.includes('atk \\'), 'axis label');
});

test('stance table renders the four merged types', () => {
  const container = { dataset: { matrix: 'stances' }, innerHTML: '' };
  bootMatrix(container);
  for (const type of DATA.merged.order) assert.ok(container.innerHTML.includes(type), type);
});

test('highlight marks the running mock row and column', () => {
  const container = { dataset: { matrix: 'wheel', highlight: 'on' }, innerHTML: '' };
  const windowStub = bootMatrix(container, { youIndex: 10, foeIndex: 0 });
  windowStub.CGFXM.render(container);
  const you = DATA.creatures[10], foe = DATA.creatures[0];
  assert.ok(container.innerHTML.includes('hl-row'), 'row highlighted');
  assert.ok(container.innerHTML.includes('hl-col'), 'column highlighted');
  assert.ok(container.innerHTML.includes('hl-x'), 'intersection highlighted');
  assert.ok(you.types.length && foe.types.length);
});
