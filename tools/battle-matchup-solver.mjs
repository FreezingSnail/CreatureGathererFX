#!/usr/bin/env node
// Matchup web solver for the 8-type duel.
//
// The matrix is a directed graph of "beats" edges (usually several overlapping
// 3-cycles plus singles); undecided pairs stay neutral. The solver's job:
//   1. derive the x/y chart from an edge list,
//   2. prove that for every ordered pair of creature move-sets (A, B) there is
//      at least one positive matchup: some a in A beats some b in B,
//   3. propose the smallest set of extra edges that fixes violations, preferring
//      edges that close new 3-cycles (keeping the web triangle-heavy), and
//   4. report pairs that no edge can fix (identical singleton sets).
//
// Run: node tools/battle-matchup-solver.mjs [--fix]
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const repo = path.dirname(fileURLToPath(import.meta.url)) + '/..';

export function chartFromEdges(order, edges) {
  const index = new Map(order.map((type, position) => [type, position]));
  const chart = order.map(() => order.map(() => 1));
  for (const [from, to] of edges) {
    const row = index.get(from), col = index.get(to);
    if (row === undefined || col === undefined) throw Error(`unknown edge ${from}>${to}`);
    if (row === col) throw Error(`self edge ${from}>${to}`);
    if (chart[row][col] !== 1 || chart[col][row] !== 1) throw Error(`conflicting edge ${from}>${to}`);
    chart[row][col] = 2;
    chart[col][row] = 0.5;
  }
  return chart;
}

export function edgeList(order, chart) {
  const edges = [];
  for (let row = 0; row < order.length; row++) {
    for (let col = 0; col < order.length; col++) {
      if (chart[row][col] === 2) edges.push([order[row], order[col]]);
    }
  }
  return edges;
}

const beats = (chart, order, a, b) => {
  const row = order.indexOf(a), col = order.indexOf(b);
  return row >= 0 && col >= 0 && chart[row][col] === 2;
};

// 3-cycles in the directed graph: a beats b beats c beats a.
export function triangles(order, chart) {
  const found = [];
  for (let a = 0; a < order.length; a++) {
    for (let b = 0; b < order.length; b++) {
      for (let c = 0; c < order.length; c++) {
        if (a === b || b === c || a === c) continue;
        if (chart[a][b] === 2 && chart[b][c] === 2 && chart[c][a] === 2 && a < b && a < c) {
          found.push([order[a], order[b], order[c]]);
        }
      }
    }
  }
  return found;
}

// Demands: what the web must satisfy for the roster (deduplicated by type set).
//  - two distinct sets need one decisive edge in either direction, so every
//    matchup has at least one positive option ("always a positive matchup
//    between sets");
//  - identical sets need an internal edge (a > b, both in the set) for the
//    same reason;
//  - identical singleton sets are mirrors no matrix can fix (data issue).
export function uniqueSets(sets) {
  const byKey = new Map();
  for (const set of sets) {
    const key = [...set.types].sort().join('/');
    if (!byKey.has(key)) byKey.set(key, { key, types: [...set.types].sort(), name: set.name, count: 0 });
    byKey.get(key).count += 1;
  }
  return [...byKey.values()];
}

// Sets carried by more than one creature produce mirror matchups; a matrix can
// only help those with an internal edge (multi-type) or not at all (singleton).
export function mirrorSets(sets) {
  return uniqueSets(sets).filter((set) => set.count > 1 && set.types.length === 1);
}

export function demands(sets) {
  const unique = uniqueSets(sets);
  const list = [];
  for (let i = 0; i < unique.length; i++) {
    for (let j = i + 1; j < unique.length; j++) {
      list.push({ kind: 'either', a: unique[i], b: unique[j] });
    }
  }
  // Worse-keyed first so reports read deterministically.
  for (const set of unique) {
    if (set.count > 1 && set.types.length > 1) list.push({ kind: 'internal', a: set, b: set });
  }
  return list;
}

export function satisfied(chart, order, demand) {
  const wins = (from, to) => from.types.some((typeA) => to.types.some((typeB) => typeA !== typeB && beats(chart, order, typeA, typeB)));
  return demand.kind === 'internal'
    ? wins(demand.a, demand.a)
    : wins(demand.a, demand.b) || wins(demand.b, demand.a);
}

export function unsatisfiedDemands(order, chart, sets) {
  return demands(sets).filter((demand) => !satisfied(chart, order, demand));
}

