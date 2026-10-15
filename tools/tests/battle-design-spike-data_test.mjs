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

test('eight-type wheel keeps every cell meaningful', () => {
  const data = buildSpikeData();
  const { order, chart } = data.wheel;
  assert.equal(order.length, 8);
  assert.equal(new Set(order).size, 8);
  order.forEach((type, row) => {
    assert.equal(chart[row].filter((value) => value === 2).length, 1, `${type} beats`);
    assert.equal(chart[row].filter((value) => value === 0.5).length, 1, `${type} loses to`);
    assert.ok(chart[row].every((value) => value !== 0), 'no immunity cells');
    assert.equal(chart[row][row], 1);
  });
  // Every pair resolves one way: win/mirror-loss or neutral/neutral, never both.
  for (let row = 0; row < 8; row++) {
    for (let col = row + 1; col < 8; col++) {
      const forward = chart[row][col], backward = chart[col][row];
      if (forward === 2) assert.equal(backward, 0.5, `${order[row]} vs ${order[col]}`);
      else if (forward === 0.5) assert.equal(backward, 2, `${order[row]} vs ${order[col]}`);
      else assert.equal(backward, 1, `${order[row]} vs ${order[col]}`);
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
