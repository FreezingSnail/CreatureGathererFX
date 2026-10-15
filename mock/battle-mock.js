'use strict';

/*
 * Creature Gatherer battle mock — 128x64 playable spike over the real roster.
 *
 * Goal: test how battle designs communicate on the actual display, not art.
 *   A "current"  the shipping 8-type dual-type move menu.
 *   B "trim"     4-type single-type cycle + per-move effectiveness arrows +
 *                optional SMT-style press-turn (super hit acts again).
 *   C "stances"  simultaneous hidden picks from the 4-type merged cycle
 *                (flame > bloom > tide > stone), affinity when the stance is
 *                the creature's own, ties trade chip.
 *   D "wheel"    simultaneous hidden picks from each creature's own moves,
 *                resolved by the 8-type wheel (docs/battle-wheel-matrix.md):
 *                win = full hit, lose = blocked + chip, other pairs trade.
 *
 * Keys (browser):
 *   Arrows/WASD  move cursor / pick creature in setup
 *   Z or Space   A: confirm, advance feedback, start fight
 *   X            B: back to setup
 *   1 / 2 / 3    switch design (current / trim / duel wheel)
 * Page controls tune: trim type mode, arrows, press-turn, foe HP readout,
 * stance count for the stance duel, and how often the AI reads your last pick.
 *
 * Fidelity: damage, speed order, move-use limits and STAB follow the production
 * C++ (Damage.cpp, Type.hpp, MoveUses.hpp) ported to integer JS with stat stages
 * at zero. The AI is a simplified damage-scoring stand-in for the tactical AI.
 * Not simulated: statuses, stages, effects, accuracy, party/switching, escape,
 * gather, animations. Status/utility moves (type status/none) are excluded from
 * every pick list in this spike. Feedback text replaces the staged presenter; HP
 * updates immediately. Sprite layout matches BattleLayout.hpp (foe 0,0; you
 * 80,0; menu y=48..63; HP column x=48..79; 48x48 front/back frames).
 */

const DATA = (typeof module !== 'undefined' && module.exports && typeof require === 'function')
  ? require('./data.js')
  : (typeof window !== 'undefined' ? window.CGFX_SPIKE_DATA : null);
const MATRIX_DATA = (typeof module !== 'undefined' && module.exports && typeof require === 'function')
  ? require('./matrices.js')
  : (typeof window !== 'undefined' ? window.CGFX_MATRICES : null);

const W = 128, H = 64;
const SPRITE_BASE = '../docs/assets/battle-sprites48/';

/* ---------- seeded rng (xorshift32) so runs are comparable ---------- */
function makeRng(seedValue) {
  let s = seedValue >>> 0 || 1;
  return {
    next() { s ^= s << 13; s >>>= 0; s ^= s >>> 17; s ^= s << 5; s >>>= 0; return s / 4294967296; },
    int(n) { return Math.floor(this.next() * n); },
  };
}

// ---------- authored 8x8 type icons (shared with matrix.js via mock/icons.js) ----------
const ICON_DATA = (typeof module !== 'undefined' && module.exports && typeof require === 'function')
  ? require('./icons.js')
  : (typeof window !== 'undefined' ? window.CGFX_ICONS : null);
const ICONS = (ICON_DATA && ICON_DATA.ICONS) || {};
const MERGED_ICONS = (ICON_DATA && ICON_DATA.MERGED_ICONS) || {};

/* ---------- 5x6 symbols matching the embedded font atlas ---------- */
const SYMBOLS = {
  ' ': ['.....', '.....', '.....', '.....', '.....', '.....'],
  '/': ['....#', '...#.', '..#..', '.#...', '#....', '.....'],
  '-': ['.....', '.....', '.....', '#####', '.....', '.....'],
  '*': ['.....', '#...#', '.###.', '..#..', '.###.', '#...#'],
  '>': ['####.', '....#', '...#.', '..#..', '.#...', '#....'],
  up: ['..#..', '.###.', '#####', '.....', '.....', '.....'],
  down: ['.....', '.....', '#####', '.###.', '..#..', '.....'],
  flat: ['.....', '.....', '..#..', '.....', '.....', '.....'],
  no: ['#...#', '.#.#.', '..#..', '.#.#.', '#...#', '.....'],
};

function glyphRows(char) {
  const custom = SYMBOLS[char];
  if (custom) return custom.map((row) => row.split('').map((c) => c === '#'));
  const code = char.charCodeAt(0);
  if (code < DATA.font.firstChar || code >= DATA.font.firstChar + DATA.font.glyphs.length) return null;
  const bits = DATA.font.glyphs[code - DATA.font.firstChar];
  const rows = [];
  for (let y = 0; y < 6; y++) {
    const row = [];
    for (let x = 0; x < 5; x++) row.push(bits[y * 5 + x] === '1');
    rows.push(row);
  }
  return rows;
}

const typeIcon = (name) => ICONS[name] || MERGED_ICONS[name];
const textWidth = (str) => str.length * 5;
const num = (value) => String(value);

// This spike excludes status/utility moves: they are not battlefield picks
// under any of the duel spellings (and would only be menu noise in A/B).
const isSpikeMove = (move) => move != null && move.type !== 'status' && move.type !== 'none';

/* ---------- production rules ported (src/engine/battle + src/lib/Type) ---------- */
// Modifier enum: 0 None, 1 Quarter, 2 Half, 3 Same, 4 Double, 5 Quadruple.
function modifierFor(multiplier) {
  return multiplier === 0 ? 0 : multiplier === 0.5 ? 2 : multiplier === 1 ? 3 : 4;
}
function combineModifier(a, b) {
  if (a === 0 || b === 0) return 0;
  let exponent = a + b - 6;
  if (exponent < -2) exponent = -2;
  if (exponent > 2) exponent = 2;
  return exponent + 3;
}
function applyDamageModifier(value, modifier) {
  const shift = modifier - 3;
  return shift < 0 ? value >> -shift : value << shift;
}
function arrowFor(modifier) {
  return modifier === 0 ? 'no' : modifier >= 4 ? 'up' : modifier <= 2 ? 'down' : 'flat';
}
function chartFor(mode) { return mode === '4single' ? DATA.merged : { order: DATA.typeOrder, chart: DATA.chart }; }
function typesOf(creature, mode) { return mode === '4single' ? creature.merged : creature.types; }
function moveTypeFor(mode, moveType) {
  if (mode !== '4single') return moveType;
  return DATA.merged.map[moveType] || moveType;
}
function lookupModifier(chart, attackType, defendType) {
  if (defendType === undefined) return 3;
  if (attackType === undefined) return 0;
  const row = chart.order.indexOf(attackType), col = chart.order.indexOf(defendType);
  if (row < 0 || col < 0) return 3;
  return modifierFor(chart.chart[row][col]);
}
function attackModifier(attacker, defender, moveType, mode) {
  const chart = chartFor(mode);
  const attackType = moveTypeFor(mode, moveType);
  let modifier = 3;
  for (const type of typesOf(defender, mode)) modifier = combineModifier(modifier, lookupModifier(chart, attackType, type));
  if (typesOf(attacker, mode).includes(attackType)) modifier = combineModifier(modifier, 4);
  return modifier;
}
function computeDamage(attacker, defender, move, mode) {
  if (move.power === 0) return 0;
  const attackValue = move.physical ? attacker.stats.attack : attacker.stats.spcAtk;
  const defenseValue = move.physical ? defender.stats.defense : defender.stats.spcDef;
  const divisor = Math.max(1, Math.floor(defenseValue / 2));
  const base = Math.floor(Math.floor(move.power * attackValue / divisor) / 2);
  const modifier = attackModifier(attacker, defender, move.type, mode);
  if (modifier === 0) return 0;
  const modified = applyDamageModifier(base, modifier);
  return modified === 0 ? 1 : Math.min(255, modified);
}

