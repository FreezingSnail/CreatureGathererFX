// Run: node --test tools/tests/battle-design-spike-frame_test.mjs
// Renders mock/index.html scenes headlessly through the browser boot path.
import test from 'node:test';
import assert from 'node:assert/strict';
import { createHarness } from '../battle-design-spike-frame.mjs';

const hash = (frame) => {
  let value = 2166136261;
  for (const pixel of frame) { value ^= pixel; value = Math.imul(value, 16777619); }
  return value >>> 0;
};

test('setup scene draws sprites, names and rows without errors', () => {
  const harness = createHarness();
  harness.advances(2);
  assert.ok(harness.whitePixels() > 200, 'setup has content');
  assert.ok(harness.spritePixels() > 200, 'both 48x48 sprites drawn');
});

test('current mode reaches the move menu and feedback frames', () => {
  const harness = createHarness();
  harness.advances(2);
  harness.press('z'); // A FIGHT
  harness.advances(2);
  const choice = hash(harness.frame);
  assert.ok(harness.spritePixels() > 200, 'battle keeps sprites visible');
  harness.press('z'); // attack
  harness.advances(2);
  assert.notEqual(hash(harness.frame), choice, 'feedback replaces the menu');
});

test('trim mode shows arrows and press-turn extra step', () => {
  const harness = createHarness();
  harness.switchMode('trim');
  harness.press('z');
  harness.advances(2);
  const choice = hash(harness.frame);
  harness.press('z');
  harness.advances(2);
  const first = hash(harness.frame);
  assert.notEqual(first, choice);
  harness.press('z'); // advance to the extra action if queued
  harness.advances(2);
  assert.ok(harness.whitePixels() > 150);
});

test('duel mode reveals picks and resolves a round', () => {
  const harness = createHarness();
  harness.switchMode('duel');
  harness.press('z');
  harness.advances(2);
  const stanceRow = hash(harness.frame);
  harness.press('z'); // pick stance
  harness.advances(2);
  const reveal = hash(harness.frame);
  assert.notEqual(reveal, stanceRow, 'footer swaps to reveal');
  harness.advances(10, 100); // reveal 550 ms + result paints
  assert.notEqual(hash(harness.frame), reveal, 'result frame differs');
});

test('variant windows lock their design and honor the seed query', () => {
  const harness = createHarness({ mode: 'duel', search: '?seed=4242' });
  harness.advances(2);
  const state = harness.state();
  assert.equal(state.mode, 'duel');
  assert.equal(state.lockMode, true);
  assert.equal(state.seed, 4242);
  assert.equal(state.duel.scoring, 'power', 'default scoring');
  harness.press('1'); // design switching is locked in a variant window
  harness.advances(1);
  assert.equal(state.mode, 'duel');
  harness.press('z');
  harness.advances(2);
  assert.equal(state.scene, 'battle');
  assert.ok(harness.spritePixels() > 200, 'duel battle renders in the locked window');
});

test('pure RPS window boots with flat scoring from its body attribute', () => {
  const harness = createHarness({ mode: 'duel', duel: 'wheel', scoring: 'flat' });
  harness.advances(2);
  const state = harness.state();
  assert.equal(state.duel.kind, 'wheel');
  assert.equal(state.duel.scoring, 'flat');
  harness.press('z');
  harness.advances(2);
  assert.equal(state.scene, 'battle');
  harness.press('z');
  harness.advances(8, 100);
  const lines = harness.state().duelState.lines[0];
  assert.ok(/RPS (WIN|LOSE)|TRADE/.test(lines), lines);
});

test('bonus window boots with bonus scoring and draws type badges', () => {
  const harness = createHarness({ mode: 'duel', duel: 'wheel', scoring: 'bonus' });
  harness.advances(2);
  const state = harness.state();
  assert.equal(state.duel.scoring, 'bonus');
  // Badge plates sit at the creature corners; check a badge pixel row is painted.
  harness.press('z');
  harness.advances(2);
  assert.equal(state.scene, 'battle');
  let badgePixels = 0;
  for (let y = 37; y < 47; y++) {
    for (let x = 0; x < 22; x++) badgePixels += harness.frame[y * 128 + x];
    for (let x = 106; x < 128; x++) badgePixels += harness.frame[y * 128 + x];
  }
  assert.ok(badgePixels > 20, `type badges painted (${badgePixels} pixels)`);
});

test('type table can be chosen from the query and from the in-window selector', () => {
  // Launcher passes ?matrix=N; the duel boots with that table.
  const harness = createHarness({ mode: 'duel', duel: 'wheel', scoring: 'bonus', search: '?seed=77&matrix=5' });
  harness.advances(2);
  assert.equal(harness.state().duel.matrixId, '5');
  // In-window selector switches the live table and resets to setup.
  harness.setControl('duel-matrix', '4');
  assert.equal(harness.state().duel.matrixId, '4');
  assert.equal(harness.state().scene, 'setup');
  // Invalid query values are ignored (falls back to the demo default 5).
  const fallback = createHarness({ mode: 'duel', duel: 'wheel', search: '?matrix=99' });
  fallback.advances(2);
  assert.equal(fallback.state().duel.matrixId, '5');
  // The demo default with no query at all is also 5.
  const defaulted = createHarness({ mode: 'duel', duel: 'wheel' });
  defaulted.advances(2);
  assert.equal(defaulted.state().duel.matrixId, '5');
  // Battle starts fine on the folded table (folded resolution numbers are covered
  // by tools/tests/battle-matrices-spike_test.mjs).
  harness.press('z');
  harness.advances(2);
  assert.equal(harness.state().scene, 'battle');
});

test('mode edges 1/2/3 switch scenes from the keyboard', () => {
  const harness = createHarness();
  harness.press('2');
  harness.advances(2);
  harness.press('z');
  harness.advances(2);
  assert.ok(harness.spritePixels() > 200, 'trim battle renders');
  harness.press('3');
  harness.advances(2);
  harness.press('z');
  harness.advances(2);
  assert.ok(harness.spritePixels() > 200, 'duel battle renders');
});

test('sample team selects drive the duel party with retyped members', () => {
  const harness = createHarness({ mode: 'duel', duel: 'wheel', scoring: 'bonus', search: '?seed=99' });
  harness.advances(2);
  harness.setControl('duel-matrix', '5');
  harness.setControl('team-you', 'balanced');
  harness.setControl('team-foe', 'anchors');
  const state = harness.state();
  assert.equal(state.duel.matrixId, '5');
  assert.equal(state.duel.presetYou, 'balanced');
  assert.equal(state.duel.presetFoe, 'anchors');
  harness.press('z');
  harness.advances(2);
  assert.equal(state.scene, 'battle');
  assert.equal(state.duelState.partyTypes[0].join('/'), 'water/earth/plant');
  assert.equal(state.duelState.partyTypes[1].join('/'), 'water/water/earth');
  assert.ok(state.duelState.hp[0].length === 3 && state.duelState.hp[1].length === 3);
});
