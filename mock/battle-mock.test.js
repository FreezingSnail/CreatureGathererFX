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

// Active-slot HP accessor for duel tests.
const hp = (state, side) => state.duelState.hp[side][state.duelState.activeSlot[side]];

// Pin a duel side to exact creatures so tests stay deterministic (random team
// rolling is covered by its own test).
function pinTeam(state, side, indices) {
  state.duelState.partyIndex[side] = [...indices];
  state.duelState.hp[side] = indices.map((index) => M.creature(index).maxHp);
  state.duelState.activeSlot[side] = 0;
}

function duelMock({ you, foe, teamSize = 1, matrixId = '8', ...options } = {}) {
  // Tests pin the 8-type table unless they exercise another one; the demo
  // default is 5.
  const state = M.newMock({ mode: 'duel', teamSize, you, foe, matrixId, ...options });
  M.startBattle(state);
  if (you != null) pinTeam(state, 0, [you]);
  if (foe != null) pinTeam(state, 1, [foe]);
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

test('status and utility moves are excluded from every pick list', () => {
  for (let index = 0; index < M.DATA.creatures.length; index++) {
    const state = duelMock({ you: index, foe: index, seed: 1 });
    for (const option of M.duelMoves(state, 0)) {
      assert.ok(option.type !== 'status' && option.type !== 'none', `${option.name} must not be a pick`);
    }
  }
  // bell keeps only wisper; its Sharpen (status) is gone from the battle menu too.
  const bell = M.DATA.creatures.findIndex((entry) => entry.name === 'bell');
  const battle = M.newMock({ mode: 'trim', you: bell, foe: 0, seed: 5 });
  M.startBattle(battle);
  const usable = M.usableSlots(battle, 0).map((slot) => M.moveOf(M.creature(bell), slot).name);
  assert.deepEqual(usable, ['wisper']);
});

test('every move type has a drawable icon (including status and none)', () => {
  for (const move of Object.values(M.DATA.moves)) {
    const bits = M.ICONS[move.type] || M.MERGED_ICONS[move.type];
    assert.ok(bits && bits.length === 8, `icon for type ${move.type}`);
  }
});

test('duel uses each creature\'s own moves and the wheel decides the clash', () => {
  const state = duelMock({ you: 10, foe: 12, seed: 3 });
  const mine = M.duelMoves(state, 0);
  assert.deepEqual(mine.map((move) => move.type), ['fire', 'fire']);
  assert.ok(M.hasAffinity(state, 0, mine[0]), 'fire move on a fire creature is affinity');
  const beforeYou = hp(state, 0), beforeFoe = hp(state, 1);
  // ember stoke (fire) vs hedge root (plant): fire beats plant -> wheel win.
  state.duelState.picks = [0, 0];
  M.duelResolve(state);
  assert.equal(state.duelState.lines[0], 'WHEEL WIN');
  assert.ok(beforeFoe - hp(state, 1) > beforeYou - hp(state, 0), 'winner deals more than chip');
  assert.equal(state.duelState.phase, 'result');
  state.duelState.timer = 0;
  M.duelUpdate(state, 2000);
  assert.equal(state.duelState.phase, 'choice');
  assert.equal(state.duelState.round, 2);
});

test('duel losses and trades resolve on the correct sides', () => {
  const index = (name) => M.DATA.creatures.findIndex((entry) => entry.name === name);
  const state = duelMock({ you: index('hedge'), foe: index('ember'), seed: 3 });
  const beforeYou = hp(state, 0), beforeFoe = hp(state, 1);
  state.duelState.picks = [0, 0]; // plant move vs fire move: plant loses to fire
  M.duelResolve(state);
  assert.equal(state.duelState.lines[0], 'WHEEL LOSE');
  assert.ok(beforeYou - hp(state, 0) > beforeFoe - hp(state, 1), 'loser takes the full hit');
  // Trade: same type on both sides.
  const same = duelMock({ you: 10, foe: 10, seed: 3 });
  const tradeYou = hp(same, 0), tradeFoe = hp(same, 1);
  same.duelState.picks = [0, 0];
  M.duelResolve(same);
  assert.equal(same.duelState.lines[0], 'TRADE');
  assert.ok(hp(same, 0) < tradeYou && hp(same, 1) < tradeFoe, 'trade chips both sides');
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

test('teams roll 3v3 from the seed and are deterministic per seed', () => {
  const a = M.newMock({ mode: 'duel', seed: 21 });
  M.startBattle(a);
  assert.equal(a.duelState.partyIndex[0].length, 3);
  assert.equal(a.duelState.partyIndex[1].length, 3);
  for (const side of [0, 1]) {
    assert.equal(new Set(a.duelState.partyIndex[side]).size, 3, 'distinct members');
    assert.deepEqual(a.duelState.hp[side], a.duelState.partyIndex[side].map((i) => M.creature(i).maxHp));
  }
  const b = M.newMock({ mode: 'duel', seed: 21 });
  M.startBattle(b);
  assert.deepEqual(a.duelState.partyIndex, b.duelState.partyIndex, 'same seed, same teams');
});

test('stance duel: cycle order, options and beats for 3 and 4 stances', () => {
  const three = M.newMock({ mode: 'duel', duelKind: 'stances', stances: 3 });
  const four = M.newMock({ mode: 'duel', duelKind: 'stances', stances: 4 });
  assert.deepEqual(M.stanceSet(three), ['flame', 'bloom', 'tide']);
  assert.deepEqual(M.stanceSet(four), ['flame', 'bloom', 'tide', 'stone']);
  for (const state of [three, four]) {
    const order = M.stanceSet(state);
    assert.equal(M.duelOptions(state, 0).length, order.length);
    for (let position = 0; position < order.length; position++) {
      assert.ok(M.stanceBeats(order, order[position], order[(position + 1) % order.length]));
      assert.ok(!M.stanceBeats(order, order[(position + 1) % order.length], order[position]));
    }
  }
});

test('stance duel resolves damage with affinity on the correct sides', () => {
  const state = duelMock({ duelKind: 'stances', you: 10, foe: 12, seed: 3 });
  assert.equal(M.duelKind(state), 'stances');
  const beforeYou = hp(state, 0), beforeFoe = hp(state, 1);
  state.duelState.picks = [0, 1]; // flame vs bloom: flame wins
  M.duelResolve(state);
  assert.equal(state.duelState.lines[0], 'CLASH WIN');
  assert.ok(beforeFoe - hp(state, 1) > beforeYou - hp(state, 0), 'winner deals more than chip');
  // Mirror pick trades chip.
  const tie = duelMock({ duelKind: 'stances', you: 10, foe: 12, seed: 3 });
  const tieYou = hp(tie, 0), tieFoe = hp(tie, 1);
  tie.duelState.picks = [0, 0];
  M.duelResolve(tie);
  assert.equal(tie.duelState.lines[0], 'TIE');
  assert.ok(hp(tie, 0) < tieYou && hp(tie, 1) < tieFoe);
});

test('pure RPS scoring ignores power and affinity with fixed damage', () => {
  const index = (name) => M.DATA.creatures.findIndex((entry) => entry.name === name);
  const state = duelMock({ scoring: 'flat', you: index('ember'), foe: index('hedge'), seed: 3 });
  assert.equal(state.duel.scoring, 'flat');
  const beforeYou = hp(state, 0), beforeFoe = hp(state, 1);
  state.duelState.picks = [0, 0]; // stoke (fire) vs root (plant): fire beats plant -> flat win
  M.duelResolve(state);
  assert.equal(state.duelState.lines[0], 'RPS WIN');
  assert.equal(beforeFoe - hp(state, 1), 14, 'flat win damage');
  assert.equal(beforeYou - hp(state, 0), 4, 'flat chip damage');
  // Same picks, different creature/move powers: still the same fixed numbers.
  const other = duelMock({ scoring: 'flat', you: 63, foe: index('hedge'), seed: 3 });
  const otherYou = hp(other, 0), otherFoe = hp(other, 1);
  other.duelState.picks = [0, 0]; // OrbitRelic elderBurst (elder) vs hedge root (plant)
  M.duelResolve(other);
  const relation = M.wheelValue(M.duelOptions(other, 0)[0].type, M.duelOptions(other, 1)[0].type);
  if (relation === 2) {
    assert.equal(otherFoe - hp(other, 1), 14);
    assert.equal(otherYou - hp(other, 0), 4);
  } else if (relation === 0.5) {
    assert.equal(otherYou - hp(other, 0), 14);
    assert.equal(otherFoe - hp(other, 1), 4);
  } else {
    assert.equal(otherYou - hp(other, 0), 6);
    assert.equal(otherFoe - hp(other, 1), 6);
  }
  // Power scoring keeps scaling with power (control).
  const power = duelMock({ scoring: 'power', you: index('ember'), foe: index('hedge'), seed: 3 });
  power.duelState.picks = [0, 0];
  M.duelResolve(power);
  assert.equal(power.duelState.lines[0], 'WHEEL WIN');
});

test('bonus spelling scales hits by the creature matchup, penalty included', () => {
  const index = (name) => M.DATA.creatures.findIndex((entry) => entry.name === name);
  const run = (scoring, foeIndex, pick, youIndex = index('ember')) => {
    const state = duelMock({ scoring, you: youIndex, foe: foeIndex, seed: 3 });
    const beforeFoe = hp(state, 1), beforeYou = hp(state, 0);
    state.duelState.picks = pick;
    M.duelResolve(state);
    return {
      line: state.duelState.lines[0],
      dealt: beforeFoe - hp(state, 1),
      taken: beforeYou - hp(state, 0),
    };
  };
  // fire beats plant (hedge creature): bonus win = round(14 * 1.5) = 21, chip 4.
  const bonusHedge = run('bonus', index('hedge'), [0, 0]);
  assert.equal(bonusHedge.line, 'BONUS WIN');
  assert.equal(bonusHedge.dealt, 21);
  assert.equal(bonusHedge.taken, 2); // our chip also loses the matchup (plant move vs their fire type)
  // fire vs lightning creature (zip) with a spirit move: clash win but the move
  // type loses to the creature's type -> weak win, 14 * 0.5 = 7.
  const weakZip = run('bonus', index('zip'), [0, 1]);
  assert.equal(weakZip.line, 'WEAK WIN');
  assert.equal(weakZip.dealt, 7);
  assert.equal(weakZip.taken, 2); // their spirit chip loses to our fire type
  // fire vs water creature (squid): fire loses the clash and the matchup:
  // foe water beats fire -> 21 against, our chip 4 * 0.5 = 2.
  const weakLoss = run('bonus', index('squid'), [0, 0]);
  assert.equal(weakLoss.line, 'BONUS LOSE');
  assert.equal(weakLoss.dealt, 2);
  assert.equal(weakLoss.taken, 21);
  // Pure spelling stays 14 regardless, on the same matchup.
  const pureHedge = run('flat', index('hedge'), [0, 0]);
  assert.equal(pureHedge.line, 'RPS WIN');
  assert.equal(pureHedge.dealt, 14);
  // Damage ignores move power entirely: stoke (p6) and burn (p8) land the same numbers.
  const stoke = run('bonus', index('hedge'), [0, 0]);
  const burn = run('bonus', index('hedge'), [1, 0]);
  assert.equal(stoke.dealt, burn.dealt);
  // Trade scales both sides by their own matchup modifier: ember vs ember is neutral.
  const trade = run('bonus', index('hedge'), [0, 0], index('hedge'));
  assert.equal(trade.line, 'TRADE');
  assert.equal(trade.dealt, 6);
  assert.equal(trade.taken, 6);
  // Foe-side bonus mirrors: their winning hit is scaled when their move beats your creature type.
  const foeBonus = run('bonus', index('zip'), [0, 0], index('squid')); // zap (lightning) > splash (water)
  assert.equal(foeBonus.line, 'BONUS LOSE');
  assert.equal(foeBonus.dealt, 2);
  assert.equal(foeBonus.taken, 21);
  // Helper reports the multiplier the cell markers use, penalty included.
  const vsHedge = duelMock({ scoring: 'bonus', you: index('ember'), foe: index('hedge'), seed: 1 });
  assert.equal(M.duelCreatureBonus(vsHedge, 0, 'fire'), 1.5);
  const vsZip = duelMock({ scoring: 'bonus', you: index('ember'), foe: index('zip'), seed: 1 });
  assert.equal(M.duelCreatureBonus(vsZip, 0, 'fire'), 0.5);
  const vsSquid = duelMock({ scoring: 'bonus', you: index('ember'), foe: index('squid'), seed: 1 });
  assert.equal(M.duelCreatureBonus(vsSquid, 0, 'water'), 1, 'mirror type is neutral');
  assert.equal(M.duelCreatureBonus(vsSquid, 0, 'fire'), 0.5, 'fire loses to water');
});

test('bonus AI prefers the best matchup tier and never feeds weak picks when avoidable', () => {
  const index = (name) => M.DATA.creatures.findIndex((entry) => entry.name === name);
  // AI ScatterCrab [earth, lightning] vs player squid [water]:
  // earth loses to water (weak), lightning beats water (bonus) -> always bolt when not reading.
  for (let seed = 1; seed <= 6; seed++) {
    const state = duelMock({ scoring: 'bonus', you: index('squid'), foe: index('ScatterCrab'), seed });
    state.duel.aiPersona = 'reader';
    state.duel.aiRead = 0;
    M.duelPick(state, 0);
    const pick = state.duelState.picks[1];
    assert.equal(M.duelOptions(state, 1)[pick].type, 'lightning', `seed ${seed}`);
  }
});

test('personas: frequency, anti-repeat and exploiter guess the player type', () => {
  const index = (name) => M.DATA.creatures.findIndex((entry) => entry.name === name);
  const build = (persona) => {
    // Player = ember (fire moves). AI = bigsquid (water moves, beats fire).
    const state = duelMock({ scoring: 'bonus', you: index('ember'), foe: index('bigsquid'), seed: 9 });
    state.duel.aiPersona = persona;
    state.duel.aiRead = 100;
    return state;
  };
  // frequency: history fire/fire/plant -> expects fire -> water beats fire.
  const frequency = build('frequency');
  frequency.duelState.history = ['fire', 'fire', 'plant'];
  M.duelPick(frequency, 0);
  assert.equal(M.duelOptions(frequency, 1)[frequency.duelState.picks[1]].type, 'water');
  assert.equal(frequency.duelState.aiRead, true);
  // anti-repeat: history fire/plant -> last (plant) discounted -> expects fire.
  const anti = build('anti-repeat');
  anti.duelState.history = ['fire', 'plant'];
  M.duelPick(anti, 0);
  assert.equal(M.duelOptions(anti, 1)[anti.duelState.picks[1]].type, 'water');
  // exploiter: post-read samples override frequency.
  const exploiter = build('exploiter');
  exploiter.duelState.history = ['plant', 'plant'];
  exploiter.duelState.postReadCounts = { fire: 3 };
  M.duelPick(exploiter, 0);
  assert.equal(M.duelOptions(exploiter, 1)[exploiter.duelState.picks[1]].type, 'water');
  // reader keeps the old behaviour: counter the last revealed type.
  const reader = build('reader');
  reader.duelState.history = ['fire'];
  M.duelPick(reader, 0);
  assert.equal(M.duelOptions(reader, 1)[reader.duelState.picks[1]].type, 'water');
  // random never fires a read even with history.
  const random = build('random');
  random.duelState.history = ['fire', 'fire', 'fire', 'fire'];
  M.duelPick(random, 0);
  assert.equal(random.duelState.aiRead, false);
});

test('fallback when no counter exists is uniform random, not a boost preference', () => {
  const index = (name) => M.DATA.creatures.findIndex((entry) => entry.name === name);
  // player = ember (fire moves); AI = bell (spirit move, cannot beat fire).
  const state = duelMock({ scoring: 'bonus', you: index('ember'), foe: index('bell'), seed: 4 });
  state.duel.aiRead = 100;
  state.duel.aiPersona = 'reader';
  state.duelState.history = ['fire'];
  M.duelPick(state, 0);
  assert.equal(state.duelState.aiRead, false, 'no counter -> no read flag');
  assert.equal(state.duelState.picks[1], 0, 'single option picked');
  // Player history recording feeds the models.
  const recorded = duelMock({ scoring: 'bonus', you: index('ember'), foe: index('bell'), seed: 2 });
  recorded.duelState.readFiredPrev = true;
  M.recordPlayerPick(recorded, 'fire');
  recorded.duelState.thisReadFired = true;
  M.recordPlayerPick(recorded, 'plant');
  assert.deepEqual(recorded.duelState.history, ['fire', 'plant']);
  assert.deepEqual(recorded.duelState.postReadCounts, { fire: 1 });
  assert.equal(recorded.duelState.readFiredPrev, true);
});

test('switching forfeits the round: the non-switcher lands a default win', () => {
  const index = (name) => M.DATA.creatures.findIndex((entry) => entry.name === name);
  const hedge = index('hedge'), ember = index('ember'), cloud = index('cloud'), cuircuit = index('cuircuit');
  const make = () => {
    const state = duelMock({ scoring: 'bonus', you: hedge, foe: ember, seed: 3 });
    pinTeam(state, 0, [hedge, cloud]);
    pinTeam(state, 1, [ember, cuircuit]);
    return state;
  };
  const attack = make();
  attack.duelState.picks = [0, 0];
  M.duelResolve(attack);
  const clashLines = attack.duelState.lines[0];
  // Player switches (-1 = next living member) while the foe attacks: default win for the foe.
  const swap = make();
  const incoming = M.creature(cloud);
  const emberMove = M.duelMoves(swap, 1)[0];
  const expectedBonus = incoming.types.some((type) => M.wheelBeats(emberMove.type, type)) ? 21 : 14;
  swap.duelState.picks = [-1, 0];
  M.duelResolve(swap);
  assert.equal(swap.duelState.lines[0], 'DEFAULT WIN');
  assert.equal(swap.duelState.activeSlot[0], 1, 'player swap applied');
  assert.equal(hp(swap, 0), incoming.maxHp - expectedBonus, `incoming takes the default win (${expectedBonus})`);
  assert.equal(hp(swap, 1), M.creature(ember).maxHp, 'attacker untouched');
  // Targeted switch (AI-style encoding) goes to the requested slot.
  const targeted = make();
  pinTeam(targeted, 0, [hedge, cloud, hedge]);
  targeted.duelState.picks = [M.SWITCH_BASE + 2, 0];
  M.duelResolve(targeted);
  assert.equal(targeted.duelState.activeSlot[0], 2, 'targeted slot chosen');
  // Both sides switch: no damage at all.
  const both = make();
  const beforeHp = JSON.parse(JSON.stringify(both.duelState.hp));
  both.duelState.picks = [-1, -1];
  M.duelResolve(both);
  assert.equal(both.duelState.lines[0], 'SWAP');
  assert.deepEqual(both.duelState.hp, beforeHp, 'no damage on a double switch');
  assert.ok(clashLines.length > 0);
});

test('a KO on the incoming swap reverts to the outgoing creature, and the side dies only when all faint', () => {
  const index = (name) => M.DATA.creatures.findIndex((entry) => entry.name === name);
  const hedge = index('hedge'), ember = index('ember'), cloud = index('cloud');
  const state = duelMock({ scoring: 'bonus', you: hedge, foe: ember, seed: 3 });
  pinTeam(state, 0, [hedge, cloud]);
  state.duelState.hp[0][1] = 5; // fragile incoming
  state.duelState.picks = [-1, 0];
  M.duelResolve(state);
  assert.equal(state.duelState.hp[0][1], 0, 'incoming faints');
  assert.equal(state.duelState.activeSlot[0], 0, 'reverted to the outgoing creature');
  assert.ok(M.sideAlive(state, 0), 'side still alive');
  // Fainting the active promotes a living bench member.
  const promoted = duelMock({ scoring: 'bonus', you: hedge, foe: ember, seed: 3 });
  pinTeam(promoted, 0, [hedge, cloud]);
  promoted.duelState.hp[0][0] = 3;
  promoted.duelState.picks = [0, 0];
  M.duelResolve(promoted);
  assert.equal(promoted.duelState.hp[0][0], 0, 'active faints');
  assert.equal(promoted.duelState.activeSlot[0], 1, 'bench promoted');
  assert.ok(M.sideAlive(promoted, 0), 'side alive with the bench');
  promoted.duelState.hp[0][1] = 0;
  assert.equal(M.sideAlive(promoted, 0), false, 'side dead when all faint');
});

test('the AI switches when only the bench holds a counter for its guess', () => {
  const index = (name) => M.DATA.creatures.findIndex((entry) => entry.name === name);
  const hasCounter = duelMock({ scoring: 'bonus', you: index('ember'), foe: index('squid'), seed: 3 });
  hasCounter.duel.aiPersona = 'reader';
  hasCounter.duel.aiRead = 100;
  hasCounter.duelState.history = ['fire'];
  M.duelPick(hasCounter, 0);
  assert.ok(hasCounter.duelState.picks[1] < M.SWITCH_BASE, 'active counter is used directly');
  const swapping = duelMock({ scoring: 'bonus', you: index('ember'), foe: index('bell'), seed: 3 });
  pinTeam(swapping, 1, [index('bell'), index('bigsquid')]);
  swapping.duel.aiPersona = 'reader';
  swapping.duel.aiRead = 100;
  swapping.duelState.history = ['fire'];
  M.duelPick(swapping, 0);
  assert.equal(swapping.duelState.picks[1], M.SWITCH_BASE + 1, 'switched to the bench counter');
  assert.equal(swapping.duelState.aiRead, true);
  const noCounter = duelMock({ scoring: 'bonus', you: index('ember'), foe: index('bell'), seed: 3 });
  pinTeam(noCounter, 1, [index('bell'), index('bell')]);
  noCounter.duel.aiPersona = 'reader';
  noCounter.duel.aiRead = 100;
  noCounter.duelState.history = ['fire'];
  M.duelPick(noCounter, 0);
  assert.ok(noCounter.duelState.picks[1] < M.SWITCH_BASE, 'no random switches');
});

test('duel AI keeps its pick inside its own option list for both engines', () => {
  for (const [duelKind, scoring] of [['wheel', 'power'], ['wheel', 'flat'], ['stances', 'power']]) {
    const state = duelMock({ duelKind, scoring, you: 10, foe: 12, seed: 7, aiRead: 100 });
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

test('sample teams build the party with retyped members', () => {
  const index = (name) => M.DATA.creatures.findIndex((entry) => entry.name === name);
  const state = M.newMock({
    mode: 'duel', scoring: 'bonus', matrixId: '5', seed: 5,
    presetYou: 'balanced', presetFoe: 'anchors',
  });
  M.startBattle(state);
  assert.deepEqual(state.duelState.partyIndex[0], [index('squid'), index('SkitterCrab'), index('hedge')]);
  assert.deepEqual(state.duelState.partyTypes[0], ['water', 'earth', 'plant']);
  assert.deepEqual(state.duelState.partyTypes[1], ['water', 'water', 'earth']);
  assert.equal(state.duelState.hp[0].length, 3);
  // Retyped member drives the matchup: OrbitRelic is spirit/elder raw, typed
  // water in the 'retyped' team, so fire loses to its creature type.
  const retyped = M.newMock({
    mode: 'duel', scoring: 'bonus', matrixId: '5', seed: 5, presetYou: 'retyped', presetFoe: 'balanced',
  });
  M.startBattle(retyped);
  M.pinTeam ? null : null;
  const orbit = M.DATA.creatures.findIndex((entry) => entry.name === 'OrbitRelic');
  assert.ok(retyped.duelState.partyIndex[0].includes(orbit));
  // Put OrbitRelic active and check the effective type list and fire matchup.
  retyped.duelState.activeSlot[0] = retyped.duelState.partyIndex[0].indexOf(orbit);
  assert.deepEqual(M.duelTypeList(retyped, 0), ['water']);
  assert.equal(M.duelValue(retyped, 'fire', 'water'), 0.5, 'fire loses to the retyped water');
  // Unknown preset ids fall back to a random roll.
  const fallback = M.newMock({ mode: 'duel', matrixId: '5', seed: 5, presetYou: 'nope' });
  M.startBattle(fallback);
  assert.equal(fallback.duelState.partyTypes[0], null);
  assert.equal(fallback.duelState.partyIndex[0].length, 3);
});

test('folded creature types drive badges and matchups in 5-type mode', () => {
  const index = (name) => M.DATA.creatures.findIndex((entry) => entry.name === name);
  const state = M.newMock({ mode: 'duel', scoring: 'bonus', matrixId: '5', seed: 5, teamSize: 1 });
  M.startBattle(state);
  // ScatterCrab (earth) active, foe raw wind creature: wind folds to water.
  state.duelState.partyIndex = [[index('ScatterCrab')], [index('SquibbleSnail')]];
  state.duelState.partyTypes = [null, null];
  state.duelState.hp = [[M.creature(index('ScatterCrab')).maxHp], [M.creature(index('SquibbleSnail')).maxHp]];
  state.duelState.activeSlot = [0, 0];
  assert.deepEqual(M.duelTypeList(state, 1), ['water'], 'wind folds to water for badges');
  assert.deepEqual(M.duelTypeList(state, 0), ['earth']);
  assert.equal(M.duelValue(state, 'water', 'earth'), 2, 'water beats earth in the 5-type web');
});

test('demo default table is 5 and the fold drives the default matchup layer', () => {
  const state = M.newMock({ mode: 'duel' });
  assert.equal(state.duel.matrixId, '5');
  M.startBattle(state);
  // wind folds to water, lightning folds to fire in the 5-type table.
  const wind = M.DATA.creatures.findIndex((entry) => entry.name === 'SquibbleSnail');
  state.duelState.partyIndex[0] = [wind];
  state.duelState.partyTypes[0] = null;
  state.duelState.hp[0] = [M.creature(wind).maxHp];
  state.duelState.activeSlot[0] = 0;
  assert.deepEqual(M.duelTypeList(state, 0), ['water'], 'wind folds to water by default');
});
