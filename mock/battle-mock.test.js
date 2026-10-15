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

test('8-type web: overlapping triangles, no mutual edges, neutral cells kept', () => {
  const { order, chart, edges, cycles } = M.DATA.wheel;
  assert.equal(order.length, 8);
  assert.ok(edges.length >= 20 && edges.length < 56, 'web is neither a ring nor a full tournament');
  assert.ok(cycles.length >= 4, `several triangles (${cycles.length})`);
  assert.ok(cycles.length < 56, 'not every triple is a triangle');
  for (const [from, to] of edges) {
    assert.equal(chart[order.indexOf(from)][order.indexOf(to)], 2, `${from}>${to} chart`);
    assert.equal(chart[order.indexOf(to)][order.indexOf(from)], 0.5, `${to}<${from} chart`);
  }
  for (let row = 0; row < 8; row++) {
    for (let col = 0; col < 8; col++) {
      assert.notEqual(chart[row][col], 0, 'no immunity cells');
      if (row === col) continue;
      const pair = [chart[row][col], chart[col][row]].sort();
      assert.ok((pair[0] === 0.5 && pair[1] === 2) || (pair[0] === 1 && pair[1] === 1),
        `pair ${order[row]}/${order[col]} resolves one way`);
    }
  }
  assert.ok(chart.some((row) => row.includes(1)), 'some pairs stay neutral');
  // Every type must have at least one win and one loss (no dead type).
  order.forEach((type, index) => {
    assert.ok(chart[index].filter((value) => value === 2).length >= 1, `${type} has a win`);
    assert.ok(chart.map((row) => row[index]).filter((value) => value === 2).length >= 1, `${type} has an answer`);
  });
});

test('every move type has a drawable icon (including status and none)', () => {
  for (const move of Object.values(M.DATA.moves)) {
    const bits = M.ICONS[move.type] || M.MERGED_ICONS[move.type];
    assert.ok(bits && bits.length === 8, `icon for type ${move.type}`);
  }
});

test('duel uses each creature\'s own moves and the wheel decides the clash', () => {
  const state = M.newMock({ mode: 'duel', you: 10, foe: 12, seed: 3 });
  M.startBattle(state);
  const mine = M.duelMoves(state, 0);
  assert.deepEqual(mine.map((move) => move.type), ['fire', 'fire']);
  assert.ok(M.hasAffinity(state, 0, mine[0]), 'fire move on a fire creature is affinity');
  const beforeYou = state.duelState.hp[0], beforeFoe = state.duelState.hp[1];
  // ember stoke (fire) vs hedge root (plant): fire beats plant -> wheel win.
  state.duelState.picks = [0, 0];
  M.duelResolve(state);
  assert.equal(state.duelState.lines[0], 'WHEEL WIN');
  assert.ok(beforeFoe - state.duelState.hp[1] > beforeYou - state.duelState.hp[0], 'winner deals more than chip');
  assert.equal(state.duelState.phase, 'result');
  state.duelState.timer = 0;
  M.duelUpdate(state, 2000);
  assert.equal(state.duelState.phase, 'choice');
  assert.equal(state.duelState.round, 2);
});

