// Run: node --test tools/tests/battle-duel-spike_test.mjs
import test from 'node:test';
import assert from 'node:assert/strict';
import { playDuel, runMatrix } from '../battle-duel-spike.mjs';

test('duels are deterministic for a seed and always terminate', () => {
  const a = playDuel({ persona: 'reader', readRate: 50, policy: 'random', seed: 42 });
  const b = playDuel({ persona: 'reader', readRate: 50, policy: 'random', seed: 42 });
  assert.deepEqual(a, b);
  assert.ok(['WIN', 'LOSE', 'DRAW'].includes(a.outcome), a.outcome);
  assert.ok(a.rounds > 0 && a.rounds <= 40);
});

test('matrix accounting adds up', () => {
  const result = runMatrix({ battles: 60, persona: 'frequency', readRate: 50, policy: 'counter' });
  assert.equal(result.wins + result.losses + result.draws + result.timeouts, result.battles);
  assert.ok(result.avgRounds > 0);
});

test('the reader persona is exploitable by predictable rotation at high read rates', () => {
  const low = runMatrix({ battles: 150, persona: 'reader', readRate: 0, policy: 'anti-repeat' });
  const high = runMatrix({ battles: 150, persona: 'reader', readRate: 100, policy: 'anti-repeat' });
  const lowWin = low.wins / low.battles, highWin = high.wins / high.battles;
  assert.ok(highWin > lowWin + 0.2,
    `rotation exploits a full reader (${(100 * lowWin).toFixed(0)}% -> ${(100 * highWin).toFixed(0)}%)`);
});

test('a reading AI beats a random player more as its read rate rises', () => {
  const low = runMatrix({ battles: 150, persona: 'reader', readRate: 0, policy: 'random' });
  const high = runMatrix({ battles: 150, persona: 'reader', readRate: 100, policy: 'random' });
  assert.ok(high.wins < low.wins, `player wins drop (${low.wins} -> ${high.wins})`);
});
