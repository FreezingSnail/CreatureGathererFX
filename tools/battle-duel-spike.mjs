#!/usr/bin/env node
// Duel balance spike for the 5-type RPS duel (CreatureGathererFX-d73i.8).
//
// Batch duels across personas, player policies and read rates on the default
// 5-type table with teams of three. Reports wins/losses/draws/timeouts
// separately plus round-time and behaviour metrics (median/p95 rounds, switch
// frequency, one-hit KOs, type and species skew) for final balance work.
//
// Policies and the AI see only revealed history; a policy never reads the
// opponent's current hidden pick.
//
// Run: node tools/battle-duel-spike.mjs [--battles 1000] [--rounds 200]
//      node tools/battle-duel-spike.mjs --persona reader --policy counter --read 45
import path from 'node:path';
import { createHash } from 'node:crypto';
import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

const repo = path.dirname(fileURLToPath(import.meta.url)) + '/..';

export function loadMock() {
  const require = createRequire(import.meta.url);
  return require(path.join(repo, 'mock/battle-mock.js'));
}

// ---- provenance helpers (pinned in every --json artifact) ------------------
// Deterministic hashing: canonical (key-sorted) JSON so a fixture hash does not
// depend on object insertion order. No timestamps are ever emitted, so the same
// scenario + seed produces byte-identical JSON.
export function canonicalJson(value) {
  if (value === null || typeof value !== 'object') return JSON.stringify(value);
  if (Array.isArray(value)) return `[${value.map(canonicalJson).join(',')}]`;
  const keys = Object.keys(value).sort();
  return `{${keys.map((key) => `${JSON.stringify(key)}:${canonicalJson(value[key])}`).join(',')}}`;
}

export function sha256Hex(text) {
  return createHash('sha256').update(text).digest('hex');
}

// Files whose bytes identify the reporting seam.
export const PROVENANCE_FILES = [
  'tools/battle-duel-spike.mjs',
  'mock/battle-mock.js',
  'src/engine/battle/DuelKernel.hpp',
];

function fileSha(rel) {
  try {
    return sha256Hex(readFileSync(path.join(repo, rel)));
  } catch {
    return null;
  }
}

function gitRevision() {
  try {
    return execFileSync('git', ['rev-parse', 'HEAD'], { cwd: repo, encoding: 'utf8' }).trim();
  } catch {
    return null;
  }
}

function gitDirty() {
  try {
    return execFileSync('git', ['status', '--porcelain'], { cwd: repo, encoding: 'utf8' }).trim().length > 0;
  } catch {
    return null;
  }
}

// SHA-256 of the authored duel fixtures the run depends on: creature records,
// the active matrix table, and the sample teams.
export function fixtureHashes() {
  const mock = loadMock();
  const matrix5 = (mock.MATRIX_DATA && mock.MATRIX_DATA.matrices
    ? mock.MATRIX_DATA.matrices.find((matrix) => matrix.id === DEFAULT_MATRIX_ID)
    : null) ?? null;
  return {
    creatures: sha256Hex(canonicalJson(mock.DATA.creatures)),
    matrix5: sha256Hex(canonicalJson(matrix5)),
    teams: sha256Hex(canonicalJson(mock.TEAM_DATA.teams)),
  };
}

// Provenance block pinned into the --json artifact: repository revision, the
// seed keys used, and SHA-256 of the seam files and duel fixtures.
export function buildProvenance(opts = {}) {
  const battles = opts.battles ?? 200;
  const files = {};
  for (const rel of PROVENANCE_FILES) files[rel] = fileSha(rel);
  return {
    gitRevision: gitRevision(),
    gitDirty: gitDirty(),
    nodeVersion: process.version,
    seedKeys: { scheme: 'seed = 1..battles per cell (identical across cells)', battles },
    files,
    fixtures: fixtureHashes(),
  };
}

// Balanced head-to-head pairing used by the singleton read-rate isolate:
// player ScrambleSnail [wind, plant] vs AI ScatterCrab [earth, lightning].
export const SPIKE_MATCHUP = { you: 'ScrambleSnail', foe: 'ScatterCrab' };

export const DEFAULT_MATRIX_ID = '5';
export const DEFAULT_TEAM_SIZE = 3;
export const DEFAULT_SCORING = 'bonus';
export const DEFAULT_MAX_ROUNDS = 200;

