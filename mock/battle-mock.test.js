'use strict';

// Duel mock mechanics tests (CreatureGathererFX-4w89.6). Run:
//   node --test mock/battle-mock.test.js
//
// The mock is the 5-type RPS duel over the real roster. A creature's kit is its
// authored 8-type `moves` list folded onto the active type-count table; the
// shipping table (id "5") is the same 5-bit `attackMask` the device reads.
const test = require('node:test');
const assert = require('node:assert');
const M = require('./battle-mock.js');

const index = (name) => M.DATA.creatures.findIndex((entry) => entry.name === name);

// Active-slot HP accessor.
const hp = (state, side) => state.duelState.hp[side][state.duelState.activeSlot[side]];

// Pin a duel side to exact creatures so tests stay deterministic (random team
// rolling is covered by its own test).
function pinTeam(state, side, indices) {
  state.duelState.partyIndex[side] = [...indices];
  state.duelState.hp[side] = indices.map((i) => M.creature(i).maxHp);
  state.duelState.activeSlot[side] = 0;
}

function duelMock({ you, foe, teamSize = 1, matrixId = '5', ...options } = {}) {
  const state = M.newMock({ scoring: 'bonus', matrixId, teamSize, you, foe, ...options });
  M.startBattle(state);
  if (you != null) pinTeam(state, 0, [you]);
  if (foe != null) pinTeam(state, 1, [foe]);
  return state;
}

// Frozen 5-type duel fold (docs/moveset-type-mask-brief.md, data/duel.toml).
const DUEL_ORDER = ['spirit', 'water', 'earth', 'fire', 'plant'];
const DUEL_FOLD = {
  spirit: 'spirit', elder: 'spirit',
  water: 'water', wind: 'water',
  earth: 'earth',
  fire: 'fire', lightning: 'fire',
  plant: 'plant',
};

test('kits are the authored 8-type moves folded to the 5-bit attack mask', () => {
  for (const creature of M.DATA.creatures) {
    assert.ok(creature.moves.length >= 1, `${creature.name} has at least one attack type`);
    let mask = 0;
    for (const type of creature.moves) {
      assert.ok(type in DUEL_FOLD, `${creature.name}: ${type} folds to a duel type`);
      mask |= 1 << DUEL_ORDER.indexOf(DUEL_FOLD[type]);
    }
    assert.ok(mask > 0 && mask < 32, `${creature.name} mask in range (${mask})`);
    assert.equal(creature.attackMask, mask, `${creature.name} attackMask`);
  }
});

test('a duel side picks one option per mask bit in ascending duel-type order', () => {
  const state = duelMock({ you: index('cactus'), foe: index('squid'), seed: 1 });
  assert.deepEqual(M.duelOptions(state, 0).map((o) => o.type), ['spirit', 'earth', 'plant']);
  assert.deepEqual(M.duelOptions(state, 1).map((o) => o.type), ['water']);
  // The shipping table order is the mask bit order.
  const matrix = M.duelMatrix(state);
  assert.deepEqual(matrix.order, DUEL_ORDER);
});

test("duel uses each creature's own attack types and the table decides the clash", () => {
  const state = duelMock({ you: index('ember'), foe: index('hedge'), seed: 3 });
  const mine = M.duelMoves(state, 0);
  assert.deepEqual(mine.map((o) => o.type), ['fire']);
  const beforeYou = hp(state, 0), beforeFoe = hp(state, 1);
  // fire beats plant: bonus win (x1.5) vs the plant creature.
  state.duelState.picks = [0, 0];
  M.duelResolve(state);
  assert.equal(state.duelState.lines[0], 'BONUS WIN');
  assert.equal(beforeFoe - hp(state, 1), 21);
  assert.ok(beforeYou - hp(state, 0) < beforeFoe - hp(state, 1), 'winner deals more than chip');
  assert.equal(state.duelState.phase, 'result');
  state.duelState.timer = 0;
  M.duelUpdate(state, 2000);
  assert.equal(state.duelState.phase, 'choice');
  assert.equal(state.duelState.round, 2);
});

