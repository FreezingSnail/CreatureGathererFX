// Cross-language parity: the actual C++ duel kernel vs the canonical mock.
//
// The driver (tools/duel-parity/duel_kernel_driver.cpp) includes the real
// src/engine/battle/DuelKernel.hpp; this test feeds it identical states and
// compares relations, damage, HP, active/original slots and outcomes against
// mock/battle-mock.js (5-type matrix, bonus scoring). Comparisons are STRICT:
// TRADE rounds with asymmetric creature modifiers and dual-typed defenders are
// included, so any kernel divergence fails here rather than being quarantined.
//
// Run: node --test tools/tests/duel-kernel-parity_test.mjs
import test from 'node:test';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const repo = path.resolve(here, '..', '..');
const require = createRequire(import.meta.url);
const mock = require(path.join(repo, 'mock', 'battle-mock.js'));

const BUILD_DIR = path.join(repo, 'build', 'duel-sim');
const DRIVER_SRC = path.join(repo, 'tools', 'duel-parity', 'duel_kernel_driver.cpp');
const DRIVER_BIN = path.join(BUILD_DIR, 'duel_kernel_driver');

export const NO_SLOT = 255;
export const SWITCH_BASE = mock.SWITCH_BASE;

// Always rebuild the small host driver so a modified DuelKernel.hpp (or its
// BattleRng.hpp include) is picked up — never trust a stale binary.
fs.mkdirSync(BUILD_DIR, { recursive: true });
execFileSync('c++', ['-std=c++17', '-w', '-O0', '-I', repo, DRIVER_SRC, '-o', DRIVER_BIN], {
  cwd: repo, stdio: 'inherit',
});

// Seed a duel state with explicit parties, optional per-slot type overrides and
// explicit HP so damaged/KO/full states are expressible.
function makeState({
  youParty, foeParty, youTypes = null, foeTypes = null,
  youHp, foeHp, youActive = 0, foeActive = 0, seed = 1,
}) {
  const state = mock.newMock({ mode: 'duel', scoring: 'bonus', matrixId: '5', teamSize: 3, seed });
  mock.startBattle(state);
  state.duel.scoring = 'bonus';
  state.duel.matrixId = '5';
  state.duelState.partyIndex = [youParty.slice(), foeParty.slice()];
  state.duelState.partyTypes = (youTypes || foeTypes) ? [youTypes, foeTypes] : null;
  state.duelState.hp = [
    (youHp ?? maxHpOf(youParty)).slice(),
    (foeHp ?? maxHpOf(foeParty)).slice(),
  ];
  state.duelState.activeSlot = [youActive, foeActive];
  state.duelState.history = [];
  state.duelState.picks = [null, null];
  state.duelState.last = [null, null];
  return state;
}

const maxHpOf = (party) => party.map((idx) => mock.creature(idx).maxHp);

function foldedTypes(state, side, slot) {
  const matrix = mock.duelMatrix(state);
  const overrides = state.duelState.partyTypes ? state.duelState.partyTypes[side] : null;
  const override = overrides ? overrides[slot] : null;
  if (override) return [matrix.order.indexOf(override)];
  const raw = mock.creature(state.duelState.partyIndex[side][slot]).types;
  return [...new Set(raw.map((type) => matrix.order.indexOf(mock.foldType(matrix, type))))]
    .sort((a, b) => a - b);
}

function foldedOptions(state, side, slot) {
  const matrix = mock.duelMatrix(state);
  const creature = mock.creature(state.duelState.partyIndex[side][slot]);
  return creature.moves
    .filter((id) => id != null)
    .map((id) => mock.DATA.moves[id])
    .filter((move) => mock.isSpikeMove(move))
    .map((move) => matrix.order.indexOf(mock.foldType(matrix, move.type)));
}

