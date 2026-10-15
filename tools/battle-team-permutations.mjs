#!/usr/bin/env node
// 4-type team permutation census: every 3-creature team composition from the
// four folded types, every matchup between them, and the roster-level team
// counts that can fill each composition.
//
// Run: node tools/battle-team-permutations.mjs
// Test: node --test tools/tests/battle-team-permutations_test.mjs
import path from 'node:path';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';
import { FOLDS } from './battle-matrices-spike.mjs';

const repo = path.dirname(fileURLToPath(import.meta.url)) + '/..';
export const TEAM_SIZE = 3;

function load() {
  const require = createRequire(import.meta.url);
  const matrices = require(path.join(repo, 'mock/matrices.js'));
  const data = require(path.join(repo, 'mock/data.js'));
  const table = matrices.matrices.find((matrix) => matrix.id === '4');
  return { table, data, fold: FOLDS[4] };
}

// All multisets (unordered compositions) of `slots` picks from `types`.
export function typeCombos(types, slots) {
  const combos = [];
  const walk = (start, picked) => {
    if (picked.length === slots) { combos.push([...picked]); return; }
    for (let index = start; index < types.length; index++) {
      picked.push(types[index]);
      walk(index, picked);
      picked.pop();
    }
  };
  walk(0, []);
  return combos;
}

// All ordered sequences (permutations with repetition) of `slots` from `types`.
export function typeSequences(types, slots) {
  if (slots === 0) return [[]];
  const sequences = [];
  for (const prefix of typeSequences(types, slots - 1)) {
    for (const type of types) sequences.push([...prefix, type]);
  }
  return sequences;
}

const beats = (table, a, b) => {
  const row = table.order.indexOf(a), col = table.order.indexOf(b);
  return row >= 0 && col >= 0 && table.chart[row][col] === 2;
};

// Does team A hold a positive matchup against team B?
export function hasWin(table, teamA, teamB) {
  return teamA.some((a) => teamB.some((b) => beats(table, a, b)));
}

export function classify(table, comboA, comboB) {
  const aWins = hasWin(table, comboA, comboB);
  const bWins = hasWin(table, comboB, comboA);
  if (aWins && bWins) return 'mutual';
  if (aWins) return 'A';
  if (bWins) return 'B';
  return 'dead';
}

const key = (combo) => combo.join('+');
const sameMultiset = (a, b) => key([...a].sort()) === key([...b].sort());

// Roster-level team counts per composition: binomial over primary folded types.
export function rosterCounts(combos, creatures, fold) {
  const pool = {};
  for (const creature of creatures) {
    const folded = fold.map[creature.types[0]] || creature.types[0];
    pool[folded] = (pool[folded] || 0) + 1;
  }
  const choose = (n, k) => {
    if (k > n) return 0;
    let value = 1;
    for (let i = 0; i < k; i++) value = (value * (n - i)) / (i + 1);
    return Math.round(value);
  };
  const counts = new Map();
  for (const combo of combos) {
    const tally = {};
    for (const type of combo) tally[type] = (tally[type] || 0) + 1;
    let teams = 1;
    for (const [type, count] of Object.entries(tally)) teams *= choose(pool[type] || 0, count);
    counts.set(key(combo), teams);
  }
  return { counts, pool };
}

export function census() {
  const { table, data, fold } = load();
  const combos = typeCombos(table.order, TEAM_SIZE);
  const sequences = typeSequences(table.order, TEAM_SIZE);
  const { counts, pool } = rosterCounts(combos, data.creatures, fold);
  const pairs = [];
  for (let i = 0; i < combos.length; i++) {
    for (let j = i; j < combos.length; j++) {
      pairs.push({ a: combos[i], b: combos[j], kind: classify(table, combos[i], combos[j]) });
    }
  }
  const records = combos.map((combo) => {
    const vm = { advantages: 0, mutual: 0, disadvantages: 0, dead: 0, selfDead: false };
    for (const pair of pairs) {
      if (sameMultiset(combo, pair.a) && sameMultiset(combo, pair.b)) {
        vm.selfDead = pair.kind === 'dead';
        continue;
      }
      const isA = sameMultiset(combo, pair.a);
      const isB = sameMultiset(combo, pair.b);
      if (!isA && !isB) continue;
      const other = isA ? pair.b : pair.a;
      if (sameMultiset(combo, other)) { vm.selfDead = pair.kind === 'dead'; continue; }
      const mine = isA ? pair.kind === 'A' : pair.kind === 'B';
      const theirs = isA ? pair.kind === 'B' : pair.kind === 'A';
      if (pair.kind === 'mutual') vm.mutual += 1;
      else if (mine) vm.advantages += 1;
      else if (theirs) vm.disadvantages += 1;
      else vm.dead += 1;
    }
    return { combo, teams: counts.get(key(combo)), ...vm };
  });
  return { table, combos, sequences, pairs, records, pool };
}

export function report() {
  const { table, combos, sequences, pairs, records, pool } = census();
  const lines = [];
  lines.push(`4-type table: ${table.order.join(' > beats > ')} (${table.edges.length} edges)`);
  lines.push(`roster pools (primary folded type): ${JSON.stringify(pool)}`);
  lines.push(`3-creature compositions: ${combos.length} multisets, ${sequences.length} ordered sequences, `
    + `${typeSequences(table.order, 3).length} sequences / ${combos.length} multisets, `
    + `${combos.length * (combos.length + 1) / 2} unordered matchup pairs`);
  const distinct = sequences.filter((sequence) => new Set(sequence).size === 3).length;
  lines.push(`distinct-type permutations P(4,3): ${distinct}`);
  lines.push('');
  lines.push('composition                 roster teams  adv  mut  dis  dead  selfDead');
  for (const record of records) {
    lines.push(`${key(record.combo).padEnd(28)}${String(record.teams).padStart(12)}`
      + `${String(record.advantages).padStart(5)}${String(record.mutual).padStart(5)}`
      + `${String(record.disadvantages).padStart(5)}${String(record.dead).padStart(6)}`
      + `${record.selfDead ? '      yes' : '       no'}`);
  }
  lines.push('');
  const ranked = [...records].sort((a, b) => (b.advantages - b.disadvantages) - (a.advantages - a.disadvantages));
  lines.push(`most dominant compositions: ${ranked.slice(0, 5).map((r) => key(r.combo)).join(', ')}`);
  lines.push(`most punished compositions: ${ranked.slice(-5).map((r) => key(r.combo)).reverse().join(', ')}`);
  const deadMirrors = records.filter((record) => record.selfDead).map((record) => key(record.combo));
  lines.push(`self-dead mirror compositions (no internal edge): ${deadMirrors.join(', ') || 'none'}`);
  const counts = { mutual: 0, A: 0, B: 0, dead: 0 };
  for (const pair of pairs) counts[pair.kind] += 1;
  lines.push(`unordered pair classes: mutual ${counts.mutual}, one-sided ${counts.A + counts.B}, dead ${counts.dead}`);
  return lines.join('\n');
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  console.log(report());
}