test('duel losses and trades resolve on the correct sides', () => {
  const state = duelMock({ you: index('hedge'), foe: index('ember'), seed: 3 });
  const beforeYou = hp(state, 0), beforeFoe = hp(state, 1);
  state.duelState.picks = [0, 0]; // plant vs fire: plant loses to fire
  M.duelResolve(state);
  assert.equal(state.duelState.lines[0], 'BONUS LOSE');
  assert.ok(beforeYou - hp(state, 0) > beforeFoe - hp(state, 1), 'loser takes the full hit');
  // Trade: same type on both sides, fixed 6 each.
  const same = duelMock({ you: index('ember'), foe: index('ember'), seed: 3 });
  const tradeYou = hp(same, 0), tradeFoe = hp(same, 1);
  same.duelState.picks = [0, 0];
  M.duelResolve(same);
  assert.equal(same.duelState.lines[0], 'TRADE');
  assert.equal(tradeYou - hp(same, 0), 6);
  assert.equal(tradeFoe - hp(same, 1), 6);
});

test('duel flow advances through step() edges and timers', () => {
  const state = M.newMock({ seed: 11 });
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
  const a = M.newMock({ seed: 21 });
  M.startBattle(a);
  assert.equal(a.duelState.partyIndex[0].length, 3);
  assert.equal(a.duelState.partyIndex[1].length, 3);
  for (const side of [0, 1]) {
    assert.equal(new Set(a.duelState.partyIndex[side]).size, 3, 'distinct members');
    assert.deepEqual(a.duelState.hp[side], a.duelState.partyIndex[side].map((i) => M.creature(i).maxHp));
  }
  const b = M.newMock({ seed: 21 });
  M.startBattle(b);
  assert.deepEqual(a.duelState.partyIndex, b.duelState.partyIndex, 'same seed, same teams');
});

test('stance duel: cycle order, options and beats for 3 and 4 stances', () => {
  const three = M.newMock({ duelKind: 'stances', stances: 3 });
  const four = M.newMock({ duelKind: 'stances', stances: 4 });
  M.startBattle(three);
  M.startBattle(four);
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
  const state = duelMock({ duelKind: 'stances', you: index('ember'), foe: index('hedge'), seed: 3 });
  assert.equal(M.duelKind(state), 'stances');
  const beforeYou = hp(state, 0), beforeFoe = hp(state, 1);
  state.duelState.picks = [0, 1]; // flame vs bloom: flame wins
  M.duelResolve(state);
  assert.equal(state.duelState.lines[0], 'CLASH WIN');
  assert.ok(beforeFoe - hp(state, 1) > beforeYou - hp(state, 0), 'winner deals more than chip');
  const tie = duelMock({ duelKind: 'stances', you: index('ember'), foe: index('hedge'), seed: 3 });
  const tieYou = hp(tie, 0), tieFoe = hp(tie, 1);
  tie.duelState.picks = [0, 0];
  M.duelResolve(tie);
  assert.equal(tie.duelState.lines[0], 'TIE');
  assert.ok(hp(tie, 0) < tieYou && hp(tie, 1) < tieFoe);
});

test('pure RPS scoring ignores the creature matchup, fixed numbers', () => {
  const state = duelMock({ scoring: 'flat', you: index('ember'), foe: index('hedge'), seed: 3 });
  const beforeYou = hp(state, 0), beforeFoe = hp(state, 1);
  state.duelState.picks = [0, 0]; // fire vs plant: fire wins
  M.duelResolve(state);
  assert.equal(state.duelState.lines[0], 'RPS WIN');
  assert.equal(beforeFoe - hp(state, 1), 14, 'flat win damage');
  assert.equal(beforeYou - hp(state, 0), 4, 'flat chip damage');
  // Same result for a different creature matchup: the pure spelling scales nothing.
  const other = duelMock({ scoring: 'flat', you: index('cactus'), foe: index('ember'), seed: 3 });
  const otherYou = hp(other, 0), otherFoe = hp(other, 1);
  other.duelState.picks = [0, 0]; // spirit vs fire: spirit wins
  M.duelResolve(other);
  assert.equal(other.duelState.lines[0], 'RPS WIN');
  assert.equal(otherFoe - hp(other, 1), 14);
  assert.equal(otherYou - hp(other, 0), 4);
});