function stateCommands(state) {
  const lines = [`STATE ${state.duelState.partyIndex[0].length} ` +
    `${state.duelState.partyIndex[1].length} ${state.duelState.activeSlot[0]} ` +
    `${state.duelState.activeSlot[1]} 0`];
  for (let side = 0; side < 2; side += 1) {
    for (let slot = 0; slot < state.duelState.partyIndex[side].length; slot += 1) {
      const id = state.duelState.partyIndex[side][slot];
      const types = foldedTypes(state, side, slot);
      const options = foldedOptions(state, side, slot);
      const maxHp = mock.creature(id).maxHp;
      const hp = state.duelState.hp[side][slot];
      lines.push(`MEM ${side} ${slot} ${id} ${types.length} ${types[0] ?? NO_SLOT} ` +
        `${types[1] ?? 0} ${hp} ${maxHp} ${options.length} ${options.join(' ')}`.trimEnd());
    }
  }
  return lines;
}

// --- driver plumbing -------------------------------------------------------

function runCommands(commands) {
  const output = execFileSync(DRIVER_BIN, {
    input: `${commands.join('\n')}\n`, encoding: 'utf8', maxBuffer: 64 * 1024 * 1024,
  });
  return output.trimEnd().length ? output.trimEnd().split('\n') : [];
}

function parseRes(line) {
  const token = line.trim().split(/\s+/);
  assert.equal(token[0], 'RES', `expected RES, got: ${line}`);
  const res = {
    rel: +token[1], d0: +token[2], d1: +token[3], hb0: +token[4], hb1: +token[5],
    aid0: +token[6], aid1: +token[7], prev0: +token[8], prev1: +token[9],
    k0: +token[10], k1: +token[11], outcome: +token[12],
  };
  let i = 13;
  const c0 = +token[i]; i += 1;
  res.hp0 = token.slice(i, i + c0).map(Number); i += c0;
  const c1 = +token[i]; i += 1;
  res.hp1 = token.slice(i, i + c1).map(Number); i += c1;
  res.active0 = +token[i]; res.active1 = +token[i + 1]; res.histCount = +token[i + 2];
  return res;
}

function parsePick(line) {
  const token = line.trim().split(/\s+/);
  assert.equal(token[0], 'PICK', `expected PICK, got: ${line}`);
  return { kind: +token[1], index: +token[2] };
}

// --- expected values from the canonical mock -------------------------------

function resolvingSlots(state, picks) {
  const pre = state.duelState.activeSlot;
  return [0, 1].map((side) =>
    mock.isSwitchPick(picks[side]) ? mock.switchTargetSlot(state, side, picks[side]) : pre[side]);
}

function relationIds(state, picks, slots) {
  if (mock.isSwitchPick(picks[0]) || mock.isSwitchPick(picks[1]) || picks[0] < 0 || picks[1] < 0) {
    return NO_SLOT;
  }
  const chart = mock.duelMatrix(state).chart;
  const m0 = foldedOptions(state, 0, slots[0])[picks[0]];
  const m1 = foldedOptions(state, 1, slots[1])[picks[1]];
  if (m0 == null || m1 == null) return NO_SLOT;
  return chart[m0][m1] === 2 ? 2 : chart[m0][m1] === 0.5 ? 0 : 1;
}

// Fields after the mock has resolved the round, given the pre-round snapshot.
function fieldsAfterResolve(state, picks, preHp, preActive, slots) {
  const duelState = state.duelState;
  const postHp = [duelState.hp[0].slice(), duelState.hp[1].slice()];
  const hpBefore = [0, 1].map((side) => preHp[side][slots[side]]);
  const damage = [0, 1].map((side) => hpBefore[side] - postHp[side][slots[side]]);
  const prev = [0, 1].map((side) => (mock.isSwitchPick(picks[side]) ? preActive[side] : NO_SLOT));
  const activeId = [0, 1].map((side) => duelState.partyIndex[side][slots[side]]);
  const alive0 = mock.sideAlive(state, 0);
  const alive1 = mock.sideAlive(state, 1);
  const outcome = (!alive0 && !alive1) ? 3 : (!alive1) ? 1 : (!alive0) ? 2 : 0;
  return {
    rel: relationIds(state, picks, slots), hpBefore, damage, prev, activeId, outcome,
    postHp, active: [duelState.activeSlot[0], duelState.activeSlot[1]],
  };
}

