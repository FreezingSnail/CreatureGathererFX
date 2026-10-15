'use strict';

/*
 * Creature Gatherer battle mock — 128x64 playable spike over the real roster.
 *
 * Goal: test how three battle designs communicate on the actual display, not art.
 *   A "current"  the shipping 8-type dual-type move menu.
 *   B "trim"     4-type single-type cycle + per-move effectiveness arrows +
 *                optional SMT-style press-turn (super hit acts again).
 *   C "duel"     simultaneous hidden-stance RPS: both pick, reveal, damage.
 *
 * Keys (browser):
 *   Arrows/WASD  move cursor / pick creature in setup
 *   Z or Space   A: confirm, advance feedback, start fight
 *   X            B: back to setup
 *   1 / 2 / 3    switch design (current / trim / duel)
 * Page controls tune: trim type mode, arrows, press-turn, foe HP readout,
 * duel stance count (3- or 4-cycle), and how often the AI reads your last pick.
 *
 * Fidelity: damage, speed order, move-use limits and STAB follow the production
 * C++ (Damage.cpp, Type.hpp, MoveUses.hpp) ported to integer JS with stat stages
 * at zero. The AI is a simplified damage-scoring stand-in for the tactical AI.
 * Not simulated: statuses, stages, effects, accuracy, party/switching, escape,
 * gather, animations. Feedback text replaces the staged presenter; HP updates
 * immediately. Sprite layout matches BattleLayout.hpp (foe 0,0; you 80,0; menu
 * y=48..63; HP column x=48..79; 48x48 front/back frames).
 */

const DATA = (typeof module !== 'undefined' && module.exports && typeof require === 'function')
  ? require('./data.js')
  : (typeof window !== 'undefined' ? window.CGFX_SPIKE_DATA : null);

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

