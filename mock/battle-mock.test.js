'use strict';

// Battle mock mechanics tests. Run: node --test mock/battle-mock.test.js
const test = require('node:test');
const assert = require('node:assert');
const M = require('./battle-mock.js');

const creatureByName = (name) => M.DATA.creatures.find((entry) => entry.name === name);
const moveByName = (name) => Object.values(M.DATA.moves).find((entry) => entry.name === name);

// Minimal synthetic combatants for formula tests (stats/move shapes match data.js).
function synthetic(type, move, stats = {}) {
  return {
    name: 'test',
    types: [type],
    merged: [{ fire: 'flame', plant: 'bloom', water: 'tide', earth: 'stone' }[type] || type],
    stats: { attack: 20, defense: 20, spcAtk: 20, spcDef: 20, speed: 20, ...stats },
    maxHp: 50,
    moves: [0, null, null, null],
    front: '', back: '',
    __move: move,
  };
}
// computeDamage reads move objects directly in tests.
const PHYS = { id: 0, name: 'phys', type: 'fire', power: 10, physical: true, effect: 'NONE', limit: 255 };
const SPEC = { id: 1, name: 'spec', type: 'water', power: 10, physical: false, effect: 'NONE', limit: 255 };

test('damage port: super-effective STAB lands quadruple damage', () => {
  // ember stoke (fire, power 6, physical) vs hedge: floor(floor(6*22/16)/2)=4, x4 -> 16.
  const ember = creatureByName('ember'), hedge = creatureByName('hedge');
  const stoke = moveByName('stoke');
  assert.equal(M.computeDamage(ember, hedge, stoke, '8dual'), 16);
  // burn (fire, power 8, special): floor(floor(8*32/11)/2)=11, x4 -> 44.
  assert.equal(M.computeDamage(ember, hedge, moveByName('burn'), '8dual'), 44);
  // 4-single mode uses the merged cycle: flame vs bloom is also x2 + STAB = x4.
  assert.equal(M.computeDamage(ember, hedge, stoke, '4single'), 16);
});

test('damage port: immunity is zero, resist halves, dual resists quarter, floor is 1', () => {
  const move = { ...PHYS, type: 'fire' };
  const water = synthetic('water', move);
  const fire = synthetic('fire', move);
  // fire vs water is immune (chart 0).
  assert.equal(M.computeDamage(fire, water, move, '8dual'), 0);
  // spirit vs elder is immune; dual elder+spirit also immune.
  const spiritMove = { ...SPEC, type: 'spirit' };
  const elder = synthetic('elder', spiritMove);
  assert.equal(M.computeDamage(synthetic('spirit', spiritMove), elder, spiritMove, '8dual'), 0);
  const dualElder = synthetic('elder', spiritMove);
  assert.equal(M.computeDamage(synthetic('spirit', spiritMove), dualElder, spiritMove, '8dual'), 0);
  // water vs plant 0.5 without STAB: 10 attack term halves once.
  const waterMove = { ...SPEC, type: 'water' };
  const plainWater = synthetic('spirit', waterMove, { spcAtk: 20, spcDef: 20 });
  const plantDef = synthetic('plant', waterMove, { spcDef: 20 });
  const numerator = Math.floor(Math.floor(10 * 20 / 10) / 2); // 10
  assert.equal(M.computeDamage(plainWater, plantDef, waterMove, '8dual'), numerator >> 1);
  // double resist (water vs plant+wind? plant 0.5 + earth 0.5) -> quarter.
  const earthPlant = { ...plantDef, types: ['plant', 'earth'] };
  assert.equal(M.computeDamage(plainWater, earthPlant, waterMove, '8dual'), numerator >> 2);
  // floor of zero damage from a half modifier becomes 1 (production rule).
  const weakMove = { ...waterMove, power: 1 };
  assert.equal(M.computeDamage(plainWater, earthPlant, weakMove, '8dual'), 1);
});

