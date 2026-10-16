// Native authoring validation for the first-beta content contract
// (CreatureGathererFX-jp8.11.2). Proves the contract is complete, uses valid
// ids/limits, that the authored area is connected and returnable over the
// GENERATED collision, and that the roster/tables/evolution/labels agree with
// the canonical authored data. Run `make gen` first so fxdata/generated/
// raw_map.bin reflects the current maps/world_map.json.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const readText = (rel) => fs.readFileSync(path.join(ROOT, rel), 'utf8');
const readJson = (rel) => JSON.parse(readText(rel));

// ---------------------------------------------------------------------------
// Minimal, documented TOML reader for the contract's authored subset:
// `# comments`, bare keys, `[table]` / `[[array.of.tables]]` headers, and
// string / integer / boolean / array-of-int / array-of-string values. Enough
// for data/beta-area.toml and data/battle-presets.toml, and small on purpose:
// the shipped data must stay inside this subset.
// ---------------------------------------------------------------------------
function stripTomlComment(line) {
  let out = '';
  let inString = false;
  for (const ch of line) {
    if (ch === '"') inString = !inString;
    if (ch === '#' && !inString) break;
    out += ch;
  }
  return out;
}

function splitTopLevel(text) {
  const parts = [];
  let depth = 0;
  let inString = false;
  let current = '';
  for (const ch of text) {
    if (ch === '"') inString = !inString;
    if (ch === '[' || ch === '{') depth += 1;
    if (ch === ']' || ch === '}') depth -= 1;
    if (ch === ',' && depth === 0 && !inString) {
      parts.push(current);
      current = '';
      continue;
    }
    current += ch;
  }
  if (current.trim() !== '') parts.push(current);
  return parts;
}

function parseTomlScalar(raw) {
  const value = raw.trim();
  if (value.startsWith('"') && value.endsWith('"')) return value.slice(1, -1);
  if (value === 'true') return true;
  if (value === 'false') return false;
  if (value.startsWith('[') && value.endsWith(']')) {
    const inner = value.slice(1, -1).trim();
    return inner === '' ? [] : splitTopLevel(inner).map(parseTomlScalar);
  }
  if (/^-?\d+$/.test(value)) return Number(value);
  throw new Error(`beta-area contract: unsupported TOML value ${raw}`);
}

function descendInto(node, segment) {
  const target = Array.isArray(node) ? node[node.length - 1] : node;
  if (target[segment] === undefined) target[segment] = {};
  return target[segment];
}

function parseToml(text) {
  const root = {};
  let current = root;
  for (const rawLine of text.split(/\r?\n/)) {
    const line = stripTomlComment(rawLine).trim();
    if (line === '') continue;
    let header = line.match(/^\[\[(.+)\]\]$/);
    if (header) {
      const segments = header[1].split('.').map((s) => s.trim());
      let node = root;
      for (let i = 0; i < segments.length - 1; i += 1) node = descendInto(node, segments[i]);
      if (Array.isArray(node)) node = node[node.length - 1];
      const key = segments[segments.length - 1];
      if (!Array.isArray(node[key])) node[key] = [];
      current = {};
      node[key].push(current);
      continue;
    }
    header = line.match(/^\[(.+)\]$/);
    if (header) {
      const segments = header[1].split('.').map((s) => s.trim());
      let node = root;
      for (const segment of segments) node = descendInto(node, segment);
      current = node;
      continue;
    }
    const eq = line.indexOf('=');
    assert.ok(eq > 0, `beta-area contract: malformed TOML line ${JSON.stringify(rawLine)}`);
    current[line.slice(0, eq).trim()] = parseTomlScalar(line.slice(eq + 1));
  }
  return root;
}

// ---------------------------------------------------------------------------
// Canonical authored data.
// ---------------------------------------------------------------------------
const contract = parseToml(readText('data/beta-area.toml'));
const area = contract.area;
const encounters = contract.encounters;
const parties = contract.party.member;
const boss = contract.boss;
const labels = contract.labels.creature;
const edges = contract.evolution.edge;

const creatures = readJson('data/json/creatures.json');
const strings = readText('data/text/strings.txt');
const worldMap = readJson('maps/world_map.json');

