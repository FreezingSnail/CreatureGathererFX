// Run: node --test tools/tests/battle-matrices-spike_test.mjs
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import {
  buildMatrices, serializeMatrices, FOLDS, OUTPUT,
} from '../battle-matrices-spike.mjs';
import { chartFromEdges, unsatisfiedDemands, triangles } from '../battle-matchup-solver.mjs';
import { WHEEL } from '../battle-wheel.mjs';

const repo = path.dirname(fileURLToPath(import.meta.url)) + '/../..';

test('committed matrices match a fresh build', () => {
  const committed = fs.readFileSync(path.join(repo, OUTPUT), 'utf8');
  assert.equal(committed, serializeMatrices(buildMatrices()));
});

test('every fold maps all eight authored types and folds get progressively smaller', () => {
  const authored = ['spirit', 'water', 'wind', 'earth', 'fire', 'lightning', 'plant', 'elder'];
  for (const count of [8, 6, 5, 4]) {
    const fold = FOLDS[count];
    assert.equal(fold.order.length, count);
    for (const type of authored) assert.ok(fold.map[type], `${count}: ${type} mapped`);
    for (const target of Object.values(fold.map)) assert.ok(fold.order.includes(target));
  }
  assert.ok(FOLDS[6].order.length < FOLDS[8].order.length);
  assert.ok(FOLDS[4].order.length < FOLDS[5].order.length);
});

test('each table satisfies demands over the folded roster and stays triangle-based', () => {
  const built = buildMatrices();
  assert.equal(built.matrices.length, 4);
  for (const matrix of built.matrices) {
    assert.ok(matrix.stats.demands, `${matrix.id}: coverage demands met`);
    assert.ok(matrix.stats.triangles >= 2, `${matrix.id}: triangle web`);
    assert.ok(matrix.stats.edges >= matrix.order.length, `${matrix.id}: at least one win per type`);
    // Deterministic chart from the edge list.
    assert.deepEqual(chartFromEdges(matrix.order, matrix.edges), matrix.chart, matrix.id);
  }
  // The 8-type table is the live wheel (edge order comes from the chart scan).
  const eight = built.matrices.find((matrix) => matrix.id === '8');
  const norm = (edges) => edges.map(([a, b]) => `${a}>${b}`).sort();
  assert.deepEqual(eight.order, WHEEL.order);
  assert.deepEqual(norm(eight.edges), norm(WHEEL.edges));
});

test('memory load drops with type count', () => {
  const built = buildMatrices();
  const edges = Object.fromEntries(built.matrices.map((m) => [m.id, m.stats.edges]));
  assert.ok(edges['8'] > edges['6'], `${edges['8']} > ${edges['6']}`);
  assert.ok(edges['6'] > edges['5'], `${edges['6']} > ${edges['5']}`);
  assert.ok(edges['5'] > edges['4'], `${edges['5']} > ${edges['4']}`);
  assert.equal(triangles(built.matrices[0].order, built.matrices[0].chart).length, built.matrices[0].stats.triangles);
});

test('duel respects the selected table when resolving', async () => {
  const { createRequire } = await import('node:module');
  const require = createRequire(import.meta.url);
  const M = require(path.join(repo, 'mock/battle-mock.js'));
  const index = (name) => M.DATA.creatures.findIndex((entry) => entry.name === name);
  const state = M.newMock({
    mode: 'duel', scoring: 'bonus', you: index('ember'), foe: index('squid'), seed: 3, matrixId: '4',
  });
  M.startBattle(state);
  state.duelState.partyIndex = [[index('ember')], [index('squid')]];
  state.duelState.hp = [[M.creature(index('ember')).maxHp], [M.creature(index('squid')).maxHp]];
  state.duelState.activeSlot = [0, 0];
  // In the 4-type table fire folds to flame and water folds to tide; tide beats
  // flame, so the squid's splash is a bonus hit against ember.
  assert.equal(M.duelValue(state, 'fire', 'water'), 0.5);
  assert.equal(M.duelValue(state, 'water', 'fire'), 2);
  assert.equal(M.duelCreatureBonus(state, 1, 'water'), 1.5);
  assert.equal(M.duelCreatureBonus(state, 0, 'fire'), 0.5);
  state.duelState.picks = [0, 0];
  M.duelResolve(state);
  assert.equal(state.duelState.lines[0], 'BONUS LOSE');
  // The 8-type table keeps them neutral-ish: fire vs water is a loss but not a creature bonus.
  const full = M.newMock({
    mode: 'duel', scoring: 'bonus', you: index('ember'), foe: index('squid'), seed: 3, matrixId: '8',
  });
  M.startBattle(full);
  full.duelState.partyIndex = [[index('ember')], [index('squid')]];
  full.duelState.hp = [[M.creature(index('ember')).maxHp], [M.creature(index('squid')).maxHp]];
  full.duelState.activeSlot = [0, 0];
  assert.equal(M.duelCreatureBonus(full, 1, 'water'), 1.5, 'water still beats fire in the 8-type web');
  assert.equal(M.duelValue(full, 'fire', 'elder'), 1, 'elder stays neutral in the 8-type web');
  // Folded tables merge elder into the mystical side, so fire vs elder becomes the flame/spirit edge.
  const six = M.newMock({
    mode: 'duel', scoring: 'bonus', you: index('ember'), foe: index('squid'), seed: 3, matrixId: '6',
  });
  M.startBattle(six);
  const sixRelation = M.duelValue(six, 'fire', 'elder');
  assert.ok([1, 2, 0.5].includes(sixRelation), 'folded lookup resolves');
});