/* ---------- authored 8x8 type icons (new art for the spike) ---------- */
const ICONS = {
  spirit: [
    '..####..', '.#....#.', '#.#..#.#', '#......#',
    '#......#', '#.#..#.#', '.#....#.', '..####..'],
  water: [
    '...#....', '..###...', '.#####..', '.#####..',
    '#######.', '#######.', '.#####..', '..###...'],
  wind: [
    '........', '.#####..', '#.....#.', '.....#..',
    '..####..', '.#......', '#.....#.', '.#####..'],
  earth: [
    '...#....', '..###...', '.#####..', '#######.',
    '.......#', '...####.', '.#######', '#######.'],
  fire: [
    '...#....', '..##....', '..###...', '.#####..',
    '.######.', '.######.', '..####..', '...#....'],
  lightning: [
    '....###.', '...###..', '..###...', '.#####..',
    '...###..', '...##...', '..##....', '..#.....'],
  plant: [
    '....##..', '...###..', '..####..', '.#####..',
    '#####...', '####....', '###.....', '##......'],
  elder: [
    '..####..', '.#....#.', '#..##..#', '#.####.#',
    '#..##..#', '.#....#.', '..####..', '...##...'],
};
const MERGED_ICONS = { flame: ICONS.fire, bloom: ICONS.plant, tide: ICONS.water, stone: ICONS.earth };

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
    duel: { stances: options.stances || 3, aiRead: options.aiRead ?? 45, explain: options.explain !== false },
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
    state.duelState = {
      hp: [you.maxHp, foe.maxHp],
      picks: [null, null],
      last: [null, null],
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
    if (creature_.moves[slot] == null) continue;
    if (state.battle.uses[side][slot] > 0) slots.push(slot);
  }
  return slots;
}
function firstUsable(battleState) {
  const slots = [];
  for (let slot = 0; slot < 4; slot++) {
    if (battleState.you.moves[slot] == null) continue;
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

/* ---------- duel (RPS) resolution ---------- */
const stanceSet = (state) => DATA.merged.order.slice(0, state.duel.stances);
function beats(order, stance, other) {
  const step = (order.indexOf(other) - order.indexOf(stance) + order.length) % order.length;
  return step === 1;
}
function aiDuelPick(state) {
  const order = stanceSet(state);
  const duelState = state.duelState;
  duelState.aiRead = false;
  const last = duelState.last[0];
  if (last != null && state.rng.next() * 100 < state.duel.aiRead) {
    for (const stance of order) {
      if (beats(order, stance, order[last])) {
        duelState.aiRead = true;
        return order.indexOf(stance);
      }
    }
  }
  return state.rng.int(order.length);
}
function duelPick(state, index) {
  const duelState = state.duelState;
  duelState.picks = [index, aiDuelPick(state)];
  duelState.phase = 'reveal';
  duelState.timer = 550;
}
function duelResolve(state) {
  const duelState = state.duelState;
  const you = creature(state.youIndex), foe = creature(state.foeIndex);
  const order = stanceSet(state);
  const yourStance = order[duelState.picks[0]], foeStance = order[duelState.picks[1]];
  const affinity = (creature_, stance) => (creature_.merged[0] === stance ? 1.5 : 1);
  const roll = () => 0.85 + state.rng.next() * 0.3;
  let yourDamage = 0, foeDamage = 0;
  if (yourStance === foeStance) {
    yourDamage = Math.max(1, Math.round((3 + foe.stats.attack * 0.25) * roll()));
    foeDamage = Math.max(1, Math.round((3 + you.stats.attack * 0.25) * roll()));
    duelState.lines = ['TIE', `-${yourDamage} -${foeDamage}`];
  } else if (beats(order, yourStance, foeStance)) {
    yourDamage = Math.max(1, Math.round((4 + you.stats.attack * 0.5) * affinity(you, yourStance) * roll()));
    foeDamage = Math.max(1, Math.round((2 + foe.stats.attack * 0.2) * roll()));
    duelState.lines = ['CLASH', `win -${yourDamage} -${foeDamage}`];
  } else {
    foeDamage = Math.max(1, Math.round((4 + foe.stats.attack * 0.5) * affinity(foe, foeStance) * roll()));
    yourDamage = Math.max(1, Math.round((2 + you.stats.attack * 0.2) * roll()));
    duelState.lines = ['CLASH', `lose -${yourDamage} -${foeDamage}`];
  }
  duelState.hp[0] = Math.max(0, duelState.hp[0] - foeDamage);
  duelState.hp[1] = Math.max(0, duelState.hp[1] - yourDamage);
  duelState.last = duelState.picks.slice();
  duelState.phase = 'result';
  duelState.timer = 1300;
}
function duelUpdate(state, dt) {
  const duelState = state.duelState;
  if (duelState.phase === 'reveal') {
    duelState.timer -= dt;
    if (duelState.timer <= 0) duelResolve(state);
  } else if (duelState.phase === 'result') {
    duelState.timer -= dt;
    if (duelState.timer <= 0) {
      if (duelState.hp[0] <= 0 || duelState.hp[1] <= 0) {
        state.endText = duelState.hp[0] <= 0 && duelState.hp[1] <= 0 ? 'DRAW'
          : duelState.hp[1] <= 0 ? 'WIN' : 'LOSE';
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
          const count = stanceSet(state).length;
          if (edge === 'left' || edge === 'up') duelState.cursor = (duelState.cursor + count - 1) % count;
          else if (edge === 'right' || edge === 'down') duelState.cursor = (duelState.cursor + 1) % count;
          else if (edge === 'a') duelPick(state, duelState.cursor);
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
  if (!showFoe) typesOf(battleState.foe, activeMode(state)).slice(0, 2)
    .forEach((type, index) => drawIcon(ctx, typeIcon(type), 64 + index * 9, 2));
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
    if (!move) continue;
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

function drawStanceRow(ctx, state) {
  const duelState = state.duelState;
  const order = stanceSet(state);
  const count = order.length;
  const cell = Math.floor(120 / count);
  for (let index = 0; index < count; index++) {
    const x = 4 + index * cell, y = 48;
    const selected = duelState.cursor === index;
    if (selected) rect(ctx, x, y, cell - 2, 14, true);
    drawIcon(ctx, typeIcon(order[index]), x + Math.floor((cell - 2 - 8) / 2), y + 2, !selected);
  }
}

function drawDuelFeedback(ctx, state) {
  const duelState = state.duelState;
  rect(ctx, 0, 48, 128, 16, false);
  const order = stanceSet(state);
  if (duelState.picks[0] != null) {
    drawIcon(ctx, typeIcon(order[duelState.picks[0]]), 6, 50, true);
    text(ctx, 'vs', 18, 51, true);
    drawIcon(ctx, typeIcon(order[duelState.picks[1]]), 30, 50, true);
  }
  if (duelState.lines[0]) text(ctx, duelState.lines[0], 42, 48, true);
  if (duelState.lines[1]) text(ctx, duelState.lines[1].slice(0, 17), 42, 56, true);
  if (state.duel.explain && duelState.aiRead) text(ctx, 'AI READ', 93, 48, true);
}

function drawDuel(ctx, state) {
  const duelState = state.duelState;
  const you = creature(state.youIndex), foe = creature(state.foeIndex);
  drawSprite(ctx, state, foe.front, 0, 0);
  drawSprite(ctx, state, you.back, 80, 0);
  const showFoe = state.trim.foeNumbers;
  drawHpPanel(ctx, 0, foe, duelState.hp[1], showFoe);
  drawHpPanel(ctx, 1, you, duelState.hp[0], true);
  if (!showFoe) typesOf(foe, activeMode(state)).slice(0, 2)
    .forEach((type, index) => drawIcon(ctx, typeIcon(type), 64 + index * 9, 2));
  text(ctx, `R${num(duelState.round)}`, 48, 41, true);
  const affinityStance = you.merged[0];
  if (stanceSet(state).includes(affinityStance)) drawIcon(ctx, typeIcon(affinityStance), 68, 40);
  if (duelState.phase === 'choice') drawStanceRow(ctx, state);
  else drawDuelFeedback(ctx, state);
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
  const query = typeof location !== 'undefined' && typeof location.search === 'string' ? location.search : '';
  const seedMatch = /[?&]seed=(\d+)/.exec(query);
  const seed = seedMatch ? Number(seedMatch[1]) : undefined;

  const state = newMock({
    mode: bodyMode || 'current',
    lockMode: !!bodyMode,
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
    setMode(state, state.mode);
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
    set('duel-read', (element) => { element.value = String(state.duel.aiRead); });
    set('duel-explain', (element) => { element.checked = state.duel.explain; });
    set('duel-read-value', (element) => { element.textContent = `${state.duel.aiRead}%`; });
    set('seed-value', (element) => { element.textContent = String(state.seed); });
    set('status', (element) => {
      element.textContent = [
        state.mode,
        state.mode === 'trim' ? `${state.trim.typeMode} arrows:${state.trim.arrows ? 'on' : 'off'} press:${state.trim.pressTurn ? 'on' : 'off'}` : '',
        state.mode === 'duel' ? `${state.duel.stances} stances ai-read:${state.duel.aiRead}%` : '',
        `you:${creature(state.youIndex).name} foe:${creature(state.foeIndex).name}`,
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
    applyDamageModifier, modifierFor, arrowFor, beats, stanceSet, moveTypeFor,
    creature, moveOf, activeMode, typesOf, drawIcon, text, glyphRows,
    ICONS, MERGED_ICONS, SYMBOLS, DATA, W, H, SPRITE_BASE,
  };
}
if (typeof window !== 'undefined' && typeof document !== 'undefined') boot();
