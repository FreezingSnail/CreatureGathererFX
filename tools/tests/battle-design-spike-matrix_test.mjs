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
const ICON_DATA = require(path.join(repo, 'mock/icons.js'));
const source = fs.readFileSync(path.join(repo, 'mock/matrix.js'), 'utf8');

function bootMatrix(container, battleMock, options = {}) {
  const documentStub = {
    readyState: 'complete',
    querySelectorAll: () => [container],
    addEventListener() {},
  };
  if (options.canvas) {
    documentStub.createElement = () => ({
      width: 0,
      height: 0,
      getContext: () => ({ fillStyle: '', fillRect() {} }),
      toDataURL: () => 'data:image/png;base64,SPRITE',
    });
  }
  const windowStub = { CGFX_SPIKE_DATA: DATA, CGFX_ICONS: ICON_DATA, __battleMock: battleMock };
  vm.runInNewContext(source, {
    window: windowStub,
    document: documentStub,
    setInterval: () => 0,
    clearInterval: () => {},
  });
  return windowStub;
}

test('wheel table headers render the shared type sprites', () => {
  const container = { dataset: { matrix: 'wheel' }, innerHTML: '' };
  bootMatrix(container, null, { canvas: true });
  assert.ok(container.innerHTML.includes('class="m-icon"'), 'header sprites rendered');
  assert.ok(container.innerHTML.includes('title="fire"'), 'sprite keeps the type name as tooltip');
  for (const type of DATA.wheel.order) assert.ok(container.innerHTML.includes(`alt="${type}"`), type);
  assert.ok(container.innerHTML.includes('m-win'), 'has ▲ cells');
  assert.ok(container.innerHTML.includes('m-lose'), 'has ▼ cells');
});

test('without canvas the headers fall back to type names', () => {
  const container = { dataset: { matrix: 'wheel' }, innerHTML: '' };
  bootMatrix(container);
  assert.ok(!container.innerHTML.includes('m-icon'), 'no sprites without canvas');
  assert.ok(container.innerHTML.includes('fire'), 'text header instead');
});

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

test('spike tables render from the generated matrices', () => {
  const MATRICES = require(path.join(repo, 'mock/matrices.js'));
  const container = { dataset: { matrix: 'spike:4' }, innerHTML: '' };
  const documentStub = {
    readyState: 'complete',
    querySelectorAll: () => [container],
    addEventListener() {},
    createElement: () => ({ getContext: () => ({ fillStyle: '', fillRect() {} }), toDataURL: () => 'data:image/png;base64,SPRITE' }),
  };
  const windowStub = { CGFX_SPIKE_DATA: DATA, CGFX_ICONS: ICON_DATA, CGFX_MATRICES: MATRICES };
  vm.runInNewContext(source, {
    window: windowStub, document: documentStub, setInterval: () => 0, clearInterval: () => {},
  });
  const four = MATRICES.matrices.find((matrix) => matrix.id === '4');
  for (const type of four.order) assert.ok(container.innerHTML.includes(`alt="${type}"`), type);
  assert.ok(container.innerHTML.includes('m-win'), 'marks rendered');
});

test('duel windows follow the selected table', () => {
  const MATRICES = require(path.join(repo, 'mock/matrices.js'));
  const container = { dataset: { matrix: 'wheel', highlight: 'on', followDuel: 'on' }, innerHTML: '' };
  const documentStub = {
    readyState: 'complete',
    querySelectorAll: () => [container],
    addEventListener() {},
    createElement: () => ({ getContext: () => ({ fillStyle: '', fillRect() {} }), toDataURL: () => 'data:image/png;base64,SPRITE' }),
  };
  const battleMock = { youIndex: 10, foeIndex: 0, duel: { kind: 'wheel', matrixId: '4', aiRead: 45 } };
  const windowStub = {
    CGFX_SPIKE_DATA: DATA, CGFX_ICONS: ICON_DATA, CGFX_MATRICES: MATRICES,
    __battleMock: battleMock, CGFXM: null,
  };
  vm.runInNewContext(source, {
    window: windowStub, document: documentStub, setInterval: () => 0, clearInterval: () => {},
  });
  const four = MATRICES.matrices.find((matrix) => matrix.id === '4');
  for (const type of four.order) assert.ok(container.innerHTML.includes(`alt="${type}"`), type);
  for (const type of DATA.wheel.order.slice(4)) {
    assert.ok(!container.innerHTML.includes(`alt="${type}"`), `${type} belongs to the 8-type table`);
  }
});