function jsPickToAction(state, side, pick) {
  if (pick >= SWITCH_BASE) return { kind: 1, index: pick - SWITCH_BASE };
  if (pick === -1) return { kind: 1, index: mock.switchTargetSlot(state, side, pick) };
  return { kind: 0, index: pick };
}

function assertRoundMatches(res, expected, label) {
  assert.equal(res.rel, expected.rel, `${label}: relation`);
  assert.equal(res.k0, expected.kind0, `${label}: player action kind`);
  assert.equal(res.k1, expected.kind1, `${label}: foe action kind`);
  assert.equal(Math.min(res.d0, res.hb0), expected.damage[0], `${label}: player damage`);
  assert.equal(Math.min(res.d1, res.hb1), expected.damage[1], `${label}: foe damage`);
  assert.equal(res.hb0, expected.hpBefore[0], `${label}: player hpBefore`);
  assert.equal(res.hb1, expected.hpBefore[1], `${label}: foe hpBefore`);
  assert.equal(res.aid0, expected.activeId[0], `${label}: player resolving id`);
  assert.equal(res.aid1, expected.activeId[1], `${label}: foe resolving id`);
  assert.equal(res.prev0, expected.prev[0], `${label}: player original slot`);
  assert.equal(res.prev1, expected.prev[1], `${label}: foe original slot`);
  assert.equal(res.outcome, expected.outcome, `${label}: outcome`);
  assert.deepEqual(res.hp0, expected.postHp[0], `${label}: player hp`);
  assert.deepEqual(res.hp1, expected.postHp[1], `${label}: foe hp`);
  assert.equal(res.active0, expected.active[0], `${label}: player active slot`);
  assert.equal(res.active1, expected.active[1], `${label}: foe active slot`);
}

// --- round scenarios -------------------------------------------------------

const SQ = 6;        // water [1], one water move
const CRAB = 3;      // earth [2], earth moves
const SCRAMBLE = 2;  // water/plant [1,4], four options
const CACTUS = 26;   // earth/plant [2,4]
const COAL = 48;     // earth/fire [2,3], fire option 0
const SPORELING = 37;// spirit/plant [0,4], plant option 0
const DRAGON = 29;   // spirit/plant [0,4], option 1 is water

function attack(index) { return { kind: 0, index }; }
function swap(slot) { return { kind: 1, index: slot }; }
function toPicks(actions) {
  return actions.map((action) => (action.kind === 1 ? SWITCH_BASE + action.index : action.index));
}

const ROUND_CASES = [
  { name: 'ordinary-win', youParty: [SQ], foeParty: [CRAB], actions: [attack(0), attack(0)] },
  { name: 'ordinary-trade', youParty: [SQ], foeParty: [SQ], actions: [attack(0), attack(0)] },
  { name: 'trade-asymmetric', youParty: [DRAGON], foeParty: [SQ], actions: [attack(1), attack(0)] },
  { name: 'ordinary-lose', youParty: [CRAB], foeParty: [SQ], actions: [attack(0), attack(0)] },
  { name: 'dual-bonus-win', youParty: [COAL], foeParty: [CACTUS], actions: [attack(0), attack(0)] },
  { name: 'dual-half-win', youParty: [SPORELING], foeParty: [CACTUS], actions: [attack(0), attack(1)] },
  { name: 'custom-option-index', youParty: [SCRAMBLE], foeParty: [SQ], actions: [attack(2), attack(0)] },
  { name: 'retype-override', youParty: [SCRAMBLE], foeParty: [SCRAMBLE],
    foeTypes: ['plant', 'plant', 'plant'], youTypes: null, actions: [attack(0), attack(0)] },
  { name: 'retype-bonus', youParty: [DRAGON], foeParty: [CRAB],
    youTypes: ['plant', 'plant', 'plant'], foeTypes: null, actions: [attack(1), attack(0)] },
  { name: 'switch-default-win', youParty: [SQ], foeParty: [CRAB, CRAB], actions: [attack(0), swap(1)] },
  { name: 'both-switch', youParty: [SQ, SQ], foeParty: [SQ, SQ], actions: [swap(1), swap(1)] },
  { name: 'ko-revert', youParty: [SQ], foeParty: [CRAB, CRAB],
    foeHp: [mock.creature(CRAB).maxHp, 10], actions: [attack(0), swap(1)] },
  { name: 'promotion', youParty: [SQ], foeParty: [CRAB, CRAB],
    foeHp: [10, mock.creature(CRAB).maxHp], actions: [attack(0), attack(0)] },
  { name: 'draw', youParty: [SQ], foeParty: [CRAB], youHp: [2], foeHp: [21],
    actions: [attack(0), attack(0)] },
];

