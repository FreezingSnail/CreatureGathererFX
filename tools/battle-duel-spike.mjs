#!/usr/bin/env node
// Read-rate spike for the duel AI: batch duels across personas, read rates and
// player policies on a chosen mixed matchup. Answers "does the read rate feel
// fair, and where should persona defaults sit?".
//
// Run: node tools/battle-duel-spike.mjs [--battles 200] [--rounds 40]
import path from 'node:path';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';

const repo = path.dirname(fileURLToPath(import.meta.url)) + '/..';

export function loadMock() {
  const require = createRequire(import.meta.url);
  return require(path.join(repo, 'mock/battle-mock.js'));
}

// Balanced 1v1 matchup (used with partySize 1 to isolate the read model):
// player ScrambleSnail [wind, plant] vs AI ScatterCrab [earth, lightning].
export const SPIKE_MATCHUP = { you: 'ScrambleSnail', foe: 'ScatterCrab' };

function playerPick(mock, state, policy, rng) {
  const options = mock.duelOptions(state, 0);
  if (options.length <= 1 || policy === 'random') return rng.int(options.length);
  const lastAi = state.duelState.last[1];
  if (policy === 'swap') {
    const lastAiType = state.duelState.last[1];
    if (lastAiType && !options.some((option) => mock.wheelBeats(option.type, lastAiType))) {
      const bench = mock.duelBench(state, 0);
      const benchMoves = bench ? bench.moves.filter((id) => id != null).map((id) => mock.DATA.moves[id])
        .filter((move) => move.type !== 'status' && move.type !== 'none') : [];
      if (benchMoves.some((move) => mock.wheelBeats(move.type, lastAiType))) return -1;
    }
    return rng.int(options.length);
  }
  if (policy === 'counter' && lastAi) {
    const counters = options
      .map((option, index) => ({ option, index }))
      .filter((entry) => mock.wheelBeats(entry.option.type, lastAi));
    if (counters.length) return counters[rng.int(counters.length)].index;
  }
  if (policy === 'anti-repeat' && state.duelState.history.length >= 2) {
    const lastMine = state.duelState.history[state.duelState.history.length - 1];
    const alternatives = options
      .map((option, index) => ({ option, index }))
      .filter((entry) => entry.option.type !== lastMine);
    if (alternatives.length) return alternatives[rng.int(alternatives.length)].index;
  }
  return rng.int(options.length);
}

export function playDuel({ persona, readRate, policy, seed, matchup = SPIKE_MATCHUP, maxRounds = 40 }) {
  const mock = loadMock();
  const index = (name) => mock.DATA.creatures.findIndex((entry) => entry.name === name);
  const state = mock.newMock({
    mode: 'duel',
    scoring: 'bonus',
    you: index(matchup.you),
    foe: index(matchup.foe),
    seed,
    aiRead: readRate,
    aiPersona: persona,
    matrixId: '8', // read-rate baseline: mutual-counter matchups (the 5-type default can fold into one-sided pairs)
    teamSize: 1, // isolate the read model from team pairings
  });
  mock.startBattle(state);
  // Pin the configured matchup: teamSize 1, one specific creature per side.
  const youIdx = index(matchup.you), foeIdx = index(matchup.foe);
  state.duelState.partyIndex = [[youIdx], [foeIdx]];
  state.duelState.hp = [[mock.creature(youIdx).maxHp], [mock.creature(foeIdx).maxHp]];
  state.duelState.activeSlot = [0, 0];
  let rngState = (seed * 2654435761) >>> 0 || 1;
  const rng = { int: (n) => { rngState ^= rngState << 13; rngState >>>= 0; rngState ^= rngState >>> 17; rngState ^= rngState << 5; rngState >>>= 0; return rngState % n; } };
  let rounds = 0;
  while (state.scene === 'battle' && rounds < maxRounds) {
    rounds += 1;
    mock.duelPick(state, playerPick(mock, state, policy, rng));
    mock.duelUpdate(state, 1000); // resolve immediately, skip reveal/result timers
    mock.duelUpdate(state, 2000);
  }
  const outcome = state.scene === 'end' ? state.endText : 'TIMEOUT';
  return { outcome, rounds };
}

export function runMatrix({ battles = 200, persona, readRate, policy, matchup, maxRounds }) {
  let wins = 0, losses = 0, draws = 0, timeouts = 0, rounds = 0;
  for (let seed = 1; seed <= battles; seed++) {
    const { outcome, rounds: fought } = playDuel({ persona, readRate, policy, seed, matchup, maxRounds });
    rounds += fought;
    if (outcome === 'WIN') wins += 1;
    else if (outcome === 'LOSE') losses += 1;
    else if (outcome === 'DRAW') draws += 1;
    else timeouts += 1;
  }
  return { wins, losses, draws, timeouts, battles, avgRounds: rounds / battles };
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const arg = (name, fallback) => {
    const at = process.argv.indexOf(`--${name}`);
    return at === -1 ? fallback : Number(process.argv[at + 1]);
  };
  const battles = arg('battles', 200);
  const rates = [0, 25, 50, 75, 100];
  const personas = ['reader', 'frequency', 'anti-repeat', 'exploiter'];
  const policies = ['random', 'counter', 'anti-repeat', 'swap'];
  console.log(`matchup ${SPIKE_MATCHUP.you} (player) vs ${SPIKE_MATCHUP.foe} (AI), ${battles} duels/cell`);
  for (const persona of personas) {
    for (const policy of policies) {
      const cells = rates.map((readRate) => {
        const result = runMatrix({ battles, persona, readRate, policy });
        const winPct = (100 * result.wins / battles).toFixed(0);
        return `${readRate}%:${winPct.padStart(3)}%`;
      });
      console.log(`  ${persona.padEnd(11)} vs ${policy.padEnd(11)} ${cells.join(' ')}`);
    }
  }
  console.log('(player win %; TIMEOUT counts as non-win; bonus spelling)');
}
