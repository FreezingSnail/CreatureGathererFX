// Run: node --test tools/tests/battle-design-spike-data_test.mjs
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {
  buildSpikeData,
  serializeSpikeData,
  moveUseLimit,
  OUTPUT,
  MERGED,
} from '../battle-design-spike-data.mjs';

test('committed spike data matches a fresh generation', () => {
  const committed = fs.readFileSync(OUTPUT, 'utf8');
  assert.equal(committed, serializeSpikeData(buildSpikeData()));
});

test('creature records match authored creatures and equipped moves', () => {
  const data = buildSpikeData();
  const authored = JSON.parse(fs.readFileSync('data/json/creatures.json', 'utf8'));
  assert.equal(data.creatures.length, authored.length);
  for (const creature of data.creatures) {
    const source = authored.find((entry) => entry.id === creature.id);
    assert.equal(creature.name, source.name);
    assert.ok(creature.types.length >= 1 && creature.types.length <= 2);
    assert.ok(creature.maxHp > 30);
    for (const id of creature.moves) {
      if (id === null) continue;
      assert.ok(data.moves[id], `move ${id} missing`);
    }
    for (const type of creature.types) assert.ok(MERGED.map[type], type);
    for (const merged of creature.merged) assert.ok(MERGED.order.includes(merged));
  }
});

test('every referenced sprite file exists on disk', () => {
  const data = buildSpikeData();
  for (const creature of data.creatures) {
    for (const file of [creature.front, creature.back]) {
      assert.ok(fs.existsSync(`docs/assets/battle-sprites48/${file}`), file);
    }
  }
});

test('merged four-type chart is a clean beat cycle', () => {
  const data = buildSpikeData();
  const { order, chart } = data.merged;
  assert.deepEqual(order, ['flame', 'bloom', 'tide', 'stone']);
  assert.equal(new Set(Object.values(MERGED.map)).size, 4);
  order.forEach((attacker, row) => {
    const beats = chart[row].filter((value) => value === 2).length;
    const loses = chart[row].filter((value) => value === 0.5).length;
    assert.equal(beats, 1, `${attacker} beats count`);
    assert.equal(loses, 1, `${attacker} loses count`);
    assert.equal(chart[row][row], 1);
  });
});

test('eight-type web satisfies every roster set pair with a solver proof', async () => {
  const solver = await import('../battle-matchup-solver.mjs');
  const data = buildSpikeData();
  const { order, chart, edges, cycles } = data.wheel;
  assert.equal(order.length, 8);
  assert.equal(edges.length, edges.length);
  assert.ok(cycles.length >= 4, 'overlapping triangles, not a single ring');
  const convention = solver.chartFromEdges(order, edges);
  assert.deepEqual(convention, chart, 'chart mirrors the edge list');
  const sets = solver.rosterSets(data.creatures, data.moves, order);
  const unsatisfied = solver.unsatisfiedDemands(order, chart, sets);
  assert.equal(unsatisfied.length, 0, `${unsatisfied.length} set pairs without a positive matchup`);
  // Not everything matches: keep a meaningful neutral fraction.
  const decisive = solver.edgeList(order, chart).length;
  const fraction = decisive / (order.length * (order.length - 1));
  assert.ok(fraction > 0.2 && fraction < 0.7, `decisive fraction ${fraction.toFixed(2)}`);
  assert.ok(chart.some((row) => row.includes(1)), 'neutral cells exist');
  // No mutual edges and no immunity.
  for (let row = 0; row < 8; row++) {
    for (let col = 0; col < 8; col++) {
      assert.notEqual(chart[row][col], 0);
      if (row !== col) {
        const pair = [chart[row][col], chart[col][row]].sort();
        assert.ok((pair[0] === 0.5 && pair[1] === 2) || (pair[0] === 1 && pair[1] === 1));
      }
    }
  }
});

test('production chart keeps authored 8x8 values', () => {
  const data = buildSpikeData();
  assert.deepEqual(data.typeOrder, ['spirit', 'water', 'wind', 'earth', 'fire', 'lightning', 'plant', 'elder']);
  assert.equal(data.chart.length, 8);
  for (const row of data.chart) {
    assert.equal(row.length, 8);
    for (const value of row) assert.ok([0, 0.5, 1, 2].includes(value));
  }
});

test('move use limits follow the production rule', () => {
  assert.equal(moveUseLimit({ power: 0, effect: 'ATKUP' }), 3);
  assert.equal(moveUseLimit({ power: 0, effect: 'NONE' }), 255);
  assert.equal(moveUseLimit({ power: 10, effect: 'NONE' }), 2);
  assert.equal(moveUseLimit({ power: 5, effect: 'PINNED' }), 2);
  assert.equal(moveUseLimit({ power: 5, effect: 'NONE' }), 255);
  const data = buildSpikeData();
  for (const move of Object.values(data.moves)) {
    assert.equal(move.limit, moveUseLimit(move));
  }
});

test('font atlas covers printable characters 48..122 at 5x6', () => {
  const data = buildSpikeData();
  assert.equal(data.font.width, 5);
  assert.equal(data.font.height, 6);
  assert.equal(data.font.firstChar, 48);
  assert.equal(data.font.glyphs.length, 75);
  for (const bits of data.font.glyphs) {
    assert.match(bits, /^[01]{30}$/);
    assert.ok(bits.includes('1'), 'empty glyph');
  }
});