/* ---------- state ---------- */
function newMock(options = {}) {
  const state = {
    mode: options.mode || 'current', // current | trim | duel
    lockMode: options.lockMode === true, // variant windows pin their design
    trim: {
      typeMode: options.typeMode || '4single',
      arrows: options.arrows !== false,
      pressTurn: options.pressTurn !== false,
      foeNumbers: options.foeNumbers !== false,
    },
    duel: {
      kind: options.duelKind || 'wheel', // wheel | stances
      scoring: options.scoring || 'power', // power | flat (pure) | bonus (recommended)
      matrixId: options.matrixId || '8', // type-count table: 8 | 6 | 5 | 4
      teamSize: options.teamSize || 3, // 3v3 by default; 1 = benchless spike mode
      aiPersona: options.aiPersona || 'reader',
      stances: options.stances || 3,
      aiRead: options.aiRead ?? 45,
      explain: options.explain !== false,
    },
    scene: 'setup', // setup | battle | end
    youIndex: options.you ?? 10,
    foeIndex: options.foe ?? 0,
    battle: null,
    duelState: null,
    endText: '',
    seed: options.seed ?? ((Date.now() & 0xffff) || 1),
    rng: null,
    loadImage: options.loadImage || null,
    images: new Map(),
  };
  state.rng = makeRng(state.seed);
  return state;
}

const creature = (index) => DATA.creatures[index];
const moveOf = (creature_, slot) => (creature_.moves[slot] == null ? null : DATA.moves[creature_.moves[slot]]);
const activeMode = (state) => (state.mode === 'trim' ? state.trim.typeMode : '8dual');

function newSeed(state) {
  state.seed = (state.seed * 48271 + 1) & 0xffff;
  state.rng = makeRng(state.seed);
  if (state.scene === 'battle') startBattle(state);
}

/* ---------- battle construction ---------- */
function startBattle(state) {
  state.rng = makeRng(state.seed);
  const you = creature(state.youIndex), foe = creature(state.foeIndex);
  if (state.mode === 'duel') {
    // Teams are rolled from the battle seed: 3v3 by default, benchless at 1.
    const youParty = rollTeam(state, state.duel.teamSize);
    const foeParty = rollTeam(state, state.duel.teamSize);
    state.duelState = {
      partyIndex: [youParty, foeParty],
      activeSlot: [0, 0],
      hp: [
        youParty.map((index) => creature(index).maxHp),
        foeParty.map((index) => creature(index).maxHp),
      ],
      picks: [null, null],
      last: [null, null],
      history: [],
      postReadCounts: {},
      thisReadFired: false,
      readFiredPrev: false,
      round: 1,
      cursor: 0,
      phase: 'choice', // choice | reveal | result
      timer: 0,
      lines: ['', ''],
      aiRead: false,
    };
  } else {
    state.battle = {
      you, foe,
      hp: [you.maxHp, foe.maxHp],
      uses: [
        you.moves.map((id) => (id == null ? 0 : DATA.moves[id].limit)),
        foe.moves.map((id) => (id == null ? 0 : DATA.moves[id].limit)),
      ],
      turn: 1,
      phase: 'choice', // choice | feedback
      cursor: 0,
      queue: [],
      step: -1,
      lines: ['', ''],
      arrow: null,
      flash: '',
    };
    state.battle.cursor = firstUsable(state.battle);
  }
  state.scene = 'battle';
}

/* ---------- moves-mode turn resolution ---------- */
function usableSlots(state, side) {
  const creature_ = side === 0 ? state.battle.you : state.battle.foe;
  const slots = [];
  for (let slot = 0; slot < 4; slot++) {
    if (!isSpikeMove(moveOf(creature_, slot))) continue;
    if (state.battle.uses[side][slot] > 0) slots.push(slot);
  }
  return slots;
}
function firstUsable(battleState) {
  const slots = [];
  for (let slot = 0; slot < 4; slot++) {
    const move = moveOf(battleState.you, slot);
    if (!isSpikeMove(move)) continue;
    if (battleState.uses[0][slot] > 0) slots.push(slot);
  }
  return slots.length ? slots[0] : 255;
}
function aiPick(state, side) {
  const battleState = state.battle;
  const attacker = side === 0 ? battleState.you : battleState.foe;
  const defender = side === 0 ? battleState.foe : battleState.you;
  const slots = usableSlots(state, side);
  if (!slots.length) return null;
  let best = null, bestScore = -1;
  for (const slot of slots) {
    const move = moveOf(attacker, slot);
    const damage = computeDamage(attacker, defender, move, activeMode(state));
    const utility = move.power === 0 && move.effect !== 'NONE' ? 1 : 0;
    const score = damage * 4 + utility * 3 + state.rng.next();
    if (score > bestScore) { bestScore = score; best = slot; }
  }
  return best;
}

function planTurn(state, passOnly) {
  const battleState = state.battle;
  const actions = [{ side: 0, slot: passOnly ? null : battleState.cursor }];
  actions.push({ side: 1, slot: aiPick(state, 1) });
  const speed = (side) => (side === 0 ? battleState.you.stats.speed : battleState.foe.stats.speed);
  if (speed(1) > speed(0)) actions.reverse(); // player wins speed ties (firstMover contract)

  const queue = [];
  const extras = [false, false];
  for (const action of actions) {
    queue.push(action);
    if (state.mode === 'trim' && state.trim.pressTurn && action.slot != null) {
      const attacker = action.side === 0 ? battleState.you : battleState.foe;
      const defender = action.side === 0 ? battleState.foe : battleState.you;
      const move = moveOf(attacker, action.slot);
      if (battleState.uses[action.side][action.slot] > 0
          && attackModifier(attacker, defender, move.type, activeMode(state)) >= 4
          && !extras[action.side]) {
        extras[action.side] = true;
        queue.push({ side: action.side, slot: action.slot, extra: true });
      }
    }
  }
  battleState.queue = queue;
  battleState.step = -1;
  battleState.phase = 'feedback';
  advanceStep(state);
}

function advanceStep(state) {
  const battleState = state.battle;
  battleState.step += 1;
  if (battleState.step >= battleState.queue.length) {
    if (battleState.hp[0] <= 0 || battleState.hp[1] <= 0) {
      state.endText = battleState.hp[0] <= 0 && battleState.hp[1] <= 0 ? 'DRAW'
        : battleState.hp[1] <= 0 ? 'WIN' : 'LOSE';
      state.scene = 'end';
      return;
    }
    battleState.turn += 1;
    battleState.phase = 'choice';
    battleState.lines = ['', ''];
    battleState.arrow = null;
    battleState.flash = '';
    battleState.cursor = firstUsable(battleState);
    return;
  }
  const action = battleState.queue[battleState.step];
  const attacker = action.side === 0 ? battleState.you : battleState.foe;
  const defender = action.side === 0 ? battleState.foe : battleState.you;
  const defenderIndex = action.side === 0 ? 1 : 0;

  battleState.arrow = null;
  // A knockout cancels that side's remaining scheduled actions (no replacements in 1v1).
  if (battleState.hp[action.side] <= 0) {
    battleState.lines = [attacker.name, 'down, no action'];
    battleState.queue = battleState.queue.slice(0, battleState.step + 1);
    return;
  }
  if (action.slot == null) {
    battleState.lines = [attacker.name, 'cannot act'];
    return;
  }
  const move = moveOf(attacker, action.slot);
  if (battleState.uses[action.side][action.slot] === 0) {
    battleState.lines = [attacker.name, 'out of power'];
    return;
  }
  const damage = computeDamage(attacker, defender, move, activeMode(state));
  const modifier = attackModifier(attacker, defender, move.type, activeMode(state));
  battleState.uses[action.side][action.slot] -= 1;
  battleState.hp[defenderIndex] = Math.max(0, battleState.hp[defenderIndex] - damage);
  const suffix = damage > 0 ? `-${damage}` : 'no effect';
  battleState.arrow = state.mode === 'trim' && state.trim.arrows ? arrowFor(modifier) : null;
  battleState.lines = [attacker.name, `${move.name} ${suffix}`];
  battleState.flash = action.extra ? 'EXTRA' : '';
}

