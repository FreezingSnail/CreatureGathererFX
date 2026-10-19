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
  assert.equal(edges.length, 6, 'six authored evolution edges (two per starter chain)');
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
// Shipping fresh-game party wiring (CreatureGathererFX-jp8.11.24).
// ---------------------------------------------------------------------------
// The ordinary !restored branch must apply the generated authored "beta" preset
// and seed the collection; the legacy species-only default load is gone and the
// start stays inside the fresh-game guard so a valid restore keeps its party.
test('shipping fresh-game boot applies the authored beta preset only when fresh', () => {
  const ino = readText('CreatureGathererFX.ino');
  const marker = 'Ordinary new game';
  const at = ino.indexOf(marker);
  assert.ok(at >= 0, 'shipping setup keeps the ordinary new-game marker');
  const seedAt = ino.indexOf('SaveController::seedParty()', at);
  assert.ok(seedAt > at, 'ordinary new game seeds the collection');
  const block = ino.slice(at, seedAt);
  assert.ok(block.includes('battle::applyPlayerPreset(player, BattlePresets::beta)'),
    'ordinary boot applies the generated beta preset');
  assert.ok(block.includes('item::setBattleInventory(&player.inventory)'),
    'ordinary boot keeps the one bag wired for battle use');
  assert.ok(!block.includes('player.basic()'),
    'ordinary boot no longer uses the legacy species-only default load');
  const guard = ino.lastIndexOf('if (!restored)', at);
  assert.ok(guard >= 0 && at - guard < 200,
    'beta start stays inside the !restored branch');
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
// Growth pacing arithmetic (CreatureGathererFX-jp8.11.26).
// ---------------------------------------------------------------------------
// The brief docs/first-beta-pacing-brief.md pins these controls. Assertions read
// the shipping sources so a future rebalance (curve, gain constant or encounter
// level range) fails loudly instead of the numbers silently drifting.
test('exp curve, ordinary-wild gain constant and win bounds to the level-8 edge', () => {
  // LEVEL_EXP[n] = n^3 from the shipping level curve.
  const levelCurve = readText('src/creature/LevelCurve.hpp');
  const tableMatch = levelCurve.match(/LEVEL_EXP\[LEVEL_COUNT\][^=]*=\s*\{([^}]*)\}/);
  assert.ok(tableMatch, 'LevelCurve.hpp declares the LEVEL_EXP table');
  const LEVEL_EXP = tableMatch[1]
    .split(',').map((v) => v.trim()).filter((v) => v !== '').map(Number);
  assert.equal(LEVEL_EXP.length, 32, 'the level curve carries 32 cumulative thresholds');
  for (let n = 0; n < LEVEL_EXP.length; n += 1) {
    assert.equal(LEVEL_EXP[n], n * n * n, `LEVEL_EXP[${n}] must equal n^3`);
  }
  assert.equal(LEVEL_EXP[3], 27, 'level-3 threshold is 27');
  assert.equal(LEVEL_EXP[8], 512, 'level-8 threshold is 512');

  // First-stage evolution is the level-8 edge in the contract.
  const firstStageLevel = Math.min(...edges.map((e) => e.level));
  assert.equal(firstStageLevel, 8, 'the first evolution edge is level 8');
  const startLevel = parties[0].level;
  assert.equal(startLevel, 3, 'party start level is 3 (contract)');
  const expToEdge = LEVEL_EXP[firstStageLevel] - LEVEL_EXP[startLevel];
  assert.equal(expToEdge, 485, 'exp from level 3 to the level-8 edge is 512-27');

  // Ordinary-wild exp gain is 8 * foeLevel, read from the shipping source.
  const battleFlow = readText('src/engine/battle/BattleFlow.cpp');
  const gainMatch = battleFlow.match(
    /const uint16_t gain = static_cast<uint16_t>\(\s*state\.active\[[^\]]*\]\.level\) \* (\d+)u;/);
  assert.ok(gainMatch,
    'BattleFlow.cpp still computes the ordinary-wild gain as foeLevel * constant');
  const gainPerLevel = Number(gainMatch[1]);
  assert.equal(gainPerLevel, 8, 'ordinary-wild exp gain constant is 8 * foeLevel');

  const levelMin = encounters.level_min;
  const levelMax = encounters.level_max;
  assert.deepEqual([levelMin, levelMax], [2, 5], 'authored encounter levels are 2..5');
  const minGain = gainPerLevel * levelMin;
  const maxGain = gainPerLevel * levelMax;
  assert.deepEqual([minGain, maxGain], [16, 40], 'win gains span 16..40 exp');
  assert.equal(Math.ceil(expToEdge / maxGain), 13, 'fastest path to the level-8 edge is 13 wins');
  assert.equal(Math.ceil(expToEdge / minGain), 31, 'slowest path to the level-8 edge is 31 wins');
});