test('duel round resolution matches the mock kernel (fixed committed picks)', () => {
  const commands = [];
  const expectations = [];
  for (const scenario of ROUND_CASES) {
    const state = makeState({
      youParty: scenario.youParty, foeParty: scenario.foeParty,
      youTypes: scenario.youTypes ?? null, foeTypes: scenario.foeTypes ?? null,
      youHp: scenario.youHp, foeHp: scenario.foeHp,
    });
    const picks = toPicks(scenario.actions);
    const preHp = [state.duelState.hp[0].slice(), state.duelState.hp[1].slice()];
    const preActive = [state.duelState.activeSlot[0], state.duelState.activeSlot[1]];
    const slots = resolvingSlots(state, picks);
    const setupLines = stateCommands(state);
    state.duelState.picks = picks.slice();
    mock.duelResolve(state);
    const expected = fieldsAfterResolve(state, picks, preHp, preActive, slots);
    const [a0, a1] = scenario.actions;
    expected.kind0 = a0.kind;
    expected.kind1 = a1.kind;

    for (const line of setupLines) { commands.push(line); expectations.push('OK'); }
    commands.push(`ACT ${a0.kind} ${a0.index} ${a1.kind} ${a1.index} -1`);
    expectations.push({ type: 'RES', expected, label: scenario.name });
  }
  commands.push('DONE');
  expectations.push('BYE');

  const output = runCommands(commands);
  assert.equal(output.length, expectations.length, 'driver line count');
  for (let i = 0; i < output.length; i += 1) {
    const expect = expectations[i];
    if (expect === 'OK' || expect === 'BYE') {
      assert.equal(output[i].trim(), expect, `line ${i}: ${output[i]}`);
      continue;
    }
    assertRoundMatches(parseRes(output[i]), expect.expected, expect.label);
  }
});

// --- deterministic RNG stream ----------------------------------------------

test('xorshift32 int-floor stream matches the mock makeRng', () => {
  const bounds = [2, 3, 4, 5, 7, 8, 100, 255];
  const seeds = [1, 42, 12345, 999983, 0xdeadbeef];
  const commands = [];
  const expectedRolls = [];
  for (const seed of seeds) {
    commands.push(`ROLL ${seed} ${bounds.length} ${bounds.join(' ')}`);
    const rng = mock.makeRng(seed);
    expectedRolls.push(bounds.map((bound) => rng.int(bound)));
  }
  commands.push('DONE');
  const output = runCommands(commands);
  const rollLines = output.filter((line) => line.startsWith('ROLLS'));
  assert.equal(rollLines.length, seeds.length);
  rollLines.forEach((line, index) => {
    const values = line.trim().split(/\s+/).slice(1).map(Number);
    assert.deepEqual(values, expectedRolls[index], `seed ${seeds[index]}`);
  });
});