// Version of the machine-readable `--json` artifact. Bump on any breaking
// schema change (renamed/retyped/removed field). Additive fields keep the
// version; see docs/battle-simulator.md for the documented schema.
export const DUEL_SPIKE_SCHEMA_VERSION = 1;

export const PERSONAS = ['reader', 'frequency', 'anti-repeat', 'exploiter', 'random'];
// Balance contract policies: random / repeat / counter / rotation / swap / tier,
// plus the retained anti-repeat face. `tier` mirrors the AI's non-read branch
// (best creature-type x1.5/x0.5 matchup tier), so the paired baseline reflects
// the shipped up/down creature-type marker a player can read. Added by ogh.18.
export const POLICIES = ['random', 'repeat', 'counter', 'rotation', 'swap', 'tier', 'anti-repeat'];
export const READ_RATES = [0, 25, 50, 75, 100];

// Folded-matrix relation (2 beats, 0.5 loses, 1 trades) for the active table.
// This replaces the old 8-type wheel lookups so policies follow the same
// 5-type scoring the duel actually uses.
function beats(mock, state, attackType, defendType) {
  return attackType != null && defendType != null &&
    mock.duelValue(state, attackType, defendType) === 2;
}

function xorshift32(value) {
  let s = value >>> 0;
  s ^= s << 13; s >>>= 0;
  s ^= s >>> 17;
  s ^= s << 5; s >>>= 0;
  return s >>> 0;
}

function makePlayerRng(seed) {
  let s = ((seed * 2654435761) >>> 0) || 1;
  return {
    int(n) {
      s = xorshift32(s);
      return n ? Math.floor((s / 4294967296) * n) : 0;
    },
  };
}

// Player policies. Every policy chooses only from the side's own owned option
// slots (mock.duelOptions(state, 0)) and reads only REVEALED history: the AI's
// last resolved type (state.duelState.last[1]) and the player's own revealed
// picks (state.duelState.history). It never inspects the hidden current pick.
//
//   random     - uniform over owned options
//   repeat     - deterministic: re-pick the owned option matching the last
//                revealed own type (first match), else option 0
//   counter    - beat the AI's last revealed type with an owned option
//   rotation   - deterministic: next owned option after the last revealed own
//                type, else option 0
//   swap       - switch to a bench creature that owns a counter, else attack
//   tier       - mirror the AI's non-read branch: pick an owned option at the
//                best creature-type x1.5/x0.5 matchup tier (random tie-break)
//   anti-repeat- avoid repeating the last revealed own type (retained)
export function playerPick(mock, state, policy, rng) {
  const options = mock.duelOptions(state, 0);
  if (options.length === 0) return mock.WAIT_PICK; // unusable kit -> WAIT
  const history = state.duelState.history || [];
  const lastMine = history.length ? history[history.length - 1] : null;
  const indexOfType = (type) => options.findIndex((option) => option.type === type);

  if (policy === 'repeat') {
    const index = lastMine != null ? indexOfType(lastMine) : -1;
    return index >= 0 ? index : 0;
  }
  if (policy === 'rotation') {
    const index = lastMine != null ? indexOfType(lastMine) : -1;
    return index >= 0 ? (index + 1) % options.length : 0;
  }

  // tier: the same rule aiDuelPick uses on a no-read round - score every owned
  // option by the creature-type matchup modifier against the foe's active types
  // and pick uniformly among the best tier. It reads only the opponent's
  // already-active creature types (never the hidden current pick). Added ogh.18.
  if (policy === 'tier') {
    const scored = options.map((option, index) => ({
      index, modifier: mock.duelCreatureBonus(state, 0, option.type),
    }));
    const bestTier = Math.max(...scored.map((entry) => entry.modifier));
    const pool = scored.filter((entry) => entry.modifier === bestTier);
    return pool[rng.int(pool.length)].index;
  }

  if (options.length <= 1 || policy === 'random') return rng.int(options.length);
  const lastAi = state.duelState.last[1];
  if (policy === 'swap') {
    if (lastAi && !options.some((option) => beats(mock, state, option.type, lastAi))) {
      const benchSlot = mock.livingBenchSlots(state, 0)[0];
      const benchOptions = benchSlot == null ? [] : mock.duelOptionsFor(state, 0, benchSlot);
      if (benchOptions.some((option) => beats(mock, state, option.type, lastAi))) return -1;
    }
    return rng.int(options.length);
  }
  if (policy === 'counter' && lastAi) {
    const counters = options
      .map((option, index) => ({ option, index }))
      .filter((entry) => beats(mock, state, entry.option.type, lastAi));
    if (counters.length) return counters[rng.int(counters.length)].index;
  }
  if (policy === 'anti-repeat' && lastMine != null) {
    const alternatives = options
      .map((option, index) => ({ option, index }))
      .filter((entry) => entry.option.type !== lastMine);
    if (alternatives.length) return alternatives[rng.int(alternatives.length)].index;
  }
  return rng.int(options.length);
}