test('bonus spelling scales hits by the creature matchup, penalty included', () => {
  const run = (scoring, foeIndex, pick, youIndex = index('ember')) => {
    const state = duelMock({ scoring, you: youIndex, foe: foeIndex, seed: 3 });
    const beforeFoe = hp(state, 1), beforeYou = hp(state, 0);
    state.duelState.picks = pick;
    M.duelResolve(state);
    return { line: state.duelState.lines[0], dealt: beforeFoe - hp(state, 1), taken: beforeYou - hp(state, 0) };
  };
  // fire beats the plant creature: bonus win = round(14 * 1.5) = 21, chip 4 * 0.5 = 2.
  const bonusHedge = run('bonus', index('hedge'), [0, 0]);
  assert.equal(bonusHedge.line, 'BONUS WIN');
  assert.equal(bonusHedge.dealt, 21);
  assert.equal(bonusHedge.taken, 2);
  // fire loses the clash and the matchup to the water creature: 21 against, chip 2.
  const weakLoss = run('bonus', index('squid'), [0, 0]);
  assert.equal(weakLoss.line, 'BONUS LOSE');
  assert.equal(weakLoss.dealt, 2);
  assert.equal(weakLoss.taken, 21);
  // Pure spelling stays 14 regardless, on the same matchup.
  const pureHedge = run('flat', index('hedge'), [0, 0]);
  assert.equal(pureHedge.line, 'RPS WIN');
  assert.equal(pureHedge.dealt, 14);
  // Trade scales both sides by their own matchup modifier: ember vs ember neutral.
  const trade = run('bonus', index('ember'), [0, 0], index('ember'));
  assert.equal(trade.line, 'TRADE');
  assert.equal(trade.dealt, 6);
  assert.equal(trade.taken, 6);
  // Helper reports the multiplier the cell markers use, penalty included.
  assert.equal(M.duelCreatureBonus(duelMock({ you: index('ember'), foe: index('hedge'), seed: 1 }), 0, 'fire'), 1.5);
  assert.equal(M.duelCreatureBonus(duelMock({ you: index('ember'), foe: index('squid'), seed: 1 }), 0, 'fire'), 0.5);
  assert.equal(M.duelCreatureBonus(duelMock({ you: index('ember'), foe: index('squid'), seed: 1 }), 0, 'water'), 1, 'mirror type is neutral');
});

test('bonus AI prefers the best matchup tier and never feeds weak picks when avoidable', () => {
  // AI cactus [spirit/earth/plant] vs player squid [water]: plant beats water
  // (bonus), spirit and earth lose to it (weak) -> always plant when not reading.
  for (let seed = 1; seed <= 6; seed++) {
    const state = duelMock({ you: index('squid'), foe: index('cactus'), seed });
    state.duel.aiPersona = 'reader';
    state.duel.aiRead = 0;
    M.duelPick(state, 0);
    const pick = state.duelState.picks[1];
    assert.equal(M.duelOptions(state, 1)[pick].type, 'plant', `seed ${seed}`);
  }
});