test('persona prediction matches on non-tied histories', () => {
  const matrix = mock.MATRIX_DATA.matrices.find((m) => m.id === '5');
  const cases = [
    { persona: 1, raw: ['fire', 'water', 'plant'] },
    { persona: 1, raw: ['spirit'] },
    { persona: 2, raw: ['water', 'water', 'fire'] },
    { persona: 3, raw: ['water', 'water', 'fire'] },
    { persona: 2, raw: ['earth', 'earth', 'earth', 'plant'] },
    { persona: 3, raw: ['plant', 'plant', 'plant', 'spirit'] },
  ];
  const personaName = { 1: 'reader', 2: 'frequency', 3: 'anti-repeat' };
  const commands = [];
  const expected = [];
  for (const entry of cases) {
    const ids = entry.raw.map((name) => matrix.order.indexOf(name));
    commands.push(`PREDICT ${entry.persona} 7 ${ids.length} ${ids.join(' ')}`);
    const state = mock.newMock({ mode: 'duel', scoring: 'bonus', matrixId: '5', aiPersona: personaName[entry.persona] });
    state.duelState = { history: entry.raw.slice() };
    state.rng = mock.makeRng(7);
    expected.push(mock.predictPlayerType(state));
  }
  commands.push('DONE');
  const output = runCommands(commands);
  const predLines = output.filter((line) => line.startsWith('PRED'));
  assert.equal(predLines.length, cases.length);
  predLines.forEach((line, index) => {
    assert.equal(Number(line.trim().split(/\s+/)[1]), matrix.order.indexOf(expected[index]), `case ${index}`);
  });
});

// --- AI pick parity (fresh seeded stream) -----------------------------------

test('AI committed pick matches the mock on fixed states', () => {
  const matrix = mock.MATRIX_DATA.matrices.find((m) => m.id === '5');
  const setups = [
    { persona: 1, readRate: 100, raw: [] },
    { persona: 1, readRate: 100, raw: ['fire'] },
    { persona: 1, readRate: 0, raw: ['water', 'plant'] },
    { persona: 0, readRate: 100, raw: ['earth'] },
    { persona: 2, readRate: 100, raw: ['water', 'water', 'fire'] },
    { persona: 3, readRate: 100, raw: ['earth', 'earth', 'spirit', 'spirit'] },
  ];
  const seed = 2026;
  const commands = [];
  const expectedPicks = [];
  for (const setup of setups) {
    const state = makeState({ youParty: [SCRAMBLE], foeParty: [COAL] });
    state.duel.aiPersona = { 0: 'random', 1: 'reader', 2: 'frequency', 3: 'anti-repeat' }[setup.persona];
    state.duel.aiRead = setup.readRate;
    state.duelState.history = setup.raw.slice();
    state.duelState.readFiredPrev = false;
    state.duelState.postReadCounts = {};
    state.rng = mock.makeRng(seed);
    mock.duelPick(state, 0);
    expectedPicks.push(jsPickToAction(state, 1, state.duelState.picks[1]));
    for (const line of stateCommands(state)) { commands.push(line); }
    const ids = setup.raw.map((name) => matrix.order.indexOf(name));
    if (ids.length) commands.push(`HIST ${ids.length} ${ids.join(' ')}`);
    commands.push(`AIFRESH ${seed} ${setup.persona} ${setup.readRate}`);
  }
  commands.push('DONE');
  const output = runCommands(commands);
  const pickLines = output.filter((line) => line.startsWith('PICK'));
  assert.equal(pickLines.length, setups.length);
  pickLines.forEach((line, index) => {
    const pick = parsePick(line);
    assert.equal(pick.kind, expectedPicks[index].kind, `setup ${index} kind`);
    assert.equal(pick.index, expectedPicks[index].index, `setup ${index} index`);
  });
});

// --- randomized single-round parity (broad resolution coverage) -------------