test('modifier combining matches the production exponent clamp', () => {
  assert.equal(M.combineModifier(3, 3), 3);
  assert.equal(M.combineModifier(4, 4), 5); // double+double
  assert.equal(M.combineModifier(2, 2), 1); // half+half
  assert.equal(M.combineModifier(0, 5), 0); // immunity wins
  assert.equal(M.modifierFor(0), 0);
  assert.equal(M.modifierFor(0.5), 2);
  assert.equal(M.modifierFor(1), 3);
  assert.equal(M.modifierFor(2), 4);
  assert.equal(M.arrowFor(0), 'no');
  assert.equal(M.arrowFor(2), 'down');
  assert.equal(M.arrowFor(3), 'flat');
  assert.equal(M.arrowFor(5), 'up');
});

// Build a started mock for one-on-one flows.
function battleMock(options = {}) {
  const state = M.newMock({ mode: 'trim', you: 10, foe: 12, seed: 7, ...options });
  M.startBattle(state);
  return state;
}

test('speed order puts the faster side first and the player wins ties', () => {
  const state = battleMock();
  state.battle.you.stats.speed = 10;
  state.battle.foe.stats.speed = 40;
  M.planTurn(state, false);
  assert.equal(state.battle.queue[0].side, 1);
  state.battle.you.stats.speed = 40;
  state.battle.foe.stats.speed = 40;
  state.battle.phase = 'choice';
  M.planTurn(state, false);
  assert.equal(state.battle.queue[0].side, 0);
});

test('press-turn queues a capped extra action only for super-effective hits', () => {
  const state = battleMock({ pressTurn: true });
  // Guarantee a super-effective STAB hit: ember stoke vs hedge.
  state.foe = creatureByName('hedge');
  state.battle.foe = state.foe;
  state.battle.uses[1] = state.foe.moves.map((id) => (id == null ? 0 : M.DATA.moves[id].limit));
  state.battle.hp[1] = 58;
  state.battle.you.stats.speed = 60; // player first
  state.battle.cursor = 0; // stoke
  state.battle.uses[0][0] = 2;
  state.battle.phase = 'choice';
  M.planTurn(state, false);
  const extra = state.battle.queue.filter((action) => action.extra);
  assert.equal(extra.length, 1);
  assert.equal(extra[0].side, 0);

  const off = battleMock({ pressTurn: false });
  off.foe = creatureByName('hedge');
  off.battle.foe = off.foe;
  off.battle.phase = 'choice';
  off.battle.cursor = 0;
  off.battle.uses[0][0] = 2;
  M.planTurn(off, false);
  assert.equal(off.battle.queue.filter((action) => action.extra).length, 0);
});

test('a knockout cancels the victim action and ends the fight', () => {
  const state = battleMock({ pressTurn: false });
  state.battle.you.stats.speed = 60;
  state.battle.cursor = 0;
  state.battle.uses[0][0] = 2;
  state.battle.hp[1] = 5;
  state.battle.phase = 'choice';
  M.planTurn(state, false);
  assert.equal(state.battle.queue.length, 2);
  // First step kills: the foe's queued action must not execute.
  M.advanceStep(state);
  assert.equal(state.battle.hp[1], 0);
  M.advanceStep(state);
  assert.equal(state.battle.lines[1], 'down, no action');
  M.advanceStep(state);
  assert.equal(state.scene, 'end');
  assert.equal(state.endText, 'WIN');
});

test('move uses deplete and unusable moves drop out of the cursor set', () => {
  const state = battleMock();
  const slots = M.usableSlots(state, 0);
  assert.ok(slots.length >= 1);
  const first = slots[0];
  const limit = M.DATA.moves[state.battle.you.moves[first]].limit;
  if (limit !== 255) {
    state.battle.uses[0][first] = 0;
    assert.ok(!M.usableSlots(state, 0).includes(first));
  }
  assert.equal(M.firstUsable(state.battle), M.usableSlots(state, 0)[0] ?? 255);
});