test('personas: frequency, anti-repeat and exploiter guess the player type', () => {
  // Player = ember (fire). AI = bigsquid (water beats fire).
  const build = (persona) => {
    const state = duelMock({ scoring: 'bonus', you: index('ember'), foe: index('bigsquid'), seed: 9 });
    state.duel.aiPersona = persona;
    state.duel.aiRead = 100;
    return state;
  };
  const frequency = build('frequency');
  frequency.duelState.history = ['fire', 'fire', 'plant'];
  M.duelPick(frequency, 0);
  assert.equal(M.duelOptions(frequency, 1)[frequency.duelState.picks[1]].type, 'water');
  assert.equal(frequency.duelState.aiRead, true);
  const anti = build('anti-repeat');
  anti.duelState.history = ['fire', 'plant'];
  M.duelPick(anti, 0);
  assert.equal(M.duelOptions(anti, 1)[anti.duelState.picks[1]].type, 'water');
  const exploiter = build('exploiter');
  exploiter.duelState.history = ['plant', 'plant'];
  exploiter.duelState.postReadCounts = { fire: 3 };
  M.duelPick(exploiter, 0);
  assert.equal(M.duelOptions(exploiter, 1)[exploiter.duelState.picks[1]].type, 'water');
  const reader = build('reader');
  reader.duelState.history = ['fire'];
  M.duelPick(reader, 0);
  assert.equal(M.duelOptions(reader, 1)[reader.duelState.picks[1]].type, 'water');
  const random = build('random');
  random.duelState.history = ['fire', 'fire', 'fire', 'fire'];
  M.duelPick(random, 0);
  assert.equal(random.duelState.aiRead, false);
});

test('fallback when no counter exists is uniform random, not a boost preference', () => {
  // player ember (fire); AI ember (fire, cannot beat fire): no read counter.
  const state = duelMock({ scoring: 'bonus', you: index('ember'), foe: index('ember'), seed: 4 });
  state.duel.aiRead = 100;
  state.duel.aiPersona = 'reader';
  state.duelState.history = ['fire'];
  M.duelPick(state, 0);
  assert.equal(state.duelState.aiRead, false, 'no counter -> no read flag');
  assert.equal(state.duelState.picks[1], 0, 'single option picked');
  // Player history recording feeds the models.
  const recorded = duelMock({ scoring: 'bonus', you: index('ember'), foe: index('ember'), seed: 2 });
  recorded.duelState.readFiredPrev = true;
  M.recordPlayerPick(recorded, 'fire');
  recorded.duelState.thisReadFired = true;
  M.recordPlayerPick(recorded, 'plant');
  assert.deepEqual(recorded.duelState.history, ['fire', 'plant']);
  assert.deepEqual(recorded.duelState.postReadCounts, { fire: 1 });
  assert.equal(recorded.duelState.readFiredPrev, true);
});

test('switching forfeits the round: the non-switcher lands a default win', () => {
  const hedge = index('hedge'), ember = index('ember'), cloud = index('cloud');
  const make = () => {
    const state = duelMock({ scoring: 'bonus', you: hedge, foe: ember, seed: 3 });
    pinTeam(state, 0, [hedge, cloud]);
    pinTeam(state, 1, [ember, index('squid')]);
    return state;
  };
  const attack = make();
  attack.duelState.picks = [0, 0];
  M.duelResolve(attack);
  const clashLines = attack.duelState.lines[0];
  // Player switches (-1 = next living member) while the foe attacks: default win.
  const swap = make();
  const incoming = M.creature(cloud);
  swap.duelState.picks = [-1, 0];
  M.duelResolve(swap);
  assert.equal(swap.duelState.lines[0], 'DEFAULT WIN');
  assert.equal(swap.duelState.activeSlot[0], 1, 'player swap applied');
  // cloud folds to water; fire loses to water -> modifier 0.5 -> round(14 * 0.5) = 7.
  assert.equal(hp(swap, 0), incoming.maxHp - 7, 'incoming takes the default win');
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
  const hasCounter = duelMock({ scoring: 'bonus', you: index('ember'), foe: index('squid'), seed: 3 });
  hasCounter.duel.aiPersona = 'reader';
  hasCounter.duel.aiRead = 100;
  hasCounter.duelState.history = ['fire'];
  M.duelPick(hasCounter, 0);
  assert.ok(hasCounter.duelState.picks[1] < M.SWITCH_BASE, 'active counter is used directly');
  // AI ember (fire, no fire counter) with a water bench: switch to the counter.
  const swapping = duelMock({ scoring: 'bonus', you: index('ember'), foe: index('ember'), seed: 3 });
  pinTeam(swapping, 1, [index('ember'), index('bigsquid')]);
  swapping.duel.aiPersona = 'reader';
  swapping.duel.aiRead = 100;
  swapping.duelState.history = ['fire'];
  M.duelPick(swapping, 0);
  assert.equal(swapping.duelState.picks[1], M.SWITCH_BASE + 1, 'switched to the bench counter');
  assert.equal(swapping.duelState.aiRead, true);
  const noCounter = duelMock({ scoring: 'bonus', you: index('ember'), foe: index('ember'), seed: 3 });
  pinTeam(noCounter, 1, [index('ember'), index('ember')]);
  noCounter.duel.aiPersona = 'reader';
  noCounter.duel.aiRead = 100;
  noCounter.duelState.history = ['fire'];
  M.duelPick(noCounter, 0);
  assert.ok(noCounter.duelState.picks[1] < M.SWITCH_BASE, 'no random switches');
});