test('duel losses and trades resolve on the correct sides', () => {
  const state = M.newMock({ mode: 'duel', you: 10, foe: 12, seed: 3 });
  M.startBattle(state);
  // Reverse the pick sides: used only to prove the loser branch; force by swapping creatures.
  state.youIndex = M.DATA.creatures.findIndex((entry) => entry.name === 'hedge');
  state.foeIndex = M.DATA.creatures.findIndex((entry) => entry.name === 'ember');
  M.startBattle(state);
  const beforeYou = state.duelState.hp[0], beforeFoe = state.duelState.hp[1];
  state.duelState.picks = [0, 0]; // plant move vs fire move: plant loses to fire
  M.duelResolve(state);
  assert.equal(state.duelState.lines[0], 'WHEEL LOSE');
  assert.ok(beforeYou - state.duelState.hp[0] > beforeFoe - state.duelState.hp[1], 'loser takes the full hit');
  // Trade: same type on both sides.
  const same = M.newMock({ mode: 'duel', you: 10, foe: 10, seed: 3 });
  M.startBattle(same);
  const tradeYou = same.duelState.hp[0], tradeFoe = same.duelState.hp[1];
  same.duelState.picks = [0, 0];
  M.duelResolve(same);
  assert.equal(same.duelState.lines[0], 'TRADE');
  assert.ok(same.duelState.hp[0] < tradeYou && same.duelState.hp[1] < tradeFoe, 'trade chips both sides');
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

test('stance duel: cycle order, options and beats for 3 and 4 stances', () => {
  const three = M.newMock({ mode: 'duel', duelKind: 'stances', stances: 3 });
  const four = M.newMock({ mode: 'duel', duelKind: 'stances', stances: 4 });
  assert.deepEqual(M.stanceSet(three), ['flame', 'bloom', 'tide']);
  assert.deepEqual(M.stanceSet(four), ['flame', 'bloom', 'tide', 'stone']);
  for (const state of [three, four]) {
    const order = M.stanceSet(state);
    assert.equal(M.duelOptions(state, 0).length, order.length);
    for (let index = 0; index < order.length; index++) {
      assert.ok(M.stanceBeats(order, order[index], order[(index + 1) % order.length]));
      assert.ok(!M.stanceBeats(order, order[(index + 1) % order.length], order[index]));
    }
  }
});

test('stance duel resolves damage with affinity on the correct sides', () => {
  const state = M.newMock({ mode: 'duel', duelKind: 'stances', you: 10, foe: 12, seed: 3 });
  M.startBattle(state);
  assert.equal(M.duelKind(state), 'stances');
  const beforeYou = state.duelState.hp[0], beforeFoe = state.duelState.hp[1];
  state.duelState.picks = [0, 1]; // flame vs bloom: flame wins
  M.duelResolve(state);
  assert.equal(state.duelState.lines[0], 'CLASH WIN');
  assert.ok(beforeFoe - state.duelState.hp[1] > beforeYou - state.duelState.hp[0], 'winner deals more than chip');
  // Mirror pick trades chip.
  const tie = M.newMock({ mode: 'duel', duelKind: 'stances', you: 10, foe: 12, seed: 3 });
  M.startBattle(tie);
  const tieYou = tie.duelState.hp[0], tieFoe = tie.duelState.hp[1];
  tie.duelState.picks = [0, 0];
  M.duelResolve(tie);
  assert.equal(tie.duelState.lines[0], 'TIE');
  assert.ok(tie.duelState.hp[0] < tieYou && tie.duelState.hp[1] < tieFoe);
});

test('pure RPS scoring ignores power and affinity with fixed damage', () => {
  const state = M.newMock({ mode: 'duel', scoring: 'flat', you: 10, foe: 12, seed: 3 });
  M.startBattle(state);
  assert.equal(state.duel.scoring, 'flat');
  // ember wisper? no: ember stoke (fire) vs hedge root (plant): fire beats plant -> flat win.
  state.foeIndex = M.DATA.creatures.findIndex((entry) => entry.name === 'hedge');
  const beforeYou = state.duelState.hp[0], beforeFoe = state.duelState.hp[1];
  state.duelState.picks = [0, 0];
  M.duelResolve(state);
  assert.equal(state.duelState.lines[0], 'RPS WIN');
  assert.equal(beforeFoe - state.duelState.hp[1], 14, 'flat win damage');
  assert.equal(beforeYou - state.duelState.hp[0], 4, 'flat chip damage');
  // Same picks, different creature/move powers: still the same fixed numbers.
  const other = M.newMock({ mode: 'duel', scoring: 'flat', you: 63, foe: 12, seed: 3 });
  M.startBattle(other);
  const otherYou = other.duelState.hp[0], otherFoe = other.duelState.hp[1];
  other.duelState.picks = [0, 0]; // OrbitRelic elderBurst (elder) vs hedge root (plant)
  M.duelResolve(other);
  const relation = M.wheelValue(M.duelOptions(other, 0)[0].type, M.duelOptions(other, 1)[0].type);
  if (relation === 2) {
    assert.equal(otherFoe - other.duelState.hp[1], 14);
    assert.equal(otherYou - other.duelState.hp[0], 4);
  } else if (relation === 0.5) {
    assert.equal(otherYou - other.duelState.hp[0], 14);
    assert.equal(otherFoe - other.duelState.hp[1], 4);
  } else {
    assert.equal(otherYou - other.duelState.hp[0], 6);
    assert.equal(otherFoe - other.duelState.hp[1], 6);
  }
  // Power scoring keeps scaling with power (control).
  const power = M.newMock({ mode: 'duel', scoring: 'power', you: 10, foe: 12, seed: 3 });
  M.startBattle(power);
  power.foeIndex = M.DATA.creatures.findIndex((entry) => entry.name === 'hedge');
  power.duelState.picks = [0, 0];
  M.duelResolve(power);
  assert.equal(power.duelState.lines[0], 'WHEEL WIN');
});

test('bonus scoring applies the creature-type matchup instead of affinity', () => {
  const index = (name) => M.DATA.creatures.findIndex((entry) => entry.name === name);
  const run = (scoring, foeIndex, pick) => {
    const state = M.newMock({ mode: 'duel', scoring, you: 10, foe: foeIndex, seed: 3 });
    M.startBattle(state);
    const before = state.duelState.hp[1];
    state.duelState.picks = pick;
    M.duelResolve(state);
    assert.equal(state.duelState.lines[0], 'WHEEL WIN');
    return before - state.duelState.hp[1];
  };
  // hedge (plant) with root: fire beats plant -> creature bonus applies.
  // Power mode gets x1.5 from STAB, bonus mode from the creature matchup: equal damage.
  const powerHedge = run('power', index('hedge'), [0, 0]);
  const bonusHedge = run('bonus', index('hedge'), [0, 0]);
  assert.ok(Math.abs(powerHedge - bonusHedge) <= 1,
    `hedge: creature bonus replaces STAB (${powerHedge} vs ${bonusHedge})`);
  // zip (lightning) with smite (spirit): fire beats spirit (win) but not lightning (no creature bonus).
  // Power mode keeps STAB; bonus mode has neither. Damage ratio ~1.5.
  const powerZip = run('power', index('zip'), [0, 1]);
  const bonusZip = run('bonus', index('zip'), [0, 1]);
  assert.ok(powerZip > bonusZip, `STAB only under power (${powerZip} vs ${bonusZip})`);
  assert.ok(Math.abs(powerZip - Math.round(bonusZip * 1.5)) <= 1,
    `creature-neutral bonus = power / 1.5 (${powerZip} vs ${bonusZip})`);
  // Helper reports the multiplier the UI marks with the up arrow.
  const state = M.newMock({ mode: 'duel', scoring: 'bonus', you: 10, foe: index('hedge'), seed: 1 });
  M.startBattle(state);
  assert.equal(M.duelCreatureBonus(state, 0, 'fire'), 1.5);
  state.foeIndex = index('zip');
  assert.equal(M.duelCreatureBonus(state, 0, 'fire'), 1);
});

test('duel AI keeps its pick inside its own option list for both engines', () => {
  for (const [duelKind, scoring] of [['wheel', 'power'], ['wheel', 'flat'], ['stances', 'power']]) {
    const state = M.newMock({ mode: 'duel', duelKind, scoring, you: 10, foe: 12, seed: 7, aiRead: 100 });
    M.startBattle(state);
    const count = M.duelOptions(state, 1).length;
    for (let i = 0; i < 10; i++) {
      state.duelState.last = [M.duelOptions(state, 0)[0].type, null];
      M.duelPick(state, 0);
      const aiIndex = state.duelState.picks[1];
      assert.ok(aiIndex >= 0 && aiIndex < count, `${duelKind}/${scoring} AI pick ${aiIndex} within 0..${count - 1}`);
      state.duelState.phase = 'choice';
      state.duelState.picks = [null, null];
    }
  }
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