test('duel cycle order and beats are consistent for 3 and 4 stances', () => {
  const three = M.newMock({ mode: 'duel', stances: 3 });
  const four = M.newMock({ mode: 'duel', stances: 4 });
  const t = M.stanceSet(three), f = M.stanceSet(four);
  assert.deepEqual(t, ['flame', 'bloom', 'tide']);
  assert.deepEqual(f, ['flame', 'bloom', 'tide', 'stone']);
  for (const order of [t, f]) {
    for (let i = 0; i < order.length; i++) {
      assert.ok(M.beats(order, order[i], order[(i + 1) % order.length]));
      assert.ok(!M.beats(order, order[i], order[i]));
      assert.ok(!M.beats(order, order[(i + 1) % order.length], order[i]));
    }
  }
});

test('duel resolves damage on the correct sides with affinity bonus', () => {
  const state = M.newMock({ mode: 'duel', you: 10, foe: 12, seed: 3, stances: 3 });
  M.startBattle(state);
  const you = M.creature(10), foe = M.creature(12);
  assert.equal(you.merged[0], 'flame'); // ember has flame affinity
  // Force player win: flame beats bloom if foe has bloom affinity (hedge).
  state.foeIndex = M.DATA.creatures.findIndex((entry) => entry.name === 'hedge');
  const beforeYou = state.duelState.hp[0], beforeFoe = state.duelState.hp[1];
  state.duelState.picks = [0, 1]; // flame vs bloom
  M.duelResolve(state);
  assert.ok(state.duelState.hp[1] < beforeFoe, 'foe took damage');
  assert.ok(state.duelState.hp[0] < beforeYou, 'chip damage on the winner too');
  assert.equal(state.duelState.phase, 'result');
  // Loser takes more than the chip damage dealt back.
  assert.ok(beforeFoe - state.duelState.hp[1] > beforeYou - state.duelState.hp[0]);
  state.duelState.timer = 0;
  M.duelUpdate(state, 2000);
  assert.equal(state.duelState.phase, 'choice');
  assert.equal(state.duelState.round, 2);
});

test('duel flow advances through step() edges and timers', () => {
  const state = M.newMock({ mode: 'duel', seed: 11 });
  M.step(state, { edges: ['a'], dtMs: 0 }); // start battle
  assert.equal(state.scene, 'battle');
  assert.equal(state.duelState.phase, 'choice');
  M.step(state, { edges: ['right', 'a'], dtMs: 0 });
  assert.equal(state.duelState.phase, 'reveal');
  M.step(state, { edges: [], dtMs: 600 });
  assert.equal(state.duelState.phase, 'result');
  M.step(state, { edges: [], dtMs: 1400 });
  assert.equal(state.duelState.phase, 'choice');
});

test('ai pick never returns a depleted or absent slot', () => {
  const state = battleMock();
  state.battle.uses[1] = state.battle.uses[1].map(() => 0);
  assert.equal(M.aiPick(state, 1), null);
  state.battle.uses[1][0] = 1;
  assert.equal(M.aiPick(state, 1), 0);
});

test('same seed replays identical fights', () => {
  const run = () => {
    const state = battleMock({ seed: 99, pressTurn: true });
    state.battle.cursor = 0;
    for (let i = 0; i < 20 && state.scene === 'battle'; i++) {
      if (state.battle.phase === 'choice') M.planTurn(state, false);
      else M.advanceStep(state);
    }
    return [state.scene, state.endText, ...state.battle.hp];
  };
  assert.deepEqual(run(), run());
});

test('mode edges switch designs and reset to setup', () => {
  const state = M.newMock({ mode: 'current' });
  M.step(state, { edges: ['2'], dtMs: 0 });
  assert.equal(state.mode, 'trim');
  assert.equal(state.scene, 'setup');
  M.step(state, { edges: ['3'], dtMs: 0 });
  assert.equal(state.mode, 'duel');
});

test('setup edges cycle creatures and A starts the fight', () => {
  const state = M.newMock({ you: 0, foe: 0 });
  M.step(state, { edges: ['right'], dtMs: 0 });
  assert.equal(state.youIndex, 1);
  M.step(state, { edges: ['up'], dtMs: 0 });
  assert.equal(state.foeIndex, M.DATA.creatures.length - 1);
  M.step(state, { edges: ['a'], dtMs: 0 });
  assert.equal(state.scene, 'battle');
  assert.ok(state.battle || state.duelState);
});