test('a zero-option kit yields WAIT and never throws (kernel unusable-kit parity)', () => {
  const ember = index('ember');
  const savedMoves = M.DATA.creatures[ember].moves;
  // An unusable kit: no authored move folds onto the active table.
  M.DATA.creatures[ember].moves = [];
  try {
    // aiDuelPick returns the WAIT pick for a zero-option kit (no attack, no rng).
    const ai = duelMock({ you: index('squid'), foe: ember, seed: 1 });
    M.duelPick(ai, 0);
    assert.equal(ai.duelState.picks[1], M.WAIT_PICK);
    assert.equal(M.isWaitPick(ai.duelState.picks[1]), true);
    assert.equal(M.isSwitchPick(ai.duelState.picks[1]), false);
    // Attacker default-wins the waiting side; no throw, attacker untouched.
    const beforeYou = hp(ai, 0), beforeFoe = hp(ai, 1);
    M.duelResolve(ai);
    assert.equal(ai.duelState.lines[0], 'DEFAULT WIN');
    assert.ok(hp(ai, 1) < beforeFoe, 'waiting side takes the default win');
    assert.equal(hp(ai, 0), beforeYou, 'attacker untouched');
    // Both sides waiting: no attack, no damage.
    const both = duelMock({ you: ember, foe: ember, seed: 1 });
    both.duelState.picks = [M.WAIT_PICK, M.WAIT_PICK];
    const preHp = [hp(both, 0), hp(both, 1)];
    M.duelResolve(both);
    assert.equal(both.duelState.lines[0], 'WAIT');
    assert.deepEqual([hp(both, 0), hp(both, 1)], preHp, 'no damage on a mutual wait');
    // A zero-kit player with no bench can still commit WAIT through step().
    const solo = duelMock({ you: ember, foe: index('squid'), seed: 2, teamSize: 1 });
    assert.equal(M.duelOptions(solo, 0).length, 0);
    M.step(solo, { edges: ['a'], dtMs: 0 });
    assert.equal(solo.duelState.picks[0], M.WAIT_PICK, 'player WAIT committed');
  } finally {
    M.DATA.creatures[ember].moves = savedMoves;
  }
});