// Build a duel: default 5-type table, 3v3, bonus scoring. `youPreset` /
// `foePreset` select authored sample/retyped teams by id (mock presetTeam), so
// .4 can sweep sample/beta pairs by calling runMatrix / runPairSweep with ids,
// without touching the reference model. `matchup` pins a singleton head-to-head
// isolate (used only by the read-rate spike).
export function buildDuel(opts = {}) {
  const mock = loadMock();
  const persona = opts.persona ?? 'reader';
  const readRate = opts.readRate ?? 45;
  const seed = opts.seed ?? 1;
  const matrixId = opts.matrixId ?? DEFAULT_MATRIX_ID;
  const teamSize = opts.teamSize ?? DEFAULT_TEAM_SIZE;
  const scoring = opts.scoring ?? DEFAULT_SCORING;
  const matchup = opts.matchup ?? null;
  const index = (name) => mock.DATA.creatures.findIndex((entry) => entry.name === name);
  const state = mock.newMock({
    mode: 'duel',
    scoring,
    you: matchup ? index(matchup.you) : 0,
    foe: matchup ? index(matchup.foe) : 0,
    seed,
    aiRead: readRate,
    aiPersona: persona,
    matrixId,
    teamSize,
    presetYou: opts.youPreset ?? null,
    presetFoe: opts.foePreset ?? null,
  });
  mock.startBattle(state);
  if (matchup) {
    const youIdx = index(matchup.you), foeIdx = index(matchup.foe);
    state.duelState.partyIndex = [[youIdx], [foeIdx]];
    state.duelState.partyTypes = null;
    state.duelState.hp = [[mock.creature(youIdx).maxHp], [mock.creature(foeIdx).maxHp]];
    state.duelState.activeSlot = [0, 0];
  }
  return { mock, state, rng: makePlayerRng(seed), policy: opts.policy ?? 'random' };
}

export function playDuel(opts = {}) {
  const maxRounds = opts.maxRounds ?? DEFAULT_MAX_ROUNDS;
  const { mock, state, rng, policy } = buildDuel(opts);
  const maxHpOf = (side, slot) => mock.creature(state.duelState.partyIndex[side][slot]).maxHp;
  const species = state.duelState.partyIndex[0].map((i) => mock.creature(i).name);
  const metrics = { rounds: 0, switches: 0, kos: 0, oneHitKOs: 0, typeUse: {} };
  while (state.scene === 'battle' && metrics.rounds < maxRounds) {
    metrics.rounds += 1;
    const preHp = [state.duelState.hp[0].slice(), state.duelState.hp[1].slice()];
    const pick = playerPick(mock, state, policy, rng);
    if (pick === -1 || pick >= mock.SWITCH_BASE) metrics.switches += 1;
    mock.duelPick(state, pick);
    mock.duelUpdate(state, 1000);
    mock.duelUpdate(state, 2000);
    for (let side = 0; side < 2; side += 1) {
      for (let slot = 0; slot < state.duelState.hp[side].length; slot += 1) {
        if (preHp[side][slot] > 0 && state.duelState.hp[side][slot] === 0) {
          metrics.kos += 1;
          if (preHp[side][slot] === maxHpOf(side, slot)) metrics.oneHitKOs += 1;
        }
      }
    }
    const revealed = state.duelState.last ? state.duelState.last[0] : null;
    if (revealed) {
      const matrix = mock.duelMatrix(state);
      const name = mock.foldType(matrix, revealed);
      metrics.typeUse[name] = (metrics.typeUse[name] || 0) + 1;
    }
  }
  const outcome = state.scene === 'end' ? state.endText : 'TIMEOUT';
  return {
    outcome,
    rounds: metrics.rounds,
    switches: metrics.switches,
    kos: metrics.kos,
    oneHitKOs: metrics.oneHitKOs,
    typeUse: metrics.typeUse,
    species,
  };
}