function cycleCursor(state, direction) {
  const slots = usableSlots(state, 0);
  if (!slots.length) return;
  const current = slots.indexOf(state.battle.cursor);
  state.battle.cursor = slots[(current < 0 ? 0 : current + direction + slots.length) % slots.length];
}

/* ---------- duel: hidden simultaneous picks ----------
 * Two engines share the flow:
 *   stances: both pick one of the 3/4 merged-cycle stances (flame/bloom/tide/stone).
 *   wheel:   both pick one of their own creature's moves, resolved by the 8-type wheel. */
const duelKind = (state) => state.duel.kind;
const stanceSet = (state) => DATA.merged.order.slice(0, state.duel.stances);
function stanceBeats(order, stance, other) {
  const step = (order.indexOf(other) - order.indexOf(stance) + order.length) % order.length;
  return step === 1;
}
// Duel parties: a random 3-creature team per side (rolled from the battle seed
// at start) plus a benchless spike mode. Switching forfeits the round: the side
// that did not switch lands a default win on the incoming creature.
const benchIndexFor = (index) => (index + 1) % DATA.creatures.length;
const SWITCH_BASE = 100; // pick encoding: >= 100 means "switch to party slot pick-100"
const isSwitchPick = (pick) => pick === -1 || pick >= SWITCH_BASE;

function nextLivingSlot(state, side, excludeSlot) {
  const duelState = state.duelState;
  for (let slot = 0; slot < duelState.hp[side].length; slot++) {
    if (slot === excludeSlot) continue;
    if (duelState.hp[side][slot] > 0) return slot;
  }
  return -1;
}

function livingBenchSlots(state, side) {
  const duelState = state.duelState;
  const slots = [];
  for (let slot = 0; slot < duelState.hp[side].length; slot++) {
    if (slot === duelState.activeSlot[side]) continue;
    if (duelState.hp[side][slot] > 0) slots.push(slot);
  }
  return slots;
}

function switchTargetSlot(state, side, pick) {
  if (pick >= SWITCH_BASE) return pick - SWITCH_BASE;
  return nextLivingSlot(state, side, state.duelState.activeSlot[side]);
}

function rollTeam(state, count) {
  const picks = [];
  while (picks.length < Math.min(count, DATA.creatures.length)) {
    const index = state.rng.int(DATA.creatures.length);
    if (!picks.includes(index)) picks.push(index);
  }
  return picks;
}

const duelCreature = (state, side) => {
  const party = state.duelState.partyIndex[side];
  return creature(party[state.duelState.activeSlot[side]]);
};
const duelBench = (state, side) => {
  const slot = livingBenchSlots(state, side)[0];
  return slot === undefined ? null : creature(state.duelState.partyIndex[side][slot]);
};
const sideAlive = (state, side) => state.duelState.hp[side].some((hp) => hp > 0);

function duelMoves(state, side) {
  const creature_ = duelCreature(state, side);
  return creature_.moves
    .filter((id) => id != null)
    .map((id) => DATA.moves[id])
    .filter((move) => isSpikeMove(move));
}
function wheelValue(moveType, defendType) {
  // 2 = beats, 0.5 = loses to, 1 = neutral (8-type wheel from data.js).
  const row = DATA.wheel.order.indexOf(moveType);
  const col = DATA.wheel.order.indexOf(defendType);
  if (row < 0 || col < 0) return 1;
  return DATA.wheel.chart[row][col];
}
const wheelBeats = (a, b) => wheelValue(a, b) === 2;

// Matrix-aware lookup for the duel: state.duel.matrixId selects one of the
// type-count spike tables (8/6/5/4), folding raw type names through its map.
function duelMatrix(state) {
  const id = state.duel.matrixId || '8';
  if (id !== '8' && MATRIX_DATA && MATRIX_DATA.matrices) {
    const entry = MATRIX_DATA.matrices.find((matrix) => matrix.id === id);
    if (entry) return entry;
  }
  return { order: DATA.wheel.order, chart: DATA.wheel.chart, map: null };
}
function foldType(matrix, type) {
  if (!matrix.map) return type;
  return matrix.map[type] || type;
}
function duelValue(state, moveType, defendType) {
  const matrix = duelMatrix(state);
  const row = matrix.order.indexOf(foldType(matrix, moveType));
  const col = matrix.order.indexOf(foldType(matrix, defendType));
  if (row < 0 || col < 0) return 1;
  return matrix.chart[row][col];
}
const duelBeats = (state, a, b) => duelValue(state, a, b) === 2;
function hasAffinity(state, side, move) {
  const creature_ = duelCreature(state, side);
  return creature_.types.includes(move.type);
}
// Options a side can pick this round, in display order.
function duelOptions(state, side) {
  if (duelKind(state) === 'stances') return stanceSet(state).map((type) => ({ type }));
  return duelMoves(state, side);
}
function duelOptionAffinity(state, side, option) {
  const creature_ = duelCreature(state, side);
  return duelKind(state) === 'stances'
    ? creature_.merged[0] === option.type
    : creature_.types.includes(option.type);
}
// AI personas: trainer groups reuse these. `memory` picks how the AI guesses
// the player's hidden type; null readRate means the window's rate slider.
// exploiter is for challenge areas only; random never reads.
const DUEL_PERSONAS = {
  random: { readRate: 0, memory: 'none' },
  reader: { readRate: null, memory: 'last' },
  frequency: { readRate: null, memory: 'frequency' },
  'anti-repeat': { readRate: null, memory: 'antiRepeat' },
  exploiter: { readRate: null, memory: 'exploiter' },
};
const duelPersona = (state) => DUEL_PERSONAS[state.duel.aiPersona] || DUEL_PERSONAS.reader;

function modalOf(counts, rng) {
  let best = [], bestCount = -1;
  for (const [type, count] of Object.entries(counts)) {
    if (count > bestCount) { best = [type]; bestCount = count; }
    else if (count === bestCount) best.push(type);
  }
  return best.length ? best[rng.int(best.length)] : null;
}

function predictPlayerType(state) {
  const persona = duelPersona(state);
  const duelState = state.duelState;
  const history = duelState.history || [];
  if (!history.length) return null;
  if (persona.memory === 'last') return history[history.length - 1];
  if (persona.memory === 'exploiter') {
    const post = duelState.postReadCounts || {};
    const samples = Object.values(post).reduce((sum, count) => sum + count, 0);
    if (samples >= 2) return modalOf(post, state.rng);
  }
  const counts = {};
  history.forEach((type, index) => {
    const discount = persona.memory === 'antiRepeat' && index === history.length - 1;
    counts[type] = (counts[type] || 0) + (discount ? 0 : 1);
  });
  if (Object.values(counts).every((count) => count === 0)) return history[history.length - 1];
  return modalOf(counts, state.rng);
}

