// Run: node --test tools/tests/battle-matchup-solver_test.mjs
import test from 'node:test';
import assert from 'node:assert/strict';
import {
  chartFromEdges, edgeList, triangles, demands, satisfied, unsatisfiedDemands,
  uniqueSets, solveWeb,
} from '../battle-matchup-solver.mjs';

const ORDER = ['wind', 'earth', 'lightning', 'water'];

test('chartFromEdges builds win/loss/neutral cells and rejects bad edges', () => {
  const chart = chartFromEdges(ORDER, [['wind', 'earth'], ['earth', 'lightning']]);
  assert.equal(chart[0][1], 2);
  assert.equal(chart[1][0], 0.5);
  assert.equal(chart[0][2], 1);
  assert.equal(chart[1][2], 2);
  assert.throws(() => chartFromEdges(ORDER, [['wind', 'wind']]), /self edge/);
  assert.throws(() => chartFromEdges(ORDER, [['wind', 'sea']]), /unknown edge/);
  assert.throws(() => chartFromEdges(ORDER, [['wind', 'earth'], ['earth', 'wind']]), /conflicting/);
});

test('triangles finds every 3-cycle in the graph', () => {
  const cycle = chartFromEdges(ORDER, [['wind', 'earth'], ['earth', 'lightning'], ['lightning', 'wind']]);
  assert.equal(triangles(ORDER, cycle).length, 1);
  const plus = chartFromEdges(ORDER, [
    ['wind', 'earth'], ['earth', 'lightning'], ['lightning', 'wind'],
    ['wind', 'water'], ['water', 'lightning'],
  ]);
  assert.equal(triangles(ORDER, plus).length, 2);
  assert.equal(edgeList(ORDER, plus).length, 5);
});

test('demands: distinct sets need one direction, identical sets an internal edge, mirrors are skipped', () => {
  const sets = [
    { name: 'a', types: ['wind'] },
    { name: 'b', types: ['earth'] },
    { name: 'c', types: ['wind'] }, // identical singleton -> mirror, skipped
    { name: 'd', types: ['wind', 'earth'] }, // identical multi -> internal demand
    { name: 'e', types: ['wind', 'earth'] },
  ];
  assert.equal(uniqueSets(sets).length, 3);
  const list = demands(sets);
  assert.equal(list.filter((demand) => demand.kind === 'either').length, 3); // wind/earth, wind/wind+earth, earth/wind+earth
  assert.equal(list.filter((demand) => demand.kind === 'internal').length, 1);
  const chart = chartFromEdges(ORDER, [['wind', 'earth']]);
  // wind vs earth decided one way; the internal wind>earth edge also satisfies d/e.
  assert.equal(unsatisfiedDemands(ORDER, chart, sets).length, 0);
  for (const demand of list) assert.ok(satisfied(chart, ORDER, demand));
  // Remove the edge: all demands become unsatisfied.
  const empty = chartFromEdges(ORDER, []);
  assert.ok(unsatisfiedDemands(ORDER, empty, sets).length >= 2);
});

test('solveWeb adds the missing edges until every demand is satisfied', () => {
  const sets = [
    { name: 'a', types: ['wind'] },
    { name: 'b', types: ['earth'] },
    { name: 'c', types: ['lightning'] },
  ];
  const solution = solveWeb(ORDER, [], sets, { attempts: 10, rng: () => 0.5 });
  assert.equal(solution.outstanding, 0);
  assert.ok(solution.added.length >= 1);
  for (const demand of demands(sets)) assert.ok(satisfied(solution.chart, ORDER, demand));
});