function nearestRank(sorted, q) {
  if (!sorted.length) return 0;
  const index = Math.min(sorted.length - 1, Math.ceil(q * sorted.length) - 1);
  return sorted[Math.max(0, index)];
}

export function runMatrix(opts = {}) {
  const battles = opts.battles ?? 200;
  const maxRounds = opts.maxRounds ?? DEFAULT_MAX_ROUNDS;
  const roundList = [];
  const agg = {
    wins: 0, losses: 0, draws: 0, timeouts: 0, rounds: 0, switches: 0,
    kos: 0, oneHitKOs: 0, typeUse: {}, speciesUse: {},
  };
  for (let seed = 1; seed <= battles; seed += 1) {
    const duel = playDuel({ ...opts, seed, maxRounds });
    roundList.push(duel.rounds);
    agg.rounds += duel.rounds;
    agg.switches += duel.switches;
    agg.kos += duel.kos;
    agg.oneHitKOs += duel.oneHitKOs;
    if (duel.outcome === 'WIN') agg.wins += 1;
    else if (duel.outcome === 'LOSE') agg.losses += 1;
    else if (duel.outcome === 'DRAW') agg.draws += 1;
    else agg.timeouts += 1;
    for (const [type, count] of Object.entries(duel.typeUse)) {
      agg.typeUse[type] = (agg.typeUse[type] || 0) + count;
    }
    for (const name of duel.species) agg.speciesUse[name] = (agg.speciesUse[name] || 0) + 1;
  }
  const sorted = roundList.slice().sort((a, b) => a - b);
  const attacks = Object.values(agg.typeUse).reduce((sum, n) => sum + n, 0);
  const speciesSamples = Object.values(agg.speciesUse).reduce((sum, n) => sum + n, 0);
  return {
    battles,
    wins: agg.wins, losses: agg.losses, draws: agg.draws, timeouts: agg.timeouts,
    rounds: agg.rounds,
    avgRounds: agg.rounds / battles,
    medianRounds: nearestRank(sorted, 0.5),
    p95Rounds: nearestRank(sorted, 0.95),
    switches: agg.switches,
    switchRate: agg.rounds ? agg.switches / agg.rounds : 0,
    kos: agg.kos,
    oneHitKOs: agg.oneHitKOs,
    attacks,
    typeUse: agg.typeUse,
    speciesSamples,
    speciesUse: agg.speciesUse,
    matrixId: opts.matrixId ?? DEFAULT_MATRIX_ID,
    teamSize: opts.teamSize ?? DEFAULT_TEAM_SIZE,
    scoring: opts.scoring ?? DEFAULT_SCORING,
    youPreset: opts.youPreset ?? null,
    foePreset: opts.foePreset ?? null,
    maxRounds,
  };
}

// A per-cell machine-readable row: the runMatrix aggregate over seed keys
// 1..battles, plus the cell's selecting options. Shared by the pair-sweep and
// grid JSON emitters so a "cell" always carries the same metric fields.
export function cellFromResult(result, extra = {}) {
  return {
    ...extra,
    battles: result.battles,
    wins: result.wins,
    losses: result.losses,
    draws: result.draws,
    timeouts: result.timeouts,
    rounds: result.rounds,
    avgRounds: result.avgRounds,
    medianRounds: result.medianRounds,
    p95Rounds: result.p95Rounds,
    switches: result.switches,
    switchRate: result.switchRate,
    kos: result.kos,
    oneHitKOs: result.oneHitKOs,
    attacks: result.attacks,
    typeUse: result.typeUse,
    speciesSamples: result.speciesSamples,
    speciesUse: result.speciesUse,
  };
}

// All authored sample-team ordered pairs; .4 can extend with its beta teams by
// passing `pairs: [{ you, foe }, ...]` to runPairSweep.
export const TEAM_PAIRS = (() => {
  const mock = loadMock();
  const ids = (mock.TEAM_DATA && mock.TEAM_DATA.teams ? mock.TEAM_DATA.teams : []).map((t) => t.id);
  const pairs = [];
  for (const you of ids) for (const foe of ids) pairs.push({ you, foe });
  return pairs;
})();

