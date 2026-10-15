#!/usr/bin/env node
// Generate mock/data.js from authored sources.
// Inputs: data/json/creatures.json, data/json/moves.json, data/typetable.csv,
//         docs/assets/battle-sprites48/manifest.json, images/ArduFontTrimmed_5x6.png
// Run: node tools/battle-design-spike-data.mjs
// Test: node --test tools/tests/battle-design-spike-data_test.mjs
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { decodePng } from './creature-sprite-export.mjs';

const repo = path.dirname(fileURLToPath(import.meta.url)) + '/..';
export const OUTPUT = 'mock/data.js';
export const SPIKE_LEVEL = 8;

// Proposal under test: 8 authored types fold into a 4-type cycle so the whole
// chart fits in the player's head and on the move menu. Placeholder mapping:
// retype creatures only after the owner accepts the cycle.
export const MERGED = {
  order: ['flame', 'bloom', 'tide', 'stone'],
  map: {
    fire: 'flame', lightning: 'flame',
    plant: 'bloom', spirit: 'bloom',
    water: 'tide', wind: 'tide',
    earth: 'stone', elder: 'stone',
  },
  // beats: flame > bloom > tide > stone > flame
  cycle: ['flame', 'bloom', 'tide', 'stone'],
};

// 8-type wheel for the simultaneous-pick duel: each type beats exactly the next
// entry (clockwise) and loses to the previous one; everything else is neutral.
// One ring diagram teaches the entire 8x8 matrix, and no cell is immunity.
export const WHEEL = {
  order: ['wind', 'earth', 'lightning', 'water', 'fire', 'plant', 'spirit', 'elder'],
};

const STAT_EFFECTS = ['ATKUP', 'SPDUP', 'DEFUP', 'SPCAUP', 'ATKDWN', 'SPDDWN'];

function readJson(relative) {
  return JSON.parse(fs.readFileSync(path.join(repo, relative), 'utf8'));
}

function parseTypeChart() {
  const lines = fs.readFileSync(path.join(repo, 'data/typetable.csv'), 'utf8')
    .trim().split(/\r?\n/);
  const header = lines[0].split('\t');
  const order = header.slice(1);
  if (order.length !== 8) throw Error(`expected 8 typed columns, got ${order.length}`);
  const chart = lines.slice(1).map((line, row) => {
    const cells = line.split('\t');
    if (cells[0] !== order[row]) throw Error(`row order mismatch at ${cells[0]}`);
    return cells.slice(1).map((value) => {
      const number = Number(value);
      if (![0, 0.5, 1, 2].includes(number)) throw Error(`bad chart value ${value}`);
      return number;
    });
  });
  if (chart.length !== 8 || chart.some((row) => row.length !== 8))
    throw Error('type chart is not 8x8');
  return { order, chart };
}

function cycleChart(order) {
  // 2 = beats, 0.5 = loses to, 1 = neutral. order defines the beat cycle:
  // attacker beats the next entry and loses to the previous one.
  return order.map((attacker) => order.map((defender) => {
    if (attacker === defender) return 1;
    const step = (order.indexOf(defender) - order.indexOf(attacker) + order.length) % order.length;
    if (step === 1) return 2;
    if (step === order.length - 1) return 0.5;
    return 1;
  }));
}

function stat(level, seed) {
  return 2 * level + seed * Math.floor(level / 3);
}

export function moveUseLimit(move) {
  if (move.power === 0 && STAT_EFFECTS.includes(move.effect)) return 3;
  if (move.power >= 10 || move.effect !== 'NONE') return 2;
  return 255;
}

function glyphBit(image, x, y) {
  return image.pixels[(y * image.width + x) * 4] >= 128 ? '1' : '0';
}