// Option indexes whose type beats the guessed type (kind-aware relation).
function counterOptions(state, targetType) {
  if (duelKind(state) === 'stances') {
    const order = stanceSet(state);
    return order
      .map((type, index) => ({ type, index }))
      .filter((entry) => stanceBeats(order, entry.type, targetType))
      .map((entry) => entry.index);
  }
  return duelOptions(state, 1)
    .map((option, index) => ({ option, index }))
    .filter((entry) => duelBeats(state, entry.option.type, targetType))
    .map((entry) => entry.index);
}

function aiDuelPick(state) {
  const duelState = state.duelState;
  duelState.aiRead = false;
  duelState.thisReadFired = false;
  const persona = duelPersona(state);
  const options = duelOptions(state, 1);
  const rate = persona.readRate != null ? persona.readRate : state.duel.aiRead;
  if (persona.memory === 'none' || state.rng.next() * 100 >= rate) {
    // No read this round: attack (never a random swap), preferring the best
    // creature-type matchup tier so the AI does not feed weak picks.
    if (state.duel.scoring === 'bonus' && duelKind(state) === 'wheel') {
      const scored = options.map((option, index) => ({ option, index, modifier: duelCreatureBonus(state, 1, option.type) }));
      const bestTier = Math.max(...scored.map((entry) => entry.modifier));
      const pool = scored.filter((entry) => entry.modifier === bestTier);
      return pool[state.rng.int(pool.length)].index;
    }
    return state.rng.int(options.length);
  }
  const guessed = predictPlayerType(state);
  const counters = guessed ? counterOptions(state, guessed) : [];
  if (counters.length) {
    duelState.aiRead = true;
    duelState.thisReadFired = true;
    return counters[state.rng.int(counters.length)];
  }
  // No counter in the active kit: switch to a living bench member that holds
  // one (switching forfeits the round by default-win rule, so it costs).
  if (guessed && duelKind(state) === 'wheel') {
    const slot = benchCounterSlot(state, guessed);
    if (slot >= 0) {
      duelState.aiRead = true;
      duelState.thisReadFired = true;
      return SWITCH_BASE + slot;
    }
  }
  return state.rng.int(options.length); // fallback: uniform, no boost preference
}

// First living bench slot (AI side) whose moves beat the guessed type; -1 if none.
function benchCounterSlot(state, targetType) {
  for (const slot of livingBenchSlots(state, 1)) {
    const member = creature(state.duelState.partyIndex[1][slot]);
    const hasCounter = member.moves
      .filter((id) => id != null)
      .map((id) => DATA.moves[id])
      .filter((move) => isSpikeMove(move))
      .some((move) => duelBeats(state, move.type, targetType));
    if (hasCounter) return slot;
  }
  return -1;
}

function benchCounterExists(state, targetType) {
  return benchCounterSlot(state, targetType) >= 0;
}
function duelPick(state, index) {
  const duelState = state.duelState;
  duelState.picks = [index, aiDuelPick(state)];
  duelState.phase = 'reveal';
  duelState.timer = 550;
}
// Flat scoring: relation only, fixed damage, no power or affinity. Two
// spellings ride on it — pure RPS (flat) and RPS plus the creature-type bonus
// on winning hits (bonus).
const FLAT_WIN = 14, FLAT_CHIP = 4, FLAT_TRADE = 6;

// Creature-type bonus: your move's type against the opposing creature's types.
// 1.5 when it beats any of them, else 1 (bonus-only, no penalty).
// Creature-type matchup modifier: your move's type against the opposing
// creature's types. x1.5 when it beats any of them, x0.5 when it loses to one
// (and beats none), else 1. Bonus and penalty both apply to the damage dealt.
function duelCreatureBonus(state, side, moveType) {
  const defender = duelCreature(state, side === 0 ? 1 : 0);
  if (defender.types.some((type) => duelBeats(state, moveType, type))) return 1.5;
  if (defender.types.some((type) => duelValue(state, moveType, type) === 0.5)) return 0.5;
  return 1;
}

// Post-resolution housekeeping: when the active creature faints, prefer the
// slot it switched away from (KO'd incoming swap reverts) or the first living
// member; a side is dead only when every member has fainted.
function syncActive(state, side, preferredSlot) {
  const duelState = state.duelState;
  if (duelState.hp[side][duelState.activeSlot[side]] > 0) return;
  if (preferredSlot != null && duelState.hp[side][preferredSlot] > 0) {
    duelState.activeSlot[side] = preferredSlot;
    return;
  }
  const slot = duelState.hp[side].findIndex((hp) => hp > 0);
  if (slot >= 0) duelState.activeSlot[side] = slot;
}