// Structured seam for team-pair sweeps: runs one runMatrix per pair, returning
// the same result object per pair (with youPreset/foePreset recorded).
export function runPairSweep(opts = {}) {
  const pairs = opts.pairs ?? TEAM_PAIRS;
  return pairs.map(({ you, foe }) => ({
    you,
    foe,
    result: runMatrix({ ...opts, youPreset: you, foePreset: foe }),
  }));
}

// Full balanced-pair sweep (CreatureGathererFX-d73i.4): every authored sample
// team pair across the required player policies and read rates, aggregated to a
// compact per-pair summary plus review flags. Flags mark any timeout anywhere in
// the pair and any pair whose player win rate exceeds 80% in EVERY cell (a
// trivial sweep). Beta authored encounter pairings reuse the same seam by
// passing `pairs` built from the beta team presets.
export const PAIR_SWEEP_WIN_FLAG = 0.8;

export function runPairSweepReport(opts = {}) {
  const battles = opts.battles ?? 200;
  const maxRounds = opts.maxRounds ?? DEFAULT_MAX_ROUNDS;
  const persona = opts.persona ?? 'reader';
  const policies = opts.policies ?? POLICIES;
  const rates = opts.rates ?? READ_RATES;
  const pairs = opts.pairs ?? TEAM_PAIRS;
  const rows = [];
  for (const { you, foe } of pairs) {
    let wins = 0, losses = 0, draws = 0, timeouts = 0, total = 0;
    let allCellsSweep = true;
    let anyTimeout = false;
    const cells = [];
    for (const policy of policies) {
      for (const readRate of rates) {
        const r = runMatrix({ battles, maxRounds, persona, policy, readRate,
          youPreset: you, foePreset: foe });
        wins += r.wins; losses += r.losses; draws += r.draws; timeouts += r.timeouts;
        total += r.battles;
        if (r.timeouts > 0) anyTimeout = true;
        if (r.wins / battles <= PAIR_SWEEP_WIN_FLAG) allCellsSweep = false;
        cells.push(cellFromResult(r, {
          policy,
          rate: readRate,
          seed: { from: 1, to: battles, count: battles },
        }));
      }
    }
    rows.push({
      you, foe, wins, losses, draws, timeouts, total,
      playerWinRate: total ? wins / total : 0,
      flags: { timeouts: anyTimeout, sweep: allCellsSweep },
      cells,
    });
  }
  const flagged = rows.filter((r) => r.flags.timeouts || r.flags.sweep);
  return { battles, maxRounds, persona, policies, rates, rows, flagged };
}

export function formatPairSweepReport(sweep) {
  const lines = [];
  lines.push(`duel pair sweep · matrix5 team3 bonus · ${sweep.rows.length} pairs · ` +
    `${sweep.battles} duels/cell · policies ${sweep.policies.join('/')} · ` +
    `rates ${sweep.rates.join('/')} · cap ${sweep.maxRounds}`);
  for (const row of sweep.rows) {
    const flags = `${row.flags.timeouts ? ' TIMEOUT' : ''}${row.flags.sweep ? ' SWEEP>80%' : ''}`;
    lines.push(`  ${row.you} vs ${row.foe}`.padEnd(34) +
      ` W ${String(row.wins).padStart(5)} L ${String(row.losses).padStart(5)} ` +
      `D ${String(row.draws).padStart(4)} T ${String(row.timeouts).padStart(4)} ` +
      `P ${(100 * row.playerWinRate).toFixed(0).padStart(3)}%${flags}`);
  }
  const t = sweep.flagged.filter((r) => r.flags.timeouts).length;
  const s = sweep.flagged.filter((r) => r.flags.sweep).length;
  lines.push(`  flags: ${t} pair(s) with timeouts, ${s} pair(s) sweeping >80% in every cell`);
  return lines.join('\n');
}

