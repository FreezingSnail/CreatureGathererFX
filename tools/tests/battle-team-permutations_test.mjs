// Run: node --test tools/tests/battle-team-permutations_test.mjs
import test from 'node:test';
import assert from 'node:assert/strict';
import {
  census, typeCombos, typeSequences, classify, rosterCounts, TEAM_SIZE,
} from '../battle-team-permutations.mjs';

const choose = (n, k) => {
  let value = 1;
  for (let i = 0; i < k; i++) value = (value * (n - i)) / (i + 1);
  return Math.round(value);
};

test('four types, three slots: 20 compositions, 64 sequences, 24 distinct triples', () => {
  const { table } = census();
  assert.equal(table.id, '4');
  const combos = typeCombos(table.order, TEAM_SIZE);
  const sequences = typeSequences(table.order, TEAM_SIZE);
  assert.equal(combos.length, 20); // C(4+3-1, 3)
  assert.equal(sequences.length, 64); // 4^3
  assert.equal(sequences.filter((sequence) => new Set(sequence).size === 3).length, 24); // P(4,3)
});

test('matchup classes cover all 210 unordered composition pairs', () => {
  const { pairs } = census();
  assert.equal(pairs.length, 210); // C(20+1, 2)
  const kinds = { mutual: 0, A: 0, B: 0, dead: 0 };
  for (const pair of pairs) kinds[pair.kind] += 1;
  assert.equal(kinds.mutual + kinds.A + kinds.B + kinds.dead, 210);
  // Mono-type mirrors are the only dead pairs: no internal edge can exist.
  assert.equal(kinds.dead, 4);
});

test('mirror classification needs an internal edge', () => {
  const { table } = census();
  assert.equal(classify(table, ['flame', 'flame', 'flame'], ['flame', 'flame', 'flame']), 'dead');
  assert.equal(classify(table, ['flame', 'bloom'], ['flame', 'bloom']), 'mutual',
    'flame>bloom is an internal edge, so the mirror has a win both ways');
});

test('roster team counts partition C(64,3) over the compositions', () => {
  const { combos, records } = census();
  const total = records.reduce((sum, record) => sum + record.teams, 0);
  assert.equal(total, choose(64, 3));
  const distinctTriples = combos.filter((combo) => new Set(combo).size === 3);
  assert.equal(distinctTriples.length, 4);
  for (const record of records) {
    assert.ok(record.advantages + record.mutual + record.disadvantages + record.dead === 19,
      `${record.combo.join('+')} has 19 opponents`);
  }
});

test('roster pools fold the authored types onto the four-cycle', () => {
  const { pool } = census();
  assert.deepEqual(Object.keys(pool).sort(), ['bloom', 'flame', 'stone', 'tide']);
  assert.equal(Object.values(pool).reduce((sum, count) => sum + count, 0), 64);
});