function duelResolve(state) {
  const duelState = state.duelState;
  const roll = () => 0.85 + state.rng.next() * 0.3;
  const youSwitch = isSwitchPick(duelState.picks[0]);
  const foeSwitch = isSwitchPick(duelState.picks[1]);

  if (duelKind(state) === 'stances') {
    const order = stanceSet(state);
    const you = duelCreature(state, 0), foe = duelCreature(state, 1);
    const yourStance = youSwitch ? null : (order[duelState.picks[0]] ?? order[0]);
    const foeStance = foeSwitch ? null : (order[duelState.picks[1]] ?? order[0]);
    const affinity = (creature_, stance) => (creature_.merged[0] === stance ? 1.5 : 1);
    const chip = (attack) => Math.max(1, Math.round((3 + attack * 0.25) * roll()));
    let yourDamage = 0, foeDamage = 0;
    const previous = [null, null];
    if (youSwitch && foeSwitch) {
      previous[0] = duelState.activeSlot[0];
      previous[1] = duelState.activeSlot[1];
      duelState.activeSlot[0] = switchTargetSlot(state, 0, duelState.picks[0]);
      duelState.activeSlot[1] = switchTargetSlot(state, 1, duelState.picks[1]);
      duelState.lines = ['SWAP', 'both sides switch'];
      duelState.last = [null, null];
    } else if (youSwitch || foeSwitch) {
      const switcher = youSwitch ? 0 : 1;
      const attacker = 1 - switcher;
      previous[switcher] = duelState.activeSlot[switcher];
      duelState.activeSlot[switcher] = switchTargetSlot(state, switcher, duelState.picks[switcher]);
      if (attacker === 1) foeDamage = Math.round(FLAT_WIN * roll());
      else yourDamage = Math.round(FLAT_WIN * roll());
      duelState.lines = ['DEFAULT WIN', `-${yourDamage} -${foeDamage}`];
      duelState.last = [yourStance, foeStance];
    } else if (yourStance === foeStance) {
      yourDamage = chip(you.stats.attack);
      foeDamage = chip(foe.stats.attack);
      duelState.lines = ['TIE', `-${yourDamage} -${foeDamage}`];
    } else if (stanceBeats(order, yourStance, foeStance)) {
      yourDamage = Math.max(1, Math.round((4 + you.stats.attack * 0.5) * affinity(you, yourStance) * roll()));
      foeDamage = Math.max(1, Math.round((2 + foe.stats.attack * 0.2) * roll()));
      duelState.lines = ['CLASH WIN', `-${yourDamage} -${foeDamage}`];
    } else {
      foeDamage = Math.max(1, Math.round((4 + foe.stats.attack * 0.5) * affinity(foe, foeStance) * roll()));
      yourDamage = Math.max(1, Math.round((2 + you.stats.attack * 0.2) * roll()));
      duelState.lines = ['CLASH LOSE', `-${yourDamage} -${foeDamage}`];
    }
    duelState.hp[0][duelState.activeSlot[0]] = Math.max(0, duelState.hp[0][duelState.activeSlot[0]] - foeDamage);
    duelState.hp[1][duelState.activeSlot[1]] = Math.max(0, duelState.hp[1][duelState.activeSlot[1]] - yourDamage);
    syncActive(state, 0, previous[0]);
    syncActive(state, 1, previous[1]);
    if (!youSwitch) recordPlayerPick(state, yourStance);
    duelState.phase = 'result';
    duelState.timer = 1300;
    return;
  }

  const mine = duelMoves(state, 0), theirs = duelMoves(state, 1);
  const yourMove = youSwitch ? null : (mine[duelState.picks[0]] || mine[0]);
  const foeMove = foeSwitch ? null : (theirs[duelState.picks[1]] || theirs[0]);
  let yourDamage = 0, foeDamage = 0;
  const scoring = state.duel.scoring;
  const previous = [null, null];

  if (youSwitch && foeSwitch) {
    previous[0] = duelState.activeSlot[0];
    previous[1] = duelState.activeSlot[1];
    duelState.activeSlot[0] = switchTargetSlot(state, 0, duelState.picks[0]);
    duelState.activeSlot[1] = switchTargetSlot(state, 1, duelState.picks[1]);
    duelState.lines = ['SWAP', 'both sides switch'];
    duelState.last = [null, null];
  } else if (youSwitch || foeSwitch) {
    const switcher = youSwitch ? 0 : 1;
    previous[switcher] = duelState.activeSlot[switcher];
    duelState.activeSlot[switcher] = switchTargetSlot(state, switcher, duelState.picks[switcher]);
    const attackerSide = 1 - switcher;
    const attackerMove = attackerSide === 0 ? yourMove : foeMove;
    const incoming = duelCreature(state, switcher);
    const modifier = scoring === 'bonus' && attackerMove.type
      ? (incoming.types.some((type) => duelBeats(state, attackerMove.type, type)) ? 1.5
        : incoming.types.some((type) => duelValue(state, attackerMove.type, type) === 0.5) ? 0.5 : 1)
      : 1;
    const hit = Math.max(1, Math.round(FLAT_WIN * modifier));
    if (attackerSide === 0) yourDamage = hit; else foeDamage = hit;
    duelState.lines = ['DEFAULT WIN', `-${yourDamage} -${foeDamage}`];
    duelState.last = [yourMove ? yourMove.type : null, foeMove ? foeMove.type : null];
  } else if (scoring === 'flat' || scoring === 'bonus') {
    // Flat numbers: relation decides. In the bonus spelling the creature-type
    // matchup scales the damage dealt: x1.5 beating their type, x0.5 losing to it.
    const relation = duelValue(state, yourMove.type, foeMove.type);
    const mod = (side, move) => (scoring === 'bonus' ? duelCreatureBonus(state, side, move.type) : 1);
    const scaled = (base, multiplier) => Math.max(1, Math.round(base * multiplier));
    const yourMod = mod(0, yourMove), foeMod = mod(1, foeMove);
    if (relation === 2) {
      yourDamage = scaled(FLAT_WIN, yourMod);
      foeDamage = scaled(FLAT_CHIP, foeMod);
      duelState.lines = [yourMod > 1 ? 'BONUS WIN' : yourMod < 1 ? 'WEAK WIN' : 'RPS WIN',
        `-${yourDamage} -${foeDamage}`];
    } else if (relation === 0.5) {
      foeDamage = scaled(FLAT_WIN, foeMod);
      yourDamage = scaled(FLAT_CHIP, yourMod);
      duelState.lines = [foeMod > 1 ? 'BONUS LOSE' : foeMod < 1 ? 'WEAK LOSE' : 'RPS LOSE',
        `-${yourDamage} -${foeDamage}`];
    } else {
      yourDamage = scaled(FLAT_TRADE, yourMod);
      foeDamage = scaled(FLAT_TRADE, foeMod);
      duelState.lines = ['TRADE', `-${yourDamage} -${foeDamage}`];
    }
  } else {
    const relation = duelValue(state, yourMove.type, foeMove.type);
    const stab = (side, move) => (hasAffinity(state, side, move) ? 1.5 : 1);
    const yourPower = (4 + yourMove.power * 1.2) * stab(0, yourMove);
    const foePower = (4 + foeMove.power * 1.2) * stab(1, foeMove);
    const chip = (power) => Math.max(1, Math.round((2 + power * 0.25) * roll()));
    if (relation === 2) {
      yourDamage = Math.max(1, Math.round(yourPower * roll()));
      foeDamage = chip(foeMove.power);
      duelState.lines = ['WHEEL WIN', `-${yourDamage} -${foeDamage}`];
    } else if (relation === 0.5) {
      foeDamage = Math.max(1, Math.round(foePower * roll()));
      yourDamage = chip(yourMove.power);
      duelState.lines = ['WHEEL LOSE', `-${yourDamage} -${foeDamage}`];
    } else {
      yourDamage = chip(foeMove.power);
      foeDamage = chip(yourMove.power);
      duelState.lines = ['TRADE', `-${yourDamage} -${foeDamage}`];
    }
  }

  duelState.hp[0][duelState.activeSlot[0]] = Math.max(0, duelState.hp[0][duelState.activeSlot[0]] - foeDamage);
  duelState.hp[1][duelState.activeSlot[1]] = Math.max(0, duelState.hp[1][duelState.activeSlot[1]] - yourDamage);
  syncActive(state, 0, previous[0]);
  syncActive(state, 1, previous[1]);
  if (yourMove) {
    duelState.last = [yourMove.type, foeMove ? foeMove.type : null];
    recordPlayerPick(state, yourMove.type);
  }
  duelState.phase = 'result';
  duelState.timer = 1300;
}
// Player history for the persona models: last 8 revealed types plus counts of
// picks made after an AI read fired (exploiter input).
function recordPlayerPick(state, type) {
  const duelState = state.duelState;
  duelState.history = (duelState.history || []).concat(type).slice(-8);
  if (duelState.readFiredPrev) {
    duelState.postReadCounts = duelState.postReadCounts || {};
    duelState.postReadCounts[type] = (duelState.postReadCounts[type] || 0) + 1;
  }
  duelState.readFiredPrev = !!duelState.thisReadFired;
}

function duelUpdate(state, dt) {
  const duelState = state.duelState;
  if (duelState.phase === 'reveal') {
    duelState.timer -= dt;
    if (duelState.timer <= 0) duelResolve(state);
  } else if (duelState.phase === 'result') {
    duelState.timer -= dt;
    if (duelState.timer <= 0) {
      if (!sideAlive(state, 0) || !sideAlive(state, 1)) {
        state.endText = !sideAlive(state, 0) && !sideAlive(state, 1) ? 'DRAW'
          : !sideAlive(state, 1) ? 'WIN' : 'LOSE';
        state.scene = 'end';
      } else {
        duelState.round += 1;
        duelState.phase = 'choice';
        duelState.picks = [null, null];
        duelState.lines = ['', ''];
      }
    }
  }
}