function rngFrom(seed) {
  let s = seed >>> 0 || 1;
  return () => {
    s ^= s << 13; s >>>= 0; s ^= s >>> 17; s ^= s << 5; s >>>= 0;
    return s / 4294967296;
  };
}

test('randomized single rounds match the mock (trades and dual types included)', () => {
  const rng = rngFrom(0xc0ffee);
  const next = (n) => Math.floor(rng() * n);
  const creatureCount = mock.DATA.creatures.length;
  const commands = [];
  const expectations = [];
  let compared = 0;
  let tradeRounds = 0;

  for (let t = 0; t < 600; t += 1) {
    const rollParty = () => {
      const party = [];
      while (party.length < 3) {
        const idx = next(creatureCount);
        if (!party.includes(idx)) party.push(idx);
      }
      return party;
    };
    const youParty = rollParty();
    const foeParty = rollParty();
    const youHp = youParty.map((i) => 1 + next(mock.creature(i).maxHp));
    const foeHp = foeParty.map((i) => 1 + next(mock.creature(i).maxHp));
    const state = makeState({
      youParty, foeParty, youHp, foeHp, youActive: next(3), foeActive: next(3),
    });

    const choose = (side) => {
      const options = mock.duelOptions(state, side);
      const bench = mock.livingBenchSlots(state, side);
      const total = options.length + (bench.length ? 1 : 0);
      if (total === 0) return null;
      const roll = next(total);
      if (roll < options.length) return { kind: 0, index: roll };
      return { kind: 1, index: bench[next(bench.length)] };
    };
    const a0 = choose(0);
    const a1 = choose(1);
    if (!a0 || !a1) continue;

    const picks = toPicks([a0, a1]);
    const preHp = [state.duelState.hp[0].slice(), state.duelState.hp[1].slice()];
    const preActive = [state.duelState.activeSlot[0], state.duelState.activeSlot[1]];
    const slots = resolvingSlots(state, picks);
    const setupLines = stateCommands(state);
    state.duelState.picks = picks.slice();
    mock.duelResolve(state);
    const expected = fieldsAfterResolve(state, picks, preHp, preActive, slots);
    expected.kind0 = a0.kind;
    expected.kind1 = a1.kind;
    if (expected.rel === 1) tradeRounds += 1;

    for (const line of setupLines) { commands.push(line); expectations.push('OK'); }
    commands.push(`ACT ${a0.kind} ${a0.index} ${a1.kind} ${a1.index} -1`);
    expectations.push({ type: 'RES', expected, label: `round#${t}` });
    compared += 1;
  }

  commands.push('DONE');
  expectations.push('BYE');
  const output = runCommands(commands);
  assert.equal(output.length, expectations.length, 'driver line count');
  for (let i = 0; i < output.length; i += 1) {
    const expect = expectations[i];
    if (expect === 'OK' || expect === 'BYE') {
      assert.equal(output[i].trim(), expect, `line ${i}`);
      continue;
    }
    assertRoundMatches(parseRes(output[i]), expect.expected, expect.label);
  }
  assert.ok(compared > 400, `compared ${compared} rounds`);
  assert.ok(tradeRounds > 0, `expected trade rounds in the randomized set, saw ${tradeRounds}`);
});

// --- seeded full-duel parity (reader/random personas) -----------------------

function playerRng(seed) {
  let s = ((seed * 2654435761) >>> 0) || 1;
  return {
    int(n) {
      s ^= s << 13; s >>>= 0; s ^= s >>> 17; s ^= s << 5; s >>>= 0;
      return n ? Math.floor((s / 4294967296) * n) : 0;
    },
  };
}