test('boss level 8 and the level-8 edges are the authored contract values', () => {
  assert.equal(boss.level, 8, 'boss level 8 comes from the contract');
  const level8Edges = edges.filter((e) => e.level === boss.level);
  assert.equal(level8Edges.length, 3, 'three first-stage edges share the boss level');
  for (const edge of level8Edges) {
    assert.equal(creaturesById.get(edge.from_id).evelevel, boss.level,
      `creature ${edge.from_id} authored evelevel equals the boss level`);
  }
});

// ---------------------------------------------------------------------------
// Evolution edges versus canonical creatures.
// ---------------------------------------------------------------------------
test('evolution edges match canonical evelevel and the named pairs', () => {
  const pairs = edges.map((e) => [e.from_id, e.to_id]);
  assert.deepEqual(pairs, [[0, 1], [1, 2], [3, 4], [4, 5], [6, 7], [7, 8]]);
  for (const edge of edges) {
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
// First-area reward mapping (CreatureGathererFX-jp8.11.3).
// ---------------------------------------------------------------------------
// The generator derives the packed Rewards mapping from the contract. Prove the
// generated descriptor agrees with the authored source and that src/fxdata.h
// publishes the same symbols, so the shipped cart and header cannot drift.
test('generated reward mapping agrees with the authored contract and header', () => {
  const descriptor = readText('fxdata/generated/rewards.txt');
  const header = readText('src/fxdata.h');
  const array = (name) => {
    const match = descriptor.match(new RegExp(`uint8_t ${name}\\[\\] = \\{([^}]*)\\};`));
    assert.ok(match, `rewards.txt declares ${name}`);
    return match[1].trim() === '' ? [] : match[1].split(',').map((v) => Number(v.trim()));
  };

  // Primary type index matches the canonical 8-type order.
  const TYPE_INDEX = { spirit: 0, water: 1, wind: 2, earth: 3, fire: 4, lightning: 5, plant: 6, elder: 7 };
  const allowed = new Set(encounters.allowed_species);
  const materialItem = array('materialItem');
  const materialCount = array('materialCount');
  assert.equal(materialItem.length, creatures.length, 'materialItem is species-indexed');
  assert.equal(materialCount.length, creatures.length, 'materialCount is species-indexed');

  for (let species = 0; species < creatures.length; species += 1) {
    const creature = creaturesById.get(species);
    assert.ok(creature, `roster species ${species} exists`);
    if (allowed.has(species)) {
      const lureType = TYPE_INDEX[creature.type1] & 3;
      assert.equal(materialItem[species], lureType * 3 + 2,
        `species ${species} material is the tier-2 lure of its folded type group`);
      assert.equal(materialItem[species] % 3, 2, `species ${species} material tier is 2`);
      assert.ok(materialItem[species] / 3 < 4, `species ${species} material type is 0..3`);
      assert.equal(materialCount[species], 1, `species ${species} material count is exact`);
    } else {
      assert.equal(materialItem[species], 255, `species ${species} material sentinel`);
      assert.equal(materialCount[species], 0, `species ${species} material count sentinel`);
    }
  }

  const plant = contract.plant_source;
  assert.deepEqual(array('plantItem'), [plant.item_id], 'plant item matches plant_source');
  assert.deepEqual(array('plantCount'), [plant.count], 'plant count matches plant_source');
  assert.equal(plant.item_id % 3, 1, 'plant reward is a tier-1 lure');

  const consumableRows = readText('data/consumables.csv')
    .split(/\r?\n/)
    .filter((line) => line.trim() !== '' && !line.startsWith('#'));
  const kindById = new Map();
  for (const row of consumableRows.slice(1)) {
    const [id, kind] = row.split(',').map((v) => v.trim());
    kindById.set(Number(id), kind);
  }
  const heals = (contract.reward && contract.reward.heal) || [];
  assert.deepEqual(array('healItem'), heals.map((h) => h.item_id), 'heal items match the contract');
  assert.deepEqual(array('healCount'), heals.map((h) => h.count), 'heal counts match the contract');
  for (const heal of heals) {
    assert.ok(heal.item_id >= 0 && heal.item_id < 8, 'heal id is a consumable');
    assert.equal(kindById.get(heal.item_id), 'Heal', `heal ${heal.item_id} is a Heal definition`);
    assert.ok(heal.count >= 1 && heal.count <= 99, 'heal count within the item cap');
  }

  for (const symbol of ['materialItem', 'materialCount', 'plantItem', 'plantCount', 'healItem', 'healCount']) {
    assert.ok(header.includes(`namespace Rewards`), 'header publishes the Rewards namespace');
    assert.ok(new RegExp(`constexpr uint24_t ${symbol} = `).test(header),
      `header publishes Rewards::${symbol}`);
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

// ---------------------------------------------------------------------------
// Authored first-beta map content (CreatureGathererFX-jp8.11.25).
// ---------------------------------------------------------------------------
// The packed encounter/zone bytes and the authored warp pair are the content
// that makes the ordinary loop playable. These tests read the packed
// fxdata/data/encounters.txt ABI text and the generated collision, never
// literal generated indices, so a one-sided edit of the contract, the zone
// ABI or the map fails the standard gate.
const ENCOUNTER_ZONE_BYTES = 10;
const ENCOUNTER_TABLE_BYTES = 12;
const ENCOUNTER_SLOT_COUNT = 10;
const ENCOUNTER_ZONE_COUNT = 8;
const ENCOUNTER_TABLE_COUNT = 5;
const ENCOUNTER_NIBBLE_COUNT = 4;
const ENCOUNTER_SENTINEL = 0xffff;
// Encounter tiles are exactly the tiles whose tileset tile declares the
// `encounter` property; the packed property word carries it above bit 10.
const PROP_ENCOUNTER_BIT = 1 << 4;

function parseEncounterSource() {
  const source = readText('fxdata/data/encounters.txt');
  const array = (name) => {
    const match = source.match(new RegExp(`uint8_t ${name}\\[\\] = \\{([^}]*)\\};`));
    assert.ok(match, `encounters.txt declares ${name}`);
    return match[1].split(',').map((v) => v.trim())
      .filter((v) => v !== '').map(Number);
  };
  return { zones: array('zoneDefs'), tables: array('tables') };
}

// The `tp @from @to` source/destination pair of an authored warp script, or
// null for any other script text (the flag-gated legacy demo is neither).
function warpPair(scriptText) {
  const match = scriptText.match(/^tp\s+@(\w+)\s+@(\w+);?$/);
  return match ? { from: match[1], to: match[2] } : null;
}

function inArea(x, y) {
  return x >= area.min_x && x <= area.max_x && y >= area.min_y && y <= area.max_y;
}

test('packed encounter tables equal the five authored contract groups in order', () => {
  const { tables } = parseEncounterSource();
  const expectedTables = [-1, 0, 1, 2, 3]
    .map((lureType) => encounters.table.find((t) => t.lure_type === lureType));
  assert.equal(expectedTables.filter(Boolean).length, ENCOUNTER_TABLE_COUNT,
    'the contract authors the base table plus lure groups 0..3');
  assert.equal(tables.length, ENCOUNTER_TABLE_COUNT * ENCOUNTER_TABLE_BYTES,
    'encounters.txt packs exactly five tables');
  for (let index = 0; index < ENCOUNTER_TABLE_COUNT; index += 1) {
    const table = expectedTables[index];
    const base = index * ENCOUNTER_TABLE_BYTES;
    assert.deepEqual(tables.slice(base, base + ENCOUNTER_SLOT_COUNT), table.species,
      `packed table ${index} (${table.name}) species match the contract`);
    assert.equal(tables[base + ENCOUNTER_SLOT_COUNT], encounters.level_min,
      `packed table ${table.name} floor is the authored level_min`);
    assert.equal(tables[base + ENCOUNTER_SLOT_COUNT + 1], encounters.level_max,
      `packed table ${table.name} ceil is the authored level_max`);
  }
});

test('authored zone defs cover every route chunk and map each lure type to its themed table', () => {
  const { zones } = parseEncounterSource();
  assert.equal(zones.length, ENCOUNTER_ZONE_COUNT * ENCOUNTER_ZONE_BYTES,
    'encounters.txt packs exactly the fixed zone slots');

  // Encounter tiles come from the tileset property, so the zone coverage is
  // derived from the authored tiles, not from the map's (unpacked) grids.
  const encounterGids = new Set();
  for (const match of readText('maps/tiles.tsx')
    .matchAll(/<tile id="(\d+)"[^>]*>([\s\S]*?)<\/tile>/g)) {
    if (/name="encounter" type="bool" value="true"/.test(match[2])) {
      encounterGids.add(Number(match[1]) + 1);
    }
  }
  assert.ok(encounterGids.size > 0, 'the tileset authors an encounter property');

  const word = loadGeneratedCollision();
  const routeChunks = new Set();
  let encounterCells = 0;
  for (let y = area.min_y; y <= area.max_y; y += 1) {
    for (let x = area.min_x; x <= area.max_x; x += 1) {
      if (!encounterGids.has(worldMap.tiles[y][x])) continue;
      encounterCells += 1;
      assert.notEqual((word(x, y) >> PROP_SHIFT) & PROP_ENCOUNTER_BIT, 0,
        `packed encounter property at (${x},${y})`);
      assert.notEqual(tileIdOf(word(x, y)), 0, `packed encounter tile at (${x},${y})`);
      routeChunks.add(Math.floor(y / 4) * 32 + Math.floor(x / 8));
    }
  }
  assert.ok(encounterCells > 0, 'the authored area carries encounter tiles');

  const activeZones = [];
  let sentinelSeen = false;
  for (let index = 0; index < ENCOUNTER_ZONE_COUNT; index += 1) {
    const base = index * ENCOUNTER_ZONE_BYTES;
    const chunk = zones[base] | (zones[base + 1] << 8);
    if (chunk === ENCOUNTER_SENTINEL) {
      sentinelSeen = true;
      assert.deepEqual(zones.slice(base + 2, base + ENCOUNTER_ZONE_BYTES),
        new Array(ENCOUNTER_ZONE_BYTES - 2).fill(0), 'sentinel zone tail is zeroed');
      continue;
    }
    assert.equal(sentinelSeen, false, 'active zones precede the sentinel tail');
    activeZones.push({
      chunk,
      tableBase: zones[base + 2],
      tableCount: zones[base + 3],
      typeMask: zones[base + 4],
      nibbles: zones.slice(base + 5, base + 5 + ENCOUNTER_NIBBLE_COUNT),
      levelOffset: zones[base + 9],
    });
  }

  const covered = new Set(activeZones.map((zone) => zone.chunk));
  for (const chunk of routeChunks) {
    assert.ok(covered.has(chunk),
      `route chunk ${chunk} holds encounter tiles and must have an authored zone`);
  }
  assert.deepEqual([...covered].sort((a, b) => a - b),
    [...routeChunks].sort((a, b) => a - b),
    'the authored zones are exactly the route chunks');

  const groupByLureType = new Map([-1, 0, 1, 2, 3].map((lureType) => [lureType,
    encounters.table.find((t) => t.lure_type === lureType)]));
  const startAverage = Math.floor(parties.reduce((sum, m) => sum + m.level, 0) / parties.length);
  for (const zone of activeZones) {
    assert.equal(zone.tableBase, 0, `zone ${zone.chunk} base is the first packed table`);
    assert.equal(zone.tableCount, ENCOUNTER_TABLE_COUNT,
      `zone ${zone.chunk} spans all five authored tables`);
    assert.equal(zone.typeMask & ~((1 << ENCOUNTER_NIBBLE_COUNT) - 1), 0,
      `zone ${zone.chunk} type mask names only supported lure types`);
    for (let lureType = 0; lureType < ENCOUNTER_NIBBLE_COUNT; lureType += 1) {
      assert.equal(zone.nibbles[lureType], lureType + 1,
        `zone ${zone.chunk} lure type ${lureType} selects its themed table offset`);
      const themed = groupByLureType.get(lureType);
      assert.ok(themed, `the contract authors lure group ${lureType}`);
      assert.equal(zone.tableBase + zone.nibbles[lureType],
        [-1, 0, 1, 2, 3].indexOf(themed.lure_type),
        `zone ${zone.chunk} lure type ${lureType} maps to the ${themed.name} table bytes`);
    }
    const level = startAverage + zone.levelOffset;
    assert.ok(level >= encounters.level_min && level <= encounters.level_max,
      `zone ${zone.chunk} level offset keeps the fresh party inside the authored 2..5 band`);
  }
});

test('only the in-bounds beta door pair warps, and the authored area stays closed', () => {
  const word = loadGeneratedCollision();
  const locations = new Map(worldMap.npcs.map((npc) => [npc.script, npc]));
  for (const [scriptName, npc] of locations) {
    assert.match(scriptName, /^@/, 'every authored map marker is a @location');
    assert.ok(inArea(npc.x, npc.y), `location ${scriptName} stays inside the authored area`);
  }

  const warps = [];
  for (const script of worldMap.scripts) {
    const pair = warpPair(script.script);
    if (!pair) continue;
    const from = locations.get(`@${pair.from}`);
    const to = locations.get(`@${pair.to}`);
    assert.ok(from && to, `warp ${script.script} names authored locations`);
    warps.push({ script, from, to });
  }
  assert.equal(warps.length, 2, 'the area authors exactly one reciprocal door pair');
  const [door, gate] = warps;
  assert.deepEqual([door.from.x, door.from.y], [gate.to.x, gate.to.y],
    'the door pair is reciprocal (door source is the gate destination)');
  assert.deepEqual([door.to.x, door.to.y], [gate.from.x, gate.from.y],
    'the door pair is reciprocal (gate source is the door destination)');

  const reach = bfsReachable(word, area.start_x, area.start_y);
  const bossApproach = { x: area.boss_x - 1, y: area.boss_y };
  for (const warp of warps) {
    for (const end of [warp.from, warp.to]) {
      assert.ok(inArea(end.x, end.y),
        `warp endpoint (${end.x},${end.y}) stays inside the authored area`);
      assert.ok(isWalkableWord(word(end.x, end.y)),
        `warp endpoint (${end.x},${end.y}) is walkable`);
      assert.ok(reach.has(`${end.x},${end.y}`),
        `warp endpoint (${end.x},${end.y}) is on the reachable route`);
      assert.equal(end.x === bossApproach.x && end.y === bossApproach.y, false,
        'the boss approach tile carries no warp endpoint');
    }
  }
  for (const script of worldMap.scripts) {
    const pair = warpPair(script.script);
    if (!pair) continue;
    const to = locations.get(`@${pair.to}`);
    assert.ok(inArea(to.x, to.y), `warp ${script.script} never leaves the authored area`);
  }

  // The legacy out-of-area demo warps are gone; the only remaining extra script
  // is the flag-gated door demo, which is inert because nothing sets flag_door.
  for (const script of worldMap.scripts) {
    assert.ok(!/@test_house|@test_teleport/.test(script.script),
      'the pre-beta test warp fixtures are removed');
  }
  const flagScripts = worldMap.scripts.filter((script) => /flag_door/.test(script.script));
  assert.ok(flagScripts.length >= 1, 'the generated flag_door demo script is preserved');
  assert.ok(!worldMap.scripts.some((script) => /\bsetflag\b/i.test(script.script)),
    'no map script sets flag_door, so the gated demo warp cannot fire');
  const flagsHeader = readText('src/flags/flags.hpp');
  assert.match(flagsHeader, /uint16_t flag_door = 0;/,
    'generated flag_door keeps its index-0 ABI');

  // The authored area is sealed by walking: the start's whole reachable set
  // lies inside the authored bounds.
  assert.ok(reach.size > 0, 'the start reaches authored walkable cells');
  let areaWalkable = 0;
  for (let y = area.min_y; y <= area.max_y; y += 1) {
    for (let x = area.min_x; x <= area.max_x; x += 1) {
      if (isWalkableWord(word(x, y))) areaWalkable += 1;
    }
  }
  for (const key of reach) {
    const [x, y] = key.split(',').map(Number);
    assert.ok(inArea(x, y), `reachable cell (${x},${y}) must stay inside the authored area`);
  }
  assert.equal(reach.size, areaWalkable,
    'every walkable cell of the authored area is reachable and none outside is');
});

test('the authored routes carry encounter tiles in every reachable chunk', () => {
  const word = loadGeneratedCollision();
  const reach = bfsReachable(word, area.start_x, area.start_y);
  const reachChunks = new Set();
  for (const key of reach) {
    const [x, y] = key.split(',').map(Number);
    reachChunks.add(Math.floor(y / 4) * 32 + Math.floor(x / 8));
  }
  const { zones } = parseEncounterSource();
  const covered = new Set();
  for (let index = 0; index < ENCOUNTER_ZONE_COUNT; index += 1) {
    const base = index * ENCOUNTER_ZONE_BYTES;
    const chunk = zones[base] | (zones[base + 1] << 8);
    if (chunk !== ENCOUNTER_SENTINEL) covered.add(chunk);
  }
  for (const chunk of reachChunks) {
    assert.ok(covered.has(chunk),
      `reachable chunk ${chunk} must have an authored zone so a deposited lure survives`);
  }
  // A deposited lure is cleared when the destination chunk has encounter tiles
  // but no zone, so the reachable set must be fully covered.
  const contact = [...reach].every((key) => {
    const [x, y] = key.split(',').map(Number);
    return (word(x, y) >> PROP_SHIFT) & PROP_ENCOUNTER_BIT;
  });
  assert.equal(contact, true, 'every reachable route cell is an encounter tile');
});

test('the unpacked map encounter grids stay inert sentinels', () => {
  assert.equal(worldMap.encounter_zones.length, 0,
    'the unpacked encounter_zones list stays empty');
  for (let y = area.min_y; y <= area.max_y; y += 1) {
    for (let x = area.min_x; x <= area.max_x; x += 1) {
      assert.equal(worldMap.encounters[y][x], -1,
        `unpacked encounters[${y}][${x}] stays the -1 sentinel`);
    }
  }
});