/* ---------- step: consume fresh input edges, advance timers ---------- */
function setMode(state, mode) {
  if (state.lockMode) return;
  state.mode = mode;
  state.scene = 'setup';
}
function step(state, input) {
  const edges = (input && input.edges) || [];
  const dt = (input && input.dtMs) || 0;
  for (const edge of edges) {
    if (edge === '1' || edge === '2' || edge === '3') {
      if (state.lockMode) continue;
      setMode(state, edge === '1' ? 'current' : edge === '2' ? 'trim' : 'duel');
      continue;
    }
    if (state.scene === 'setup') {
      if (edge === 'left') state.youIndex = (state.youIndex + DATA.creatures.length - 1) % DATA.creatures.length;
      else if (edge === 'right') state.youIndex = (state.youIndex + 1) % DATA.creatures.length;
      else if (edge === 'up') state.foeIndex = (state.foeIndex + DATA.creatures.length - 1) % DATA.creatures.length;
      else if (edge === 'down') state.foeIndex = (state.foeIndex + 1) % DATA.creatures.length;
      else if (edge === 'a') startBattle(state);
    } else if (state.scene === 'battle') {
      if (edge === 'b') { state.scene = 'setup'; continue; }
      if (state.mode === 'duel') {
        const duelState = state.duelState;
        if (duelState.phase === 'choice') {
          const options = duelOptions(state, 0);
          const showSwap = livingBenchSlots(state, 0).length > 0;
          const count = Math.max(1, options.length + (showSwap ? 1 : 0));
          if (edge === 'left' || edge === 'up') duelState.cursor = (duelState.cursor + count - 1) % count;
          else if (edge === 'right' || edge === 'down') duelState.cursor = (duelState.cursor + 1) % count;
          else if (edge === 'a' && options.length) {
            duelPick(state, duelState.cursor >= options.length ? -1 : duelState.cursor);
          }
        }
      } else {
        const battleState = state.battle;
        if (battleState.phase === 'choice') {
          if (edge === 'up' || edge === 'down') cycleCursor(state, edge === 'down' ? 2 : -2);
          else if (edge === 'left') cycleCursor(state, -1);
          else if (edge === 'right') cycleCursor(state, 1);
          else if (edge === 'a') planTurn(state, !usableSlots(state, 0).length);
        } else if (battleState.phase === 'feedback') {
          if (edge === 'a') advanceStep(state);
        }
      }
    } else if (state.scene === 'end') {
      if (edge === 'a') state.scene = 'setup';
    }
  }
  if (state.scene === 'battle' && state.mode === 'duel') duelUpdate(state, dt);
}

/* ---------- rendering ---------- */
function px(ctx, x, y, color) { ctx.fillStyle = color ? '#fff' : '#000'; ctx.fillRect(x, y, 1, 1); }
function rect(ctx, x, y, w, h, color) { ctx.fillStyle = color ? '#fff' : '#000'; ctx.fillRect(x, y, w, h); }
function text(ctx, str, x, y, color = true) {
  for (const char of str) {
    const rows = glyphRows(char);
    if (rows) for (let gy = 0; gy < 6; gy++) for (let gx = 0; gx < 5; gx++)
      if (rows[gy][gx]) px(ctx, x + gx, y + gy, color);
    x += 5;
  }
  return x;
}
function drawSymbol(ctx, name, x, y, color = true) {
  const rows = SYMBOLS[name];
  if (!rows) return;
  for (let gy = 0; gy < 6; gy++) for (let gx = 0; gx < 5; gx++)
    if (rows[gy][gx] === '#') px(ctx, x + gx, y + gy, color);
}
function drawIcon(ctx, bits, x, y, color = true) {
  if (!bits) return;
  for (let gy = 0; gy < 8; gy++) for (let gx = 0; gx < 8; gx++)
    if (bits[gy][gx] === '#') px(ctx, x + gx, y + gy, color);
}
function imageFor(state, file) {
  const src = SPRITE_BASE + file;
  let entry = state.images.get(src);
  if (!entry && state.loadImage) {
    entry = { img: null, ready: false };
    state.images.set(src, entry);
    state.loadImage(src, (img) => { entry.img = img; entry.ready = true; });
  }
  return entry;
}
function drawSprite(ctx, state, file, x, y) {
  const entry = imageFor(state, file);
  if (!entry || !entry.ready || !entry.img) return;
  const image = entry.img;
  if (image.pixels) {
    for (let sy = 0; sy < image.height; sy++) for (let sx = 0; sx < image.width; sx++) {
      const offset = (sy * image.width + sx) * 4;
      if (image.pixels[offset + 3] === 0) continue;
      px(ctx, x + sx, y + sy, image.pixels[offset] >= 128);
    }
  } else {
    ctx.drawImage(image, x, y);
  }
}

// Type badges pinned to the creature art: black plate + white icon so the
// creature's type is readable without leaving the sprite.
function drawTypeBadges(ctx, state, foeOverride, youOverride) {
  const foe = foeOverride || creature(state.foeIndex), you = youOverride || creature(state.youIndex);
  const draw = (types, side) => {
    types.slice(0, 2).forEach((type, index) => {
      const x = side === 0 ? 1 + index * 12 : 113 - index * 12;
      rect(ctx, x - 1, 36, 12, 12, true); // white frame so the chip reads over art
      rect(ctx, x, 37, 10, 10, false);
      drawIcon(ctx, typeIcon(type), x + 1, 38, true);
    });
  };
  draw(typesOf(foe, activeMode(state)), 0);
  draw(typesOf(you, activeMode(state)), 1);
}

function drawHpPanel(ctx, side, creature_, hp, showNumbers) {
  const barY = side === 0 ? 10 : 35;
  if (side === 0 && showNumbers) text(ctx, `${num(hp)}/${num(creature_.maxHp)}`, 48, 1, true);
  else text(ctx, side === 0 ? 'FOE' : 'YOU', 48, side === 0 ? 2 : 20, true);
  if (side === 1) text(ctx, `${num(hp)}/${num(creature_.maxHp)}`, 48, 27, true);
  const width = creature_.maxHp === 0 ? 0 : Math.floor(hp * 28 / creature_.maxHp);
  rect(ctx, 48, barY, 32, 3, true);
  rect(ctx, 49, barY + 1, 30, 1, false);
  if (width > 0) rect(ctx, 49, barY + 1, width, 1, true);
}

function drawBattleCenter(ctx, state) {
  const battleState = state.battle;
  const showFoe = state.mode === 'trim' && state.trim.foeNumbers;
  drawHpPanel(ctx, 0, battleState.foe, battleState.hp[1], showFoe);
  drawHpPanel(ctx, 1, battleState.you, battleState.hp[0], true);
  if (battleState.phase === 'choice') {
    const move = moveOf(battleState.you, battleState.cursor);
    if (!move) { text(ctx, 'no moves', 48, 41, true); return; }
    drawIcon(ctx, typeIcon(move.type), 48, 40);
    text(ctx, num(move.power), 58, 41, true);
    const uses = battleState.uses[0][battleState.cursor];
    if (uses === 255) drawSymbol(ctx, '*', 72, 41, true);
    else text(ctx, num(uses), 72, 41, true);
  } else if (battleState.flash) {
    text(ctx, battleState.flash, 48, 41, true);
  }
}

function drawBattleMenu(ctx, state) {
  const battleState = state.battle;
  const usable = usableSlots(state, 0);
  for (let slot = 0; slot < 4; slot++) {
    const move = moveOf(battleState.you, slot);
    if (!isSpikeMove(move)) continue;
    const x = 4 + (slot % 2) * 64, y = 48 + Math.floor(slot / 2) * 8;
    const selected = battleState.phase === 'choice' && slot === battleState.cursor && usable.includes(slot);
    const depleted = battleState.uses[0][slot] === 0;
    if (selected) rect(ctx, x, y, 60, 8, true);
    const label = move.name.slice(0, 9);
    text(ctx, label, x + 3, y + 1, !selected);
    if (depleted) rect(ctx, x + 3, y + 7, textWidth(label), 1, !selected);
    if (state.mode === 'trim' && state.trim.arrows) {
      const modifier = attackModifier(battleState.you, battleState.foe, move.type, activeMode(state));
      drawSymbol(ctx, arrowFor(modifier), x + 52, y + 1, !selected);
    }
  }
  if (!usable.length) {
    rect(ctx, 4, 56, 120, 8, true);
    text(ctx, 'pass', 7, 57, false);
    drawSymbol(ctx, '>', 24, 57, false);
  }
}