function simulateDuel({ youPreset, foePreset, persona, readRate, seed, maxRounds = 90 }) {
  const state = mock.newMock({
    mode: 'duel', scoring: 'bonus', matrixId: '5', teamSize: 3, seed,
    presetYou: youPreset, presetFoe: foePreset, aiRead: readRate, aiPersona: persona,
  });
  mock.startBattle(state);
  const initialCommands = stateCommands(state);
  const rng = playerRng(seed);
  const rounds = [];
  let count = 0;
  while (state.scene === 'battle' && count < maxRounds) {
    count += 1;
    const preHp = [state.duelState.hp[0].slice(), state.duelState.hp[1].slice()];
    const preActive = [state.duelState.activeSlot[0], state.duelState.activeSlot[1]];
    const options = mock.duelOptions(state, 0);
    const playerPick = options.length ? rng.int(options.length) : 0;
    mock.duelPick(state, playerPick);
    const picks = state.duelState.picks.slice();
    const slots = resolvingSlots(state, picks);
    mock.duelUpdate(state, 1000);
    const expected = fieldsAfterResolve(state, picks, preHp, preActive, slots);
    const playerAction = jsPickToAction(state, 0, picks[0]);
    const foeAction = jsPickToAction(state, 1, picks[1]);
    const record = playerAction.kind === 0 ? foldedOptions(state, 0, slots[0])[picks[0]] : -1;
    rounds.push({ playerAction, foeAction, expected, record });
    mock.duelUpdate(state, 2000);
  }
  rounds.push({ end: state.scene === 'end' ? state.endText : 'TIMEOUT' });
  return { rounds, initialCommands };
}

test('seeded full duels match the mock (committed picks + resolution)', () => {
  const presets = [['balanced', 'anchors'], ['support', 'retyped'], ['retyped', 'trap']];
  const personas = [{ name: 'reader', id: 1, readRate: 65 }, { name: 'random', id: 0, readRate: 100 }];
  const seeds = [1, 7, 42, 1234];

  const commands = [];
  const expectations = [];
  for (const [youPreset, foePreset] of presets) {
    for (const persona of personas) {
      for (const seed of seeds) {
        const { rounds, initialCommands } = simulateDuel({
          youPreset, foePreset, persona: persona.name, readRate: persona.readRate, seed,
        });
        const label = `${youPreset}/${foePreset}/${persona.name}/${seed}`;
        for (const line of initialCommands) { commands.push(line); expectations.push('OK'); }
        commands.push(`SEED ${seed} 0`);
        expectations.push('OK');
        for (const round of rounds) {
          if (round.end !== undefined) break;
          commands.push(`AICONT ${persona.id} ${persona.readRate}`);
          expectations.push({ type: 'PICK', round, label });
          commands.push(`ACT ${round.playerAction.kind} ${round.playerAction.index} ` +
            `${round.foeAction.kind} ${round.foeAction.index} ${round.record}`);
          expectations.push({ type: 'RES', round, label });
        }
      }
    }
  }
  commands.push('DONE');
  expectations.push('BYE');

  const output = runCommands(commands);
  assert.equal(output.length, expectations.length, 'driver line count');
  let comparedRounds = 0;
  let tradeRounds = 0;
  for (let i = 0; i < output.length; i += 1) {
    const expect = expectations[i];
    if (expect === 'OK' || expect === 'BYE') {
      assert.equal(output[i].trim(), expect, `line ${i}`);
      continue;
    }
    if (expect.type === 'PICK') {
      const pick = parsePick(output[i]);
      assert.equal(pick.kind, expect.round.foeAction.kind, `${expect.label}: AI kind`);
      assert.equal(pick.index, expect.round.foeAction.index, `${expect.label}: AI index`);
    } else {
      const expected = expect.round.expected;
      expected.kind0 = expect.round.playerAction.kind;
      expected.kind1 = expect.round.foeAction.kind;
      if (expected.rel === 1) tradeRounds += 1;
      assertRoundMatches(parseRes(output[i]), expected, expect.label);
      comparedRounds += 1;
    }
  }
  assert.ok(comparedRounds > 100, `compared ${comparedRounds} full-duel rounds`);
  assert.ok(tradeRounds > 0, `expected trade rounds in full duels, saw ${tradeRounds}`);
});
