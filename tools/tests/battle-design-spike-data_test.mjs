// Run: node --test tools/tests/battle-design-spike-data_test.mjs
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {
  buildSpikeData,
  serializeSpikeData,
  duelMask,
  DUEL,
  OUTPUT,
  MERGED,
} from '../battle-design-spike-data.mjs';

test('committed spike data matches a fresh generation', () => {
  const committed = fs.readFileSync(OUTPUT, 'utf8');
  assert.equal(committed, serializeSpikeData(buildSpikeData()));
});

test('creature records match authored creatures and carry a folded attackMask', () => {
  const data = buildSpikeData();
  const authored = JSON.parse(fs.readFileSync('data/json/creatures.json', 'utf8'));
  assert.equal(data.creatures.length, authored.length);
  for (const creature of data.creatures) {
    const source = authored.find((entry) => entry.id === creature.id);
    assert.equal(creature.name, source.name);
    assert.ok(creature.types.length >= 1 && creature.types.length <= 2);
    assert.ok(creature.maxHp > 30);
    assert.ok(creature.moves.length >= 1, `${creature.name} has at least one attack type`);
    for (const type of creature.moves) assert.ok(type in DUEL.map, `${creature.name}: ${type}`);
    assert.equal(creature.attackMask, duelMask(creature.moves), `${creature.name} attackMask`);
    assert.ok(creature.attackMask > 0 && creature.attackMask < 32, `${creature.name} mask in range`);
    for (const type of creature.types) assert.ok(MERGED.map[type], type);
    for (const merged of creature.merged) assert.ok(MERGED.order.includes(merged));
  }
  // The packed move table is gone.
  assert.ok(!('moves' in data), 'no global move table');
});

test('the frozen duel fold matches data/duel.toml and the kernel order', () => {
  const toml = fs.readFileSync('data/duel.toml', 'utf8');
  assert.deepEqual(DUEL.order, ['spirit', 'water', 'earth', 'fire', 'plant']);
  const order = /order = \[([^\]]+)\]/.exec(toml)[1]
    .split(',').map((token) => token.trim().replace(/"/g, ''));
  assert.deepEqual(order, DUEL.order, 'fold order matches authored duel.toml');
  for (const [from, to] of Object.entries(DUEL.map)) {
    assert.match(toml, new RegExp(`^${from} = "${to}"$`, 'm'), `fold.map ${from} = ${to}`);
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

test('eight-type web satisfies every roster kit pair with a solver proof', async () => {
  const solver = await import('../battle-matchup-solver.mjs');
  const data = buildSpikeData();
  const { order, chart, edges, cycles } = data.wheel;
  assert.equal(order.length, 8);
  assert.ok(cycles.length >= 4, 'overlapping triangles, not a single ring');
  const convention = solver.chartFromEdges(order, edges);
  assert.deepEqual(convention, chart, 'chart mirrors the edge list');
  const sets = solver.rosterSets(data.creatures, order);
  const unsatisfied = solver.unsatisfiedDemands(order, chart, sets);
  assert.equal(unsatisfied.length, 0, `${unsatisfied.length} set pairs without a positive matchup`);
  const decisive = solver.edgeList(order, chart).length;
  const fraction = decisive / (order.length * (order.length - 1));
  assert.ok(fraction > 0.2 && fraction < 0.7, `decisive fraction ${fraction.toFixed(2)}`);
  assert.ok(chart.some((row) => row.includes(1)), 'neutral cells exist');
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
