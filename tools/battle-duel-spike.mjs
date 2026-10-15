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
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

const repo = path.dirname(fileURLToPath(import.meta.url)) + '/..';

export function loadMock() {
  const require = createRequire(import.meta.url);
  return require(path.join(repo, 'mock/battle-mock.js'));
}

// Balanced head-to-head pairing used by the singleton read-rate isolate:
// player ScrambleSnail [wind, plant] vs AI ScatterCrab [earth, lightning].
export const SPIKE_MATCHUP = { you: 'ScrambleSnail', foe: 'ScatterCrab' };

export const DEFAULT_MATRIX_ID = '5';
export const DEFAULT_TEAM_SIZE = 3;
export const DEFAULT_SCORING = 'bonus';
export const DEFAULT_MAX_ROUNDS = 200;

export const PERSONAS = ['reader', 'frequency', 'anti-repeat', 'exploiter', 'random'];
// Balance contract policies: random / repeat / counter / rotation / swap, plus
// the retained anti-repeat face.
export const POLICIES = ['random', 'repeat', 'counter', 'rotation', 'swap', 'anti-repeat'];
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
//   anti-repeat- avoid repeating the last revealed own type (retained)
export function playerPick(mock, state, policy, rng) {
  const options = mock.duelOptions(state, 0);
  if (options.length === 0) return 0;
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

  if (options.length <= 1 || policy === 'random') return rng.int(options.length);
  const lastAi = state.duelState.last[1];
  if (policy === 'swap') {
    if (lastAi && !options.some((option) => beats(mock, state, option.type, lastAi))) {
      const bench = mock.duelBench(state, 0);
      const benchMoves = bench
        ? bench.moves.filter((id) => id != null).map((id) => mock.DATA.moves[id])
          .filter((move) => mock.isSpikeMove(move))
        : [];
      if (benchMoves.some((move) => beats(mock, state, move.type, lastAi))) return -1;
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
    if (pick < 0 || pick >= mock.SWITCH_BASE) metrics.switches += 1;
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
  process.stdout.write(`${runReport(parseArgs(process.argv.slice(2)))}\n`);
}