const creaturesById = new Map(creatures.map((c) => [c.id, c]));
// Authored attack types are the five folded duel types (wind folds to water,
// lightning to fire and elder to spirit at generation time).
const DUEL_TYPES = new Set(['spirit', 'water', 'earth', 'fire', 'plant']);

function battlePresets() {
  return parseToml(readText('data/battle-presets.toml')).preset;
}

// Generated collision: property words are tileId in the low 10 bits and six
// property bits above bit 10; walkable is property bit 0 (TileProps.hpp).
const GID_MASK = 0x03ff;
const PROP_SHIFT = 10;
const PROP_WALKABLE = 1;
function tileIdOf(word) {
  return word & GID_MASK;
}
function isWalkableWord(word) {
  return tileIdOf(word) !== 0 && (((word >> PROP_SHIFT) & 0x3f) & PROP_WALKABLE) !== 0;
}

function loadGeneratedCollision() {
  const file = path.join(ROOT, 'fxdata/generated/raw_map.bin');
  if (!fs.existsSync(file)) {
    throw new Error('stale/missing generated collision: run `make gen` before `node --test tools/tests/beta-area_test.mjs`');
  }
  const buffer = fs.readFileSync(file);
  assert.equal(buffer.length, 256 * 256 * 2, 'raw_map.bin must be 256x256 property words');
  const word = (x, y) => buffer.readUInt16LE((y * 256 + x) * 2);
  // Staleness gate: every authored cell in the bounded area must already be
  // regenerated, otherwise fail with a clear instruction instead of a mystery.
  for (let y = area.min_y; y <= area.max_y; y += 1) {
    for (let x = area.min_x; x <= area.max_x; x += 1) {
      const gid = worldMap.tiles[y][x];
      const generated = tileIdOf(word(x, y));
      assert.equal(
        generated,
        gid,
        `stale generated collision at (${x},${y}): raw_map has ${generated}, world_map.json has ${gid}; run \`make gen\``,
      );
    }
  }
  return word;
}

function bfsReachable(word, startX, startY) {
  const visited = new Set();
  const key = (x, y) => `${x},${y}`;
  if (!isWalkableWord(word(startX, startY))) return visited;
  const queue = [[startX, startY]];
  visited.add(key(startX, startY));
  while (queue.length) {
    const [x, y] = queue.shift();
    for (const [dx, dy] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
      const nx = x + dx;
      const ny = y + dy;
      if (nx < 0 || ny < 0 || nx >= 256 || ny >= 256) continue;
      if (visited.has(key(nx, ny))) continue;
      if (!isWalkableWord(word(nx, ny))) continue;
      visited.add(key(nx, ny));
      queue.push([nx, ny]);
    }
  }
  return visited;
}

function reachablePoint(visited, word, x, y) {
  if (isWalkableWord(word(x, y))) return visited.has(`${x},${y}`);
  return [[1, 0], [-1, 0], [0, 1], [0, -1]].some(([dx, dy]) =>
    visited.has(`${x + dx},${y + dy}`));
}

// ---------------------------------------------------------------------------
// Contract completeness.
// ---------------------------------------------------------------------------
test('contract pins every beta area/roster/table/reward/evolution/label field', () => {
  assert.equal(contract.contract, 'first-beta');
  assert.equal(contract.scope, 'beta-area');
  for (const key of ['min_x', 'max_x', 'min_y', 'max_y', 'start_x', 'start_y',
    'rest_x', 'rest_y', 'box_x', 'box_y', 'plant_x', 'plant_y', 'boss_x', 'boss_y']) {
    assert.ok(Number.isInteger(area[key]), `area.${key} must be pinned`);
  }
  assert.deepEqual([area.min_x, area.max_x, area.min_y, area.max_y], [8, 23, 0, 11]);
  assert.deepEqual([area.start_x, area.start_y], [11, 5]);
  assert.deepEqual([area.box_x, area.box_y], [9, 2]);
  assert.deepEqual([area.plant_x, area.plant_y], [14, 2]);
  assert.deepEqual([area.rest_x, area.rest_y], [10, 4]);
  assert.deepEqual([area.boss_x, area.boss_y], [21, 9]);
  assert.equal(boss.victory_flag, 'flag_beta_complete');
  assert.equal(parties.length, 3, 'exactly three starter party members');
  assert.equal(contract.party.bag_empty, true, 'initial bag must be empty');
  assert.ok(Number.isInteger(contract.plant_source.item_id));
  for (const key of ['lure_type', 'lure_tier', 'rate', 'need_min', 'need_max', 'turns']) {
    assert.ok(Number.isInteger(contract.renewable[key]), `renewable.${key} must be pinned`);
  }
  assert.equal(encounters.table_count, 5);
  assert.equal(encounters.slots_per_table, 10);
  assert.equal(contract.encounters.table.length, 5, 'exactly five encounter tables');
  assert.equal(boss.species, 31);
  assert.equal(edges.length, 3, 'exactly three first-stage evolution edges');
  assert.ok(labels.length >= 4, 'starter and boss labels must be pinned');
});

