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
  harness.press('1'); // design switching is locked in a variant window
  harness.advances(1);
  assert.equal(state.mode, 'duel');
  harness.press('z');
  harness.advances(2);
  assert.equal(state.scene, 'battle');
  assert.ok(harness.spritePixels() > 200, 'duel battle renders in the locked window');
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