// ---- machine-readable artifact (`--json`) ----------------------------------
// One versioned object: schemaVersion + mode + config + pinned provenance +
// rows. Pair-sweep nests per-cell rows under each pair; grid emits a flat cell
// list. Always serialized with JSON.stringify(value, null, 2).
function configBlock(overrides = {}) {
  return {
    matrixId: DEFAULT_MATRIX_ID,
    teamSize: DEFAULT_TEAM_SIZE,
    scoring: DEFAULT_SCORING,
    ...overrides,
  };
}

export function buildPairSweepJson(sweep) {
  return {
    schemaVersion: DUEL_SPIKE_SCHEMA_VERSION,
    mode: 'pair-sweep',
    config: configBlock({
      maxRounds: sweep.maxRounds,
      battles: sweep.battles,
      persona: sweep.persona,
      policies: sweep.policies,
      rates: sweep.rates,
    }),
    provenance: buildProvenance({ battles: sweep.battles }),
    pairs: sweep.rows,
    flagged: sweep.flagged,
  };
}

// Build the JSON artifact for any CLI invocation. `--pair-sweep` emits the
// paired sweep; otherwise a flat grid over personas x policies x read rates
// (mirrors the text runReport defaults).
export function buildJsonReport(args = {}) {
  if (args['pair-sweep']) {
    return buildPairSweepJson(runPairSweepReport({
      battles: args.battles ?? 200,
      maxRounds: args.rounds ?? DEFAULT_MAX_ROUNDS,
      persona: args.persona ?? 'reader',
      policies: args.policy !== undefined ? [args.policy] : undefined,
      rates: args.read !== undefined ? [args.read] : undefined,
    }));
  }
  const battles = args.battles ?? 200;
  const maxRounds = args.rounds ?? DEFAULT_MAX_ROUNDS;
  const matrixId = args.matrix !== undefined ? String(args.matrix) : DEFAULT_MATRIX_ID;
  const teamSize = args.team ?? DEFAULT_TEAM_SIZE;
  const scoring = args.scoring ?? DEFAULT_SCORING;
  const youPreset = args['you-preset'] ?? null;
  const foePreset = args['foe-preset'] ?? null;
  const base = { battles, maxRounds, matrixId, teamSize, scoring, youPreset, foePreset };
  const personas = args.persona !== undefined ? [args.persona]
    : ['reader', 'frequency', 'anti-repeat', 'exploiter'];
  const policies = args.policy !== undefined ? [args.policy] : POLICIES;
  const rates = args.read !== undefined ? [args.read] : READ_RATES;
  const seed = { from: 1, to: battles, count: battles };
  const cells = [];
  for (const persona of personas) {
    for (const policy of policies) {
      for (const readRate of rates) {
        const result = runMatrix({ ...base, persona, policy, readRate });
        cells.push(cellFromResult(result, { persona, policy, rate: readRate, seed }));
      }
    }
  }
  return {
    schemaVersion: DUEL_SPIKE_SCHEMA_VERSION,
    mode: 'grid',
    config: configBlock({
      matrixId, teamSize, scoring, maxRounds, battles,
      personas, policies, rates, youPreset, foePreset,
    }),
    provenance: buildProvenance({ battles }),
    cells,
  };
}

export function formatJsonReport(report) {
  return JSON.stringify(report, null, 2);
}

export function parseArgs(argv) {
  const args = { _: [] };
  for (let i = 0; i < argv.length; i += 1) {
    const token = argv[i];
    if (!token.startsWith('--')) { args._.push(token); continue; }
    const name = token.slice(2);
    const next = argv[i + 1];
    if (next === undefined || next.startsWith('--')) { args[name] = true; continue; }
    args[name] = /^-?\d+(\.\d+)?$/.test(next) ? Number(next) : next;
    i += 1;
  }
  return args;
}

function configLabel(result) {
  return `matrix${result.matrixId} team${result.teamSize} ${result.scoring} cap${result.maxRounds}`;
}

function metricsLine(label, result) {
  const skew = Object.entries(result.typeUse)
    .sort((a, b) => b[1] - a[1])
    .map(([type, count]) => `${type}:${count}`)
    .join(' ');
  return `  ${label.padEnd(22)} W ${String(result.wins).padStart(4)} ` +
    `L ${String(result.losses).padStart(4)} D ${String(result.draws).padStart(4)} ` +
    `T ${String(result.timeouts).padStart(4)} | med ${String(result.medianRounds).padStart(3)} ` +
    `p95 ${String(result.p95Rounds).padStart(3)} sw ${(100 * result.switchRate).toFixed(1)}% ` +
    `ko ${String(result.kos).padStart(4)} ohk ${String(result.oneHitKOs).padStart(4)} ` +
    `| types ${skew} (n=${result.attacks})`;
}