// ---------------------------------------------------------------------------
// Valid ids and limits.
// ---------------------------------------------------------------------------
test('contract ids, levels and slots stay within canonical limits', () => {
  const allowedSpecies = new Set(encounters.allowed_species);
  for (const member of parties) {
    assert.ok(creaturesById.has(member.species), `party species ${member.species} exists`);
    assert.equal(member.level, 3, 'starter party is level 3');
    assert.equal(member.full_hp, true, 'starter party starts at full HP');
    assert.ok(member.moves.length >= 1 && member.moves.length <= 5,
      'each starter knows 1..5 distinct attack types');
    assert.equal(new Set(member.moves).size, member.moves.length,
      'starter attack types are distinct');
    for (const type of member.moves) {
      assert.ok(DUEL_TYPES.has(type), `party type ${type} is a duel type`);
    }
    assert.deepEqual(member.moves, creaturesById.get(member.species).moves,
      `party member ${member.species} types match the species authored moves`);
  }
  assert.deepEqual(parties.map((m) => m.species), [0, 3, 6]);

  const plant = contract.plant_source;
  assert.equal(plant.item_id, plant.lure_type * 3 + plant.lure_tier, 'plant item id is type*3+tier');
  assert.ok(plant.lure_tier >= 0 && plant.lure_tier <= 2, 'plant tier 0..2');
  assert.ok(plant.lure_type >= 0 && plant.lure_type <= 3, 'plant type is a supported 0..3');
  assert.ok(plant.item_id < 24, 'plant item id within the 8x3 lure id space');
  assert.ok(plant.count >= 1 && plant.count <= 99, 'plant count within the item cap');

  assert.equal(encounters.table.length, 5);
  for (const table of encounters.table) {
    assert.ok([-1, 0, 1, 2, 3].includes(table.lure_type), `table ${table.name} lure_type in -1..3`);
    assert.equal(table.species.length, 10, `table ${table.name} has exactly ten slots`);
    for (const species of table.species) {
      assert.ok(allowedSpecies.has(species), `table ${table.name} species ${species} is allowed`);
      assert.ok(creaturesById.has(species), `table ${table.name} species ${species} exists`);
    }
  }
  assert.ok(Number.isInteger(boss.level) && boss.level >= 1 && boss.level <= 31);
  assert.equal(boss.trainer, true);
  assert.equal(boss.gather_allowed, false);
  assert.equal(boss.escape_allowed, false);
  assert.equal(boss.repeat_reward, false);
  assert.equal(typeof boss.victory_flag, 'string');
  assert.ok(boss.victory_flag.length > 0);

  for (const edge of edges) {
    assert.ok(creaturesById.has(edge.from_id), `evolution from ${edge.from_id} exists`);
    assert.ok(creaturesById.has(edge.to_id), `evolution to ${edge.to_id} exists`);
    assert.notEqual(edge.from_id, edge.to_id, 'evolution edge is not a self-loop');
  }
});