function drawBattle(ctx, state) {
  drawSprite(ctx, state, state.battle.foe.front, 0, 0);
  drawSprite(ctx, state, state.battle.you.back, 80, 0);
  drawTypeBadges(ctx, state);
  drawBattleCenter(ctx, state);
  if (state.battle.phase === 'choice') drawBattleMenu(ctx, state);
  else {
    rect(ctx, 0, 48, 128, 16, false);
    if (state.battle.lines[0]) text(ctx, state.battle.lines[0], 2, 48, true);
    if (state.battle.lines[1]) {
      const x = state.battle.arrow ? 9 : 2;
      if (state.battle.arrow) drawSymbol(ctx, state.battle.arrow, 2, 56, true);
      text(ctx, state.battle.lines[1].slice(0, 24), x, 56, true);
    }
  }
}

function drawSetup(ctx, state) {
  const you = creature(state.youIndex), foe = creature(state.foeIndex);
  drawSprite(ctx, state, foe.front, 0, 0);
  drawSprite(ctx, state, you.back, 80, 0);
  drawTypeBadges(ctx, state);
  if (state.mode === 'duel') {
    text(ctx, '3v3 TEAMS ROLL ON START', 9, 30, true);
  }
  const youName = you.name.slice(0, 12), foeName = foe.name.slice(0, 12);
  text(ctx, youName, 2, 49, true);
  text(ctx, foeName, W - 2 - textWidth(foeName), 49, true);
  text(ctx, `HP${num(you.maxHp)}`, 2, 57, true);
  text(ctx, 'A FIGHT', 46, 57, true);
  const foeHp = `HP${num(foe.maxHp)}`;
  text(ctx, foeHp, W - 2 - textWidth(foeHp), 57, true);
}

function drawEnd(ctx, state) {
  const battleState = state.battle;
  const foe = creature(state.foeIndex), you = creature(state.youIndex);
  drawSprite(ctx, state, battleState ? battleState.foe.front : foe.front, 0, 0);
  drawSprite(ctx, state, battleState ? battleState.you.back : you.back, 80, 0);
  text(ctx, state.endText, (W - textWidth(state.endText)) / 2, 28, true);
  text(ctx, 'A RETRY', 46, 56, true);
}

function drawDuelMoveRow(ctx, state) {
  const duelState = state.duelState;
  const options = duelOptions(state, 0);
  const showSwap = livingBenchSlots(state, 0).length > 0;
  const count = Math.max(1, Math.min(options.length + (showSwap ? 1 : 0), 5));
  const cell = Math.floor(120 / count);
  for (let index = 0; index < count; index++) {
    const x = 4 + index * cell, y = 48;
    const selected = duelState.cursor === index;
    if (selected) rect(ctx, x, y, cell - 2, 14, true);
    if (index < options.length) {
      const option = options[index];
      drawIcon(ctx, typeIcon(option.type), x + Math.floor((cell - 2 - 8) / 2), y + 2, !selected);
      // Pick markers per cell: ✱ affinity (power spelling); ↑ bonus / ↓ penalty
      // from the creature-type matchup (bonus spelling).
      let marker = null;
      if (state.duel.scoring === 'power' && duelOptionAffinity(state, 0, option)) marker = '*';
      else if (state.duel.scoring === 'bonus') {
        const modifier = duelCreatureBonus(state, 0, option.type);
        marker = modifier > 1 ? 'up' : modifier < 1 ? 'down' : null;
      }
      if (marker) drawSymbol(ctx, marker, x + cell - 10, y + 2, !selected);
    } else {
      text(ctx, 'swap', x + Math.floor((cell - 2 - textWidth('swap')) / 2), y + 5, !selected);
    }
  }
}

function drawDuelFeedback(ctx, state) {
  const duelState = state.duelState;
  rect(ctx, 0, 48, 128, 16, false);
  const mine = duelOptions(state, 0), theirs = duelOptions(state, 1);
  const yourMove = mine[duelState.picks[0]] || mine[0];
  const foeMove = theirs[duelState.picks[1]] || theirs[0];
  if (yourMove) {
    drawIcon(ctx, typeIcon(yourMove.type), 6, 50, true);
    text(ctx, 'vs', 18, 51, true);
    drawIcon(ctx, typeIcon(foeMove.type), 30, 50, true);
  }
  if (duelState.lines[0]) text(ctx, duelState.lines[0], 42, 48, true);
  if (duelState.lines[1]) text(ctx, duelState.lines[1].slice(0, 17), 42, 56, true);
  if (state.duel.explain && duelState.aiRead) text(ctx, 'AI READ', 93, 48, true);
}

function drawTeamPips(ctx, state, side, y) {
  const duelState = state.duelState;
  for (let slot = 0; slot < duelState.hp[side].length && slot < 3; slot++) {
    const x = 72 + slot * 3;
    if (duelState.hp[side][slot] > 0) rect(ctx, x, y, 2, 3, true);
    if (slot === duelState.activeSlot[side]) rect(ctx, x, y + 4, 2, 1, true);
  }
}

function drawDuel(ctx, state) {
  const duelState = state.duelState;
  const you = duelCreature(state, 0), foe = duelCreature(state, 1);
  drawSprite(ctx, state, foe.front, 0, 0);
  drawSprite(ctx, state, you.back, 80, 0);
  drawTypeBadges(ctx, state, foe, you);
  const showFoe = state.trim.foeNumbers;
  drawHpPanel(ctx, 0, foe, duelState.hp[1][duelState.activeSlot[1]], showFoe);
  drawHpPanel(ctx, 1, you, duelState.hp[0][duelState.activeSlot[0]], true);
  drawTeamPips(ctx, state, 0, 2);
  drawTeamPips(ctx, state, 1, 27);
  if (duelState.phase === 'choice') {
    const options = duelOptions(state, 0);
    text(ctx, `R${num(duelState.round)}`, 48, 41, true);
    const onSwap = duelState.cursor >= options.length;
    const option = onSwap ? null : options[Math.min(duelState.cursor, options.length - 1)];
    if (onSwap) {
      text(ctx, 'swap', 60, 41, true);
    } else if (option) {
      drawIcon(ctx, typeIcon(option.type), 60, 40);
      if (state.duel.scoring === 'power' && option.power != null) {
        const label = num(option.power);
        text(ctx, label, 80 - textWidth(label), 41, true);
      }
    }
    drawDuelMoveRow(ctx, state);
  } else {
    drawDuelFeedback(ctx, state);
  }
}

function render(ctx, state) {
  rect(ctx, 0, 0, W, H, false);
  if (state.scene === 'setup') drawSetup(ctx, state);
  else if (state.scene === 'battle') { if (state.mode === 'duel') drawDuel(ctx, state); else drawBattle(ctx, state); }
  else drawEnd(ctx, state);
}

