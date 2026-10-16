#!/usr/bin/env node
// Generate mock/data.js from authored sources.
// Inputs: data/json/creatures.json, data/typetable.csv, data/duel.toml (fold),
//         docs/assets/battle-sprites48/manifest.json, images/ArduFontTrimmed_5x6.png
// Run: node tools/battle-design-spike-data.mjs
// Test: node --test tools/tests/battle-design-spike-data_test.mjs
//
// Kits are the authored 8-type `moves` name list (CreatureGathererFX-4w89.6);
// the packed move table is gone. Each creature also carries the frozen 5-bit
// duel `attackMask` (bits 0..4 = [spirit, water, earth, fire, plant]) that the
// device kernel reads.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { decodePng } from './creature-sprite-export.mjs';
import {
  chartFromEdges,
  triangles,
  unsatisfiedDemands,
  rosterSets,
} from './battle-matchup-solver.mjs';
import { WHEEL } from './battle-wheel.mjs';

export { WHEEL };

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

// 8-type matchup web for the simultaneous-pick duel: a directed graph of
// "beats" edges made of overlapping 3-cycles; undecided pairs stay neutral.
// Edges live in tools/battle-wheel.mjs; the solver
// (tools/battle-matchup-solver.mjs) proves every roster move-set pair has at
// least one positive matchup and proposes fixes for violations.

// Frozen 5-type duel fold (docs/moveset-type-mask-brief.md, data/duel.toml
// [fold]). Duplicated here so the spike does not parse TOML; the parity test
// pins it against the device kernel fold.
export const DUEL = {
  order: ['spirit', 'water', 'earth', 'fire', 'plant'],
  map: {
    spirit: 'spirit', elder: 'spirit',
    water: 'water', wind: 'water',
    earth: 'earth',
    fire: 'fire', lightning: 'fire',
    plant: 'plant',
  },
};

export function duelMask(moveTypes) {
  let mask = 0;
  for (const type of moveTypes) {
    const folded = DUEL.map[type];
    if (!folded) throw Error(`no duel fold for authored type ${type}`);
    mask |= 1 << DUEL.order.indexOf(folded);
  }
  return mask;
}

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

function glyphBit(image, x, y) {
  return image.pixels[(y * image.width + x) * 4] >= 128 ? '1' : '0';
}

export function buildSpikeData() {
  const typeChart = parseTypeChart();
  const creaturesJson = readJson('data/json/creatures.json');
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
    const moves = [...new Set(creature.moves)];
    for (const type of moves)
      if (!(type in DUEL.map)) throw Error(`bad authored move type ${type} on ${creature.name}`);
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
      moves,
      attackMask: duelMask(moves),
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
    wheel: (() => {
      const chart = chartFromEdges(WHEEL.order, WHEEL.edges);
      const sets = rosterSets(creatures, WHEEL.order);
      const unsatisfied = unsatisfiedDemands(WHEEL.order, chart, sets);
      if (unsatisfied.length) {
        const first = unsatisfied.slice(0, 6)
          .map(({ a, b }) => `${a.name} [${a.types.join('/')}] vs ${b.name} [${b.types.join('/')}]`)
          .join('; ');
        throw Error(`wheel coverage: ${unsatisfied.length} set pairs have no positive matchup: ${first}`);
      }
      return {
        order: WHEEL.order,
        edges: WHEEL.edges,
        chart,
        cycles: triangles(WHEEL.order, chart),
      };
    })(),
    font: { firstChar, width: 5, height: 6, glyphs },
    creatures,
  };
}

export function serializeSpikeData(data) {
  return '// Generated by tools/battle-design-spike-data.mjs. Do not hand-edit.\n'
    + '// Sources: data/json/creatures.json, data/typetable.csv, docs/assets/battle-sprites48/manifest.json,\n'
    + '// images/ArduFontTrimmed_5x6.png. Kits are the authored 8-type moves list + 5-bit duel attackMask.\n'
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
    + `${data.font.glyphs.length} glyphs, ${text.length} bytes)`);
}
