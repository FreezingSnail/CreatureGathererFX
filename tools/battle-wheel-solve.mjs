#!/usr/bin/env node
// Re-derive the 8-type matchup web for the current roster and print the edge
// list for tools/battle-wheel.mjs. Run: node tools/battle-wheel-solve.mjs
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { WHEEL } from './battle-wheel.mjs';
import {
  chartFromEdges, solveWeb, rosterSets, stats, unsatisfiedDemands, triangles,
} from './battle-matchup-solver.mjs';

const repo = path.dirname(fileURLToPath(import.meta.url)) + '/..';
const data = JSON.parse(fs.readFileSync(path.join(repo, 'mock/data.js'), 'utf8')
  .replace(/^[\s\S]*?const DATA = /, '').replace(/;\n[\s\S]*$/, ''));
const sets = rosterSets(data.creatures, data.moves, WHEEL.order);

// Preference: when several edges fix the same number of demands, prefer the
// ones with a readable causal story.
const preference = [
  'wind>spirit', 'plant>lightning', 'earth>spirit', 'fire>spirit',
  'water>wind', 'lightning>fire', 'lightning>wind', 'earth>plant',
  'wind>plant', 'water>elder', 'elder>earth', 'spirit>water', 'spirit>lightning',
];
let seed = 12345;
const rng = () => { seed = (seed * 1103515245 + 12345) & 0x7fffffff; return seed / 0x7fffffff; };
const solution = solveWeb(WHEEL.order, WHEEL.edges, sets, { attempts: 400, rng, preference });
const report = stats(WHEEL.order, solution.chart, sets);
console.log(`edges ${solution.edges.length} (base ${WHEEL.edges.length}, added ${solution.added.length}), `
  + `outstanding ${solution.outstanding}, stuck ${solution.stuck}`);
console.log(`triangles ${triangles(WHEEL.order, solution.chart).length}, `
  + `decisive ${(report.decisiveFraction * 100).toFixed(0)}%`);
for (const demand of unsatisfiedDemands(WHEEL.order, solution.chart, sets)) {
  console.log(`UNSATISFIED ${demand.kind}: ${demand.a.types} vs ${demand.b.types}`);
}
const added = solution.edges.slice(WHEEL.edges.length);
console.log('added edges:');
for (const [from, to] of added) console.log(`    ['${from}', '${to}'],`);