/* ---------- browser boot (skipped under CommonJS) ---------- */
function boot() {
  const screen = document.getElementById('screen');
  const ctx = screen.getContext('2d');
  const bodyMode = document.body && document.body.dataset && document.body.dataset.mode
    ? document.body.dataset.mode : null;
  const bodyDuel = document.body && document.body.dataset && document.body.dataset.duel
    ? document.body.dataset.duel : null;
  const bodyScoring = document.body && document.body.dataset && document.body.dataset.scoring
    ? document.body.dataset.scoring : null;
  const query = typeof location !== 'undefined' && typeof location.search === 'string' ? location.search : '';
  const seedMatch = /[?&]seed=(\d+)/.exec(query);
  const seed = seedMatch ? Number(seedMatch[1]) : undefined;

  const state = newMock({
    mode: bodyMode || 'current',
    lockMode: !!bodyMode,
    duelKind: bodyDuel || undefined,
    scoring: bodyScoring || undefined,
    seed,
    loadImage(src, done) {
      const img = new Image();
      img.onload = () => done(img);
      img.src = src;
    },
  });
  window.__battleMock = state;

  const KEYMAP = {
    ArrowLeft: 'left', ArrowRight: 'right', ArrowUp: 'up', ArrowDown: 'down',
    a: 'left', d: 'right', w: 'up', s: 'down',
    z: 'a', ' ': 'a', Enter: 'a', x: 'b',
    '1': '1', '2': '2', '3': '3',
  };
  const held = new Set();
  const pending = [];
  window.addEventListener('keydown', (event) => {
    const edge = KEYMAP[event.key];
    if (!edge) return;
    event.preventDefault();
    if (!held.has(edge)) { held.add(edge); pending.push(edge); }
  });
  window.addEventListener('keyup', (event) => {
    const edge = KEYMAP[event.key];
    if (edge) held.delete(edge);
  });
  for (const button of document.querySelectorAll('[data-key]')) {
    const edge = button.dataset.key;
    const press = (event) => { event.preventDefault(); if (!held.has(edge)) { held.add(edge); pending.push(edge); } };
    const release = () => held.delete(edge);
    button.addEventListener('pointerdown', press);
    button.addEventListener('pointerup', release);
    button.addEventListener('pointerleave', release);
  }

  const modeRadios = document.querySelectorAll('input[name="mode"]');
  const control = (id) => document.getElementById(id);
  const wire = (id, type, handler) => {
    const element = control(id);
    if (element) element.addEventListener(type, handler);
  };
  for (const radio of modeRadios) radio.addEventListener('change', () => {
    if (radio.checked) { setMode(state, radio.value); syncControls(); }
  });
  wire('trim-type', 'change', (event) => { state.trim.typeMode = event.target.value; syncControls(); });
  wire('trim-arrows', 'change', (event) => { state.trim.arrows = event.target.checked; syncControls(); });
  wire('trim-press', 'change', (event) => { state.trim.pressTurn = event.target.checked; syncControls(); });
  wire('trim-foe', 'change', (event) => { state.trim.foeNumbers = event.target.checked; syncControls(); });
  wire('duel-stances', 'change', (event) => {
    state.duel.stances = Number(event.target.value);
    if (state.mode === 'duel') state.scene = 'setup';
    syncControls();
  });
  wire('duel-ai', 'change', (event) => { state.duel.aiPersona = event.target.value; syncControls(); });
  wire('duel-matrix', 'change', (event) => {
    state.duel.matrixId = event.target.value;
    if (state.mode === 'duel') state.scene = 'setup';
    syncControls();
  });
  wire('duel-read', 'input', (event) => { state.duel.aiRead = Number(event.target.value); syncControls(); });
  wire('duel-explain', 'change', (event) => { state.duel.explain = event.target.checked; });
  wire('scale', 'change', (event) => applyScale(Number(event.target.value)));
  wire('new-seed', 'click', () => { newSeed(state); syncControls(); });

  function applyScale(scale) {
    screen.style.width = `${W * scale}px`;
    screen.style.height = `${H * scale}px`;
  }
  function syncControls() {
    for (const radio of modeRadios) radio.checked = radio.value === state.mode;
    const set = (id, apply) => { const element = control(id); if (element) apply(element); };
    set('trim-type', (element) => { element.value = state.trim.typeMode; });
    set('trim-arrows', (element) => { element.checked = state.trim.arrows; });
    set('trim-press', (element) => { element.checked = state.trim.pressTurn; });
    set('trim-foe', (element) => { element.checked = state.trim.foeNumbers; });
    set('duel-stances', (element) => { element.value = String(state.duel.stances); });
    set('duel-ai', (element) => { element.value = state.duel.aiPersona; });
    set('duel-matrix', (element) => { element.value = state.duel.matrixId; });
    set('duel-read', (element) => { element.value = String(state.duel.aiRead); });
    set('duel-explain', (element) => { element.checked = state.duel.explain; });
    set('duel-read-value', (element) => { element.textContent = `${state.duel.aiRead}%`; });
    set('seed-value', (element) => { element.textContent = String(state.seed); });
    set('status', (element) => {
      const you = creature(state.youIndex), foe = creature(state.foeIndex);
      const count = (entry) => entry.moves.filter((id) => id != null).length;
      element.textContent = [
        state.mode === 'duel' ? `duel ${state.duel.kind}` : state.mode,
        state.mode === 'trim' ? `${state.trim.typeMode} arrows:${state.trim.arrows ? 'on' : 'off'} press:${state.trim.pressTurn ? 'on' : 'off'}` : '',
        state.mode === 'duel' && state.duel.kind === 'stances' ? `${state.duel.stances} stances ai:${state.duel.aiPersona} ${state.duel.aiRead}%` : '',
        state.mode === 'duel' && state.duel.kind === 'wheel' ? `duel wheel ${state.duel.scoring} ${state.duel.matrixId}t ai:${state.duel.aiPersona} ${state.duel.aiRead}%` : '',
        state.mode === 'duel' && state.duelState
          ? `you:${state.duelState.partyIndex[0].map((index) => creature(index).name).join(',')} foe:${state.duelState.partyIndex[1].map((index) => creature(index).name).join(',')}`
          : '',
        state.mode !== 'duel' ? `you:${creature(state.youIndex).name}[${count(you)}m] foe:${creature(state.foeIndex).name}[${count(foe)}m]` : '',
      ].filter(Boolean).join(' · ');
    });
  }
  applyScale(Number((control('scale') || {}).value) || 4);
  syncControls();

  let last = performance.now();
  function frame(now) {
    const dt = Math.min(120, now - last);
    last = now;
    const edges = pending.splice(0, pending.length);
    step(state, { edges, dtMs: dt });
    render(ctx, state);
    if (edges.some((edge) => edge === 'a' || edge === '1' || edge === '2' || edge === '3')) syncControls();
    requestAnimationFrame(frame);
  }
  requestAnimationFrame(frame);
}

if (typeof module !== 'undefined' && module.exports) {
  module.exports = {
    newMock, step, render, setMode, startBattle, planTurn, advanceStep, aiPick,
    usableSlots, firstUsable, cycleCursor, duelPick, duelResolve, duelUpdate,
    newSeed, makeRng, computeDamage, attackModifier, combineModifier,
    applyDamageModifier, modifierFor, arrowFor, moveTypeFor,
    duelMoves, wheelValue, wheelBeats, hasAffinity, duelCreatureBonus,
    duelKind, stanceSet, stanceBeats, duelOptions, duelOptionAffinity,
    DUEL_PERSONAS, duelPersona, predictPlayerType, counterOptions, recordPlayerPick,
    duelCreature, duelBench, benchCounterExists, benchCounterSlot, sideAlive, isSwitchPick,
    livingBenchSlots, switchTargetSlot, rollTeam, SWITCH_BASE,
    duelMatrix, duelValue, duelBeats, foldType, MATRIX_DATA,
    creature, moveOf, activeMode, typesOf, drawIcon, text, glyphRows,
    ICONS, MERGED_ICONS, SYMBOLS, DATA, W, H, SPRITE_BASE, isSpikeMove,
  };
}
if (typeof window !== 'undefined' && typeof document !== 'undefined') boot();