// ---------------------------------------------------------------------------
// Reachability over generated collision.
// ---------------------------------------------------------------------------
test('authored area is connected start->plant->box->rest->boss and back', () => {
  const word = loadGeneratedCollision();
  assert.ok(isWalkableWord(word(area.start_x, area.start_y)), 'start must be walkable');

  const fromStart = bfsReachable(word, area.start_x, area.start_y);
  const checks = [
    ['plant', area.plant_x, area.plant_y],
    ['box', area.box_x, area.box_y],
    ['rest', area.rest_x, area.rest_y],
    ['boss', area.boss_x, area.boss_y],
  ];
  for (const [name, x, y] of checks) {
    assert.ok(reachablePoint(fromStart, word, x, y), `${name} (${x},${y}) must be reachable from start`);
  }

  // Explicit return route: BFS from the boss approach must reach start and the
  // other landmarks. The generated collision is undirected, so the return trip
  // exists iff both traversals cover the area.
  const bossApproach = [[1, 0], [-1, 0], [0, 1], [0, -1]]
    .map(([dx, dy]) => [area.boss_x + dx, area.boss_y + dy])
    .find(([x, y]) => isWalkableWord(word(x, y)));
  assert.ok(bossApproach, 'boss interaction must have a walkable approach tile');
  const fromBoss = bfsReachable(word, bossApproach[0], bossApproach[1]);
  assert.ok(fromBoss.has(`${area.start_x},${area.start_y}`), 'start must be reachable back from the boss');
  for (const [name, x, y] of checks) {
    assert.ok(reachablePoint(fromBoss, word, x, y), `${name} must be reachable on the return route`);
  }
});

test('lure-zone map pins are preserved byte-for-byte', () => {
  assert.equal(worldMap.tiles[2][9], 545, 'box pin tiles[2][9]');
  assert.equal(worldMap.tiles[2][14], 551, 'plant pin tiles[2][14]');
  assert.equal(worldMap.tiles[7][12], 275, 'interior floor pin tiles[7][12]');
});

// ---------------------------------------------------------------------------
// Shipping fresh-game start literal (CreatureGathererFX-o845).
// ---------------------------------------------------------------------------
// setup() publishes a fresh ordinary game's start from a hand-written
// playerLocation literal (not a generated symbol), so nothing but this test ties
// it to the authored contract. Anchored on the fresh-game block so the
// guard-only CGFX_WILD_DEMO / CGFX_LURE_WEDGE literals cannot satisfy it.
test('shipping fresh-game setup literal matches the authored start', () => {
  const ino = readText('CreatureGathererFX.ino');
  const anchor = ino.indexOf('First-beta authored start');
  assert.ok(anchor >= 0, 'shipping setup keeps the authored-start literal marker');
  const literal = ino.slice(anchor).match(
    /gameState\.playerLocation\s*=\s*static_cast<uint16_t>\((\d+)\)\s*\|\s*\(\s*static_cast<uint16_t>\((\d+)\)\s*<<\s*8\s*\)/);
  assert.ok(literal, 'fresh-game setup publishes a playerLocation start literal');
  assert.equal(Number(literal[1]), area.start_x,
    'shipping fresh-game literal x must equal the authored start_x');
  assert.equal(Number(literal[2]), area.start_y,
    'shipping fresh-game literal y must equal the authored start_y');
});

// ---------------------------------------------------------------------------
// Renewable acquisition math.
// ---------------------------------------------------------------------------
test('tier1 lure rate meets the encounter need within six turns', () => {
  const r = contract.renewable;
  assert.equal(r.rate, 1 << r.lure_tier, 'rate is 1<<tier');
  assert.equal(r.lure_type, contract.plant_source.lure_type, 'renewable lure matches plant yield type');
  assert.equal(r.lure_tier, contract.plant_source.lure_tier, 'renewable lure matches plant yield tier');
  const needFor = (level) => Math.max(8, Math.min(24, 8 + Math.floor(level / 2)));
  const needs = [];
  for (let level = encounters.level_min; level <= encounters.level_max; level += 1) needs.push(needFor(level));
  assert.equal(Math.min(...needs), r.need_min, 'need_min matches authored encounter levels');
  assert.equal(Math.max(...needs), r.need_max, 'need_max matches authored encounter levels');
  assert.ok(r.rate * r.turns >= r.need_max,
    `rate ${r.rate} x ${r.turns} turns must meet need ${r.need_max} without luck`);
});