export function buildSpikeData() {
  const typeChart = parseTypeChart();
  const creaturesJson = readJson('data/json/creatures.json');
  const movesJson = readJson('data/json/moves.json');
  const manifest = readJson('docs/assets/battle-sprites48/manifest.json');
  const font = decodePng(fs.readFileSync(path.join(repo, 'images/ArduFontTrimmed_5x6.png')));

  if (font.width !== 375 || font.height !== 6)
    throw Error(`unexpected font atlas size ${font.width}x${font.height}`);
  const firstChar = 48;
  const glyphs = [];
  for (let index = 0; index < 75; index++) {
    let bits = '';
    for (let y = 0; y < 6; y++)
      for (let x = 0; x < 5; x++)
        bits += glyphBit(font, index * 5 + x, y);
    glyphs.push(bits);
  }

  const moves = {};
  for (const move of movesJson) {
    if (moves[move.id]) throw Error(`duplicate move id ${move.id}`);
    moves[move.id] = {
      id: move.id, name: move.name, type: move.type,
      power: move.power, physical: !!move.physical,
      effect: move.effect, limit: moveUseLimit(move),
    };
  }

  const spriteById = new Map();
  for (const sprite of manifest.sprites) {
    const entry = spriteById.get(sprite.id) ?? {};
    entry[sprite.view] = sprite.file;
    spriteById.set(sprite.id, entry);
  }

  const types = (creature) => [creature.type1, creature.type2].filter((type) => type !== 'none');
  const mergedTypes = (creature) => {
    const out = [];
    for (const type of types(creature)) {
      const merged = MERGED.map[type];
      if (!merged) throw Error(`no merged mapping for ${type}`);
      if (!out.includes(merged)) out.push(merged);
    }
    return out;
  };

  const creatures = creaturesJson.map((creature) => {
    const sprite = spriteById.get(creature.id);
    if (!sprite || !sprite.front || !sprite.back)
      throw Error(`missing sprites for ${creature.name}`);
    const equipped = [];
    for (const id of [creature.move1, creature.move2, creature.move3, creature.move4]) {
      if (id === 32) { equipped.push(null); continue; }
      if (id === 255 || !moves[id]) throw Error(`bad equipped move ${id} on ${creature.name}`);
      equipped.push(id);
    }
    return {
      id: creature.id, name: creature.name, types: types(creature),
      merged: mergedTypes(creature),
      stats: {
        attack: stat(SPIKE_LEVEL, creature.atk),
        defense: stat(SPIKE_LEVEL, creature.deff),
        spcAtk: stat(SPIKE_LEVEL, creature.spcatk),
        spcDef: stat(SPIKE_LEVEL, creature.spcdef),
        speed: stat(SPIKE_LEVEL, creature.spd),
      },
      maxHp: stat(SPIKE_LEVEL, creature.hp) + 30,
      moves: equipped,
      front: sprite.front, back: sprite.back,
    };
  });

  return {
    level: SPIKE_LEVEL,
    typeOrder: typeChart.order,
    chart: typeChart.chart,
    merged: {
      order: MERGED.order,
      map: MERGED.map,
      chart: cycleChart(MERGED.order),
    },
    wheel: {
      order: WHEEL.order,
      chart: cycleChart(WHEEL.order),
    },
    font: { firstChar, width: 5, height: 6, glyphs },
    moves,
    creatures,
  };
}

export function serializeSpikeData(data) {
  return '// Generated by tools/battle-design-spike-data.mjs. Do not hand-edit.\n'
    + '// Sources: data/json/creatures.json, data/json/moves.json, data/typetable.csv,\n'
    + '// docs/assets/battle-sprites48/manifest.json, images/ArduFontTrimmed_5x6.png.\n'
    + '// Loads as a browser script (window.CGFX_SPIKE_DATA) or CommonJS (module.exports).\n'
    + '(function (root) {\n'
    + '  const DATA = ' + JSON.stringify(data) + ';\n'
    + '  if (typeof module !== "undefined" && module.exports) module.exports = DATA;\n'
    + '  else root.CGFX_SPIKE_DATA = DATA;\n'
    + '})(typeof window !== "undefined" ? window : this);\n';
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const text = serializeSpikeData(buildSpikeData());
  fs.mkdirSync(path.join(repo, path.dirname(OUTPUT)), { recursive: true });
  fs.writeFileSync(path.join(repo, OUTPUT), text);
  const data = buildSpikeData();
  console.log(`PASS: wrote ${OUTPUT} (${data.creatures.length} creatures, `
    + `${Object.keys(data.moves).length} moves, ${data.font.glyphs.length} glyphs, `
    + `${text.length} bytes)`);
}