// Randomized-restart greedy: add the neutral edge that satisfies the most
// outstanding demands, preferring edges that close 3-cycles, with random
// tie-breaks across restarts so a blocked branch is not fatal.
export function solveWeb(order, baseEdges, sets, options = {}) {
  const attempts = options.attempts || 200;
  const rng = options.rng || (() => Math.random());
  const preference = options.preference || [];
  const preferIndex = (from, to) => {
    const at = preference.indexOf(`${from}>${to}`);
    return at === -1 ? preference.length : at;
  };
  let best = null;
  for (let attempt = 0; attempt < attempts; attempt++) {
    const edges = baseEdges.map((edge) => edge.slice());
    const chart = chartFromEdges(order, edges);
    let stuck = false;
    for (let guard = 0; guard < 64; guard++) {
      const outstanding = unsatisfiedDemands(order, chart, sets);
      if (!outstanding.length) break;
      let bestEdge = null;
      for (const [from, to] of pairs(order)) {
        const row = order.indexOf(from), col = order.indexOf(to);
        if (chart[row][col] !== 1 || chart[col][row] !== 1) continue;
        chart[row][col] = 2;
        chart[col][row] = 0.5;
        const remaining = unsatisfiedDemands(order, chart, sets);
        const gain = outstanding.length - remaining.length;
        chart[row][col] = 1;
        chart[col][row] = 1;
        if (gain <= 0) continue;
        const closes = order.some((third) => third !== from && third !== to
          && beats(chart, order, to, third) && beats(chart, order, third, from));
        const score = [gain, closes ? 1 : 0, -preferIndex(from, to), rng()];
        if (!bestEdge || compare(score, bestEdge.score) > 0) bestEdge = { from, to, score };
      }
      if (!bestEdge) { stuck = true; break; }
      chart[order.indexOf(bestEdge.from)][order.indexOf(bestEdge.to)] = 2;
      chart[order.indexOf(bestEdge.to)][order.indexOf(bestEdge.from)] = 0.5;
      edges.push([bestEdge.from, bestEdge.to]);
    }
    const outstanding = unsatisfiedDemands(order, chart, sets).length;
    const result = { edges, added: edges.slice(baseEdges.length), chart, outstanding, stuck };
    if (!best || result.outstanding < best.outstanding
      || (result.outstanding === best.outstanding && result.added.length < best.added.length)) {
      best = result;
    }
    if (best.outstanding === 0 && attempt > 20) break;
  }
  return best;
}

function pairs(order) {
  const result = [];
  for (const from of order) for (const to of order) if (from !== to) result.push([from, to]);
  return result;
}
function compare(a, b) {
  for (let i = 0; i < a.length; i++) if (a[i] !== b[i]) return a[i] - b[i];
  return 0;
}

// Stats for reporting/tests.
export function stats(order, chart, sets) {
  const decisive = edgeList(order, chart).length;
  const total = order.length * (order.length - 1);
  const allDemands = demands(sets);
  return {
    edges: decisive,
    decisiveFraction: decisive / total,
    neutralFraction: 1 - decisive / total,
    triangles: triangles(order, chart),
    demands: allDemands.length,
    unsatisfied: unsatisfiedDemands(order, chart, sets).length,
  };
}
// Kits are the authored 8-type `moves` name list (CreatureGathererFX-4w89.6);
// the packed move table is gone.
export function rosterSets(creatureList, order) {
  return creatureList.map((creature) => ({
    name: creature.name,
    types: [...new Set(creature.moves
      .filter((type) => !order || order.includes(type)))],
  }));
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const { WHEEL } = await import('./battle-wheel.mjs');
  const data = JSON.parse(fs.readFileSync(path.join(repo, 'mock/data.js'), 'utf8')
    .replace(/^[\s\S]*?const DATA = /, '').replace(/;\n[\s\S]*$/, ''));
  const sets = rosterSets(data.creatures, WHEEL.order);
  const chart = chartFromEdges(WHEEL.order, WHEEL.edges);
  const report = stats(WHEEL.order, chart, sets);
  console.log(`edges ${report.edges}, decisive ${(report.decisiveFraction * 100).toFixed(0)}%, `
    + `triangles ${report.triangles.length}, demands ${report.demands}, unsatisfied ${report.unsatisfied}`);
  const missing = unsatisfiedDemands(WHEEL.order, chart, sets);
  for (const demand of missing.slice(0, 10)) {
    console.log(`  ${demand.kind === 'either' ? 'either way' : 'missing win'}: `
      + `${demand.a.name} [${demand.a.types}] vs ${demand.b.name} [${demand.b.types}]`);
  }
  if (process.argv.includes('--solve')) {
    const solution = solveWeb(WHEEL.order, WHEEL.edges, sets);
    console.log(`solve: added ${solution.added.length} edges, outstanding ${solution.outstanding}, `
      + `triangles ${triangles(WHEEL.order, solution.chart).length}`);
    console.log(`edges: ${solution.edges.map(([a, b]) => `['${a}', '${b}']`).join(', ')}`);
  }
}