// ---------------------------------------------------------------------------
// Evolution edges versus canonical creatures.
// ---------------------------------------------------------------------------
test('evolution edges match canonical evelevel and the named pairs', () => {
  const pairs = edges.map((e) => [e.from_id, e.to_id]);
  assert.deepEqual(pairs, [[0, 1], [3, 4], [6, 7]]);
  for (const edge of edges) {
    assert.equal(edge.level, 8, 'first-stage evolution threshold is level 8');
    assert.equal(creaturesById.get(edge.from_id).evelevel, edge.level,
      `creature ${edge.from_id} authored evelevel must equal the edge level`);
  }
  // Source/target symbol agreement (jp8.11.11): the canonical species target in
  // creatures.json must name the same target as the beta-area contract, and no
  // other species may author an evolution target at this revision.
  const authored = creatures.filter((c) => c.evolves_to !== undefined);
  assert.deepEqual(authored.map((c) => [c.id, c.evolves_to]),
    pairs, 'creatures.json evolves_to targets must match the contract edges');
  for (const edge of edges) {
    assert.equal(creaturesById.get(edge.from_id).evolves_to, edge.to_id,
      `creature ${edge.from_id} canonical evolves_to must match edge target`);
  }
});

// ---------------------------------------------------------------------------
// Label widths versus strings.txt.
// ---------------------------------------------------------------------------
test('battle label widths match authored creature labels', () => {
  const creatureLabels = {};
  for (const match of strings.matchAll(/creature(\d+)\s*=\s*"([^"]*)"/g)) {
    creatureLabels[Number(match[1])] = match[2];
  }
  assert.ok(Number.isInteger(contract.labels.glyph_width) && contract.labels.glyph_width > 0);
  for (const label of labels) {
    assert.equal(creatureLabels[label.species], label.text,
      `label text for species ${label.species} matches strings.txt`);
    assert.equal(label.width, contract.labels.glyph_width * label.text.length,
      `label width for species ${label.species} is glyph_width * length`);
  }
});

// ---------------------------------------------------------------------------
// Encounter table constraints.
// ---------------------------------------------------------------------------
test('encounter tables use only first-area species across all four type groups', () => {
  const allowed = new Set(encounters.allowed_species);
  const groupTables = new Map();
  for (const table of encounters.table) {
    for (const species of table.species) {
      assert.ok(allowed.has(species), `table ${table.name} excludes out-of-area species ${species}`);
      assert.ok(species !== 21 && species !== 22, 'placeholder item species are excluded');
      assert.ok(species < 32, 'later/expansion species are excluded');
    }
    assert.ok(encounters.level_min >= 2 && encounters.level_max <= 5, 'table levels are 2..5');
    if (table.lure_type >= 0) groupTables.set(table.lure_type, table);
  }
  const typeNames = ['spirit', 'water', 'wind', 'earth'];
  for (let type = 0; type <= 3; type += 1) {
    const table = groupTables.get(type);
    assert.ok(table, `type group ${type} has a table`);
    assert.ok(table.species.some((id) => creaturesById.get(id).type1 === typeNames[type]),
      `type group ${type} table contains a ${typeNames[type]} species`);
  }
  const base = encounters.table.find((t) => t.lure_type < 0);
  assert.ok(base, 'a base mixed table exists');
  assert.ok(new Set(base.species).size >= 4, 'base table is a mixed roster');
});

// ---------------------------------------------------------------------------
// Cross-file agreement.
// ---------------------------------------------------------------------------
test('battle-presets beta roster agrees with the contract party', () => {
  const beta = battlePresets().find((p) => p.name === 'beta');
  assert.ok(beta, 'data/battle-presets.toml pins a "beta" preset');
  assert.equal(beta.player.length, 3);
  for (let i = 0; i < 3; i += 1) {
    const preset = beta.player[i];
    const member = parties[i];
    assert.equal(preset.level, member.level, `beta preset member ${i} level`);
    assert.equal(creaturesById.get(member.species).name, preset.species,
      `beta preset member ${i} species name`);
    assert.deepEqual(preset.moves, member.moves,
      `beta preset member ${i} attack types match the contract`);
  }
});

test('starter attack types are the species authored type list', () => {
  for (const member of parties) {
    const authored = creaturesById.get(member.species).moves;
    assert.ok(member.moves.length >= 1, `species ${member.species} knows at least one type`);
    for (const type of member.moves) {
      assert.ok(authored.includes(type),
        `species ${member.species} authored moves include ${type}`);
    }
  }
});