test('duel AI keeps its pick inside its own option list for both engines', () => {
  for (const [duelKind, scoring] of [['wheel', 'flat'], ['wheel', 'bonus'], ['stances', 'bonus']]) {
    const state = duelMock({ duelKind, scoring, you: index('ember'), foe: index('hedge'), seed: 7, aiRead: 100 });
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

test('every attack type in every kit has a drawable icon', () => {
  for (const creature of M.DATA.creatures) {
    for (const type of creature.moves) {
      const bits = M.ICONS[type] || M.MERGED_ICONS[type];
      assert.ok(bits && bits.length === 8, `icon for type ${type}`);
    }
  }
});

test('setup edges cycle creatures and A starts the fight', () => {
  const state = M.newMock({ you: 0, foe: 0 });
  M.step(state, { edges: ['right'], dtMs: 0 });
  assert.equal(state.youIndex, 1);
  M.step(state, { edges: ['up'], dtMs: 0 });
  assert.equal(state.foeIndex, M.DATA.creatures.length - 1);
  M.step(state, { edges: ['a'], dtMs: 0 });
  assert.equal(state.scene, 'battle');
  assert.ok(state.duelState);
});

test('sample teams build the party with retyped members', () => {
  const state = M.newMock({
    scoring: 'bonus', matrixId: '5', seed: 5,
    presetYou: 'balanced', presetFoe: 'anchors',
  });
  M.startBattle(state);
  assert.deepEqual(state.duelState.partyIndex[0],
    [index('squid'), index('SkitterCrab'), index('hedge')]);
  assert.deepEqual(state.duelState.partyTypes[0], ['water', 'earth', 'plant']);
  assert.deepEqual(state.duelState.partyTypes[1], ['water', 'water', 'earth']);
  assert.equal(state.duelState.hp[0].length, 3);
  // Retyped member drives the matchup: OrbitRelic is elder/spirit raw, typed
  // water in the 'retyped' team, so fire loses to its creature type.
  const retyped = M.newMock({
    scoring: 'bonus', matrixId: '5', seed: 5, presetYou: 'retyped', presetFoe: 'balanced',
  });
  M.startBattle(retyped);
  const orbit = index('OrbitRelic');
  assert.ok(retyped.duelState.partyIndex[0].includes(orbit));
  retyped.duelState.activeSlot[0] = retyped.duelState.partyIndex[0].indexOf(orbit);
  assert.deepEqual(M.duelTypeList(retyped, 0), ['water']);
  assert.equal(M.duelValue(retyped, 'fire', 'water'), 0.5, 'fire loses to the retyped water');
  // Unknown preset ids fall back to a random roll.
  const fallback = M.newMock({ matrixId: '5', seed: 5, presetYou: 'nope' });
  M.startBattle(fallback);
  assert.equal(fallback.duelState.partyTypes[0], null);
  assert.equal(fallback.duelState.partyIndex[0].length, 3);
});

test('folded creature types drive badges and matchups in 5-type mode', () => {
  const state = M.newMock({ scoring: 'bonus', matrixId: '5', seed: 5, teamSize: 1 });
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
  const state = M.newMock({ seed: 5 });
  assert.equal(state.duel.matrixId, '5');
  assert.equal(state.duel.scoring, 'bonus');
  M.startBattle(state);
  const wind = index('SquibbleSnail');
  state.duelState.partyIndex[0] = [wind];
  state.duelState.partyTypes[0] = null;
  state.duelState.hp[0] = [M.creature(wind).maxHp];
  state.duelState.activeSlot[0] = 0;
  assert.deepEqual(M.duelTypeList(state, 0), ['water'], 'wind folds to water by default');
});

test('the 8-type analysis web stays a non-trivial neutral-rich graph', () => {
  const { order, chart, edges, cycles } = M.DATA.wheel;
  assert.equal(order.length, 8);
  assert.ok(edges.length >= 20 && edges.length < 56);
  assert.ok(cycles.length >= 4);
  for (const [from, to] of edges) {
    assert.equal(chart[order.indexOf(from)][order.indexOf(to)], 2, `${from}>${to} chart`);
    assert.equal(chart[order.indexOf(to)][order.indexOf(from)], 0.5, `${to}<${from} chart`);
  }
  assert.ok(chart.some((row) => row.includes(1)), 'some pairs stay neutral');
});