export function runReport(args) {
  const battles = args.battles ?? 200;
  const maxRounds = args.rounds ?? DEFAULT_MAX_ROUNDS;
  const matrixId = args.matrix !== undefined ? String(args.matrix) : DEFAULT_MATRIX_ID;
  const teamSize = args.team ?? DEFAULT_TEAM_SIZE;
  const scoring = args.scoring ?? DEFAULT_SCORING;
  const youPreset = args['you-preset'] ?? null;
  const foePreset = args['foe-preset'] ?? null;
  const base = { battles, maxRounds, matrixId, teamSize, scoring, youPreset, foePreset };
  const presetNote = (youPreset || foePreset) ? ` · teams ${youPreset ?? 'random'} vs ${foePreset ?? 'random'}` : '';

  const lines = [];
  if (args.persona !== undefined && args.policy !== undefined) {
    const readRate = args.read ?? 45;
    const result = runMatrix({ ...base, persona: args.persona, policy: args.policy, readRate });
    lines.push(`duel spike · ${configLabel(result)}${presetNote} · ${battles} duels · cap ${maxRounds}`);
    lines.push(metricsLine(`${args.persona}/${args.policy}@${readRate}%`, result));
    const species = Object.entries(result.speciesUse).sort((a, b) => b[1] - a[1])
      .slice(0, 8).map(([name, count]) => `${name}:${count}`).join(' ');
    lines.push(`  species ${species} (n=${result.speciesSamples})`);
    lines.push(footer());
    return lines.join('\n');
  }

  const personas = args.persona !== undefined ? [args.persona] : ['reader', 'frequency', 'anti-repeat', 'exploiter'];
  const policies = args.policy !== undefined ? [args.policy] : POLICIES;
  const rates = args.read !== undefined ? [args.read] : READ_RATES;
  const reference = args.read !== undefined ? args.read : 50;

  lines.push(`duel spike · matrix${matrixId} team${teamSize} ${scoring}${presetNote} · ` +
    `${battles} duels/cell · round cap ${maxRounds}`);
  lines.push('');
  lines.push('player win % by AI read rate (TIMEOUT = non-win):');
  const metricResults = [];
  for (const persona of personas) {
    for (const policy of policies) {
      const cells = [];
      let referenceResult = null;
      for (const readRate of rates) {
        const result = runMatrix({ ...base, persona, policy, readRate });
        cells.push(`${String(readRate).padStart(3)}%:${String(Math.round(100 * result.wins / battles)).padStart(3)}%`);
        if (readRate === reference) referenceResult = result;
      }
      lines.push(`  ${persona.padEnd(11)} vs ${policy.padEnd(11)} ${cells.join(' ')}`);
      if (referenceResult) metricResults.push([`${persona}/${policy}@${reference}%`, referenceResult]);
    }
  }
  lines.push('');
  lines.push(`metrics at ${reference}% read (outcomes split):`);
  for (const [label, result] of metricResults) lines.push(metricsLine(label, result));
  lines.push(footer());
  return lines.join('\n');
}

// Denominator legend so .4 reads the compact metrics unambiguously.
function footer() {
  return '  (ohk = full-HP -> KO in one hit; sw = player switches / rounds; ' +
    'type n = committed player attacks; species n = party samples over battles; ' +
    'ko = total faints)';
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const args = parseArgs(process.argv.slice(2));
  if (args.json) {
    process.stdout.write(`${formatJsonReport(buildJsonReport(args))}\n`);
  } else if (args['pair-sweep']) {
    process.stdout.write(`${formatPairSweepReport(runPairSweepReport({
      battles: args.battles ?? 200,
      maxRounds: args.rounds ?? DEFAULT_MAX_ROUNDS,
      persona: args.persona ?? 'reader',
      policies: args.policy !== undefined ? [args.policy] : undefined,
      rates: args.read !== undefined ? [args.read] : undefined,
    }))}\n`);
  } else {
    process.stdout.write(`${runReport(args)}\n`);
  }
}
