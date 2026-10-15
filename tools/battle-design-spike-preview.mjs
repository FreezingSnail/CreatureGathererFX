#!/usr/bin/env node
// Render mock scenes to PNG previews for docs/index.html and review.
// Run: node tools/battle-design-spike-preview.mjs
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';
import { encodePng } from './creature-sprite-export.mjs';
import { createHarness, writePreview } from './battle-design-spike-frame.mjs';

const repo = path.dirname(fileURLToPath(import.meta.url)) + '/..';
const require = createRequire(import.meta.url);
const { ICONS, MERGED_ICONS, DATA } = require(path.join(repo, 'mock/battle-mock.js'));
const creatureIndex = (name) => DATA.creatures.findIndex((entry) => entry.name === name);

const harness = createHarness({ search: '?seed=11' }); // pinned seed: previews are reproducible
const settle = (frames = 3, step = 150) => harness.advances(frames, step);

// Type icon sheet (8 types + 4 merged placeholders) at 6x for legibility review.
const sheetIcons = [
  ['spirit', ICONS.spirit], ['water', ICONS.water], ['wind', ICONS.wind], ['earth', ICONS.earth],
  ['fire', ICONS.fire], ['lightning', ICONS.lightning], ['plant', ICONS.plant], ['elder', ICONS.elder],
  ['flame', MERGED_ICONS.flame], ['bloom', MERGED_ICONS.bloom], ['tide', MERGED_ICONS.tide], ['stone', MERGED_ICONS.stone],
];
const scale = 6, cell = 8 * scale, gap = 10;
const sheetW = sheetIcons.length * cell + (sheetIcons.length - 1) * gap;
const sheet = new Uint8Array(sheetW * cell * 4);
for (let i = 0; i < sheetW * cell; i++) sheet.set([0, 0, 0, 255], i * 4); // black background
for (let index = 0; index < sheetIcons.length; index++) {
  const bits = sheetIcons[index][1];
  const ox = index * (cell + gap);
  for (let y = 0; y < 8; y++) for (let x = 0; x < 8; x++) {
    if (bits[y][x] !== '#') continue;
    for (let sy = 0; sy < scale; sy++) for (let sx = 0; sx < scale; sx++) {
      const px = ox + x * scale + sx, py = y * scale + sy;
      sheet.set([255, 255, 255, 255], (py * sheetW + px) * 4);
    }
  }
}
const iconSheet = path.join(repo, 'docs/assets/battle-design-spike/icons.png');
fs.mkdirSync(path.dirname(iconSheet), { recursive: true });
fs.writeFileSync(iconSheet, encodePng({ width: sheetW, height: cell, pixels: sheet }));

// Setup screen (mode A).
settle();
writePreview(harness, 'setup.png');

// Mode A battle: move choice.
harness.press('z');
settle();
writePreview(harness, 'current-choice.png');

// Mode B: 4-type single with arrows, foe HP and press-turn.
harness.switchMode('trim');
harness.press('z');
settle();
writePreview(harness, 'trim-choice.png');

// Advance one attack step (feedback footer + damage + possible EXTRA).
harness.press('z');
harness.press('z');
settle();
writePreview(harness, 'trim-feedback.png');

// Mode B with 8-type dual (current data model + arrows for contrast).
harness.setControl('trim-type', '8dual');
harness.press('x');
harness.press('z');
settle();
writePreview(harness, 'trim-dual-choice.png');

// Mode C: stance duel (4-type cycle) in its own harness.
const stancesHarness = createHarness({ mode: 'duel', duel: 'stances', search: '?seed=12' });
stancesHarness.advances(2);
stancesHarness.press('z');
stancesHarness.advances(2);
writePreview(stancesHarness, 'stances-choice.png');
stancesHarness.press('z');
stancesHarness.advances(8, 100);
writePreview(stancesHarness, 'stances-result.png');

// Mode D: wheel duel (8 types, own moves, power scoring).
harness.switchMode('duel');
harness.press('z');
settle();
writePreview(harness, 'duel-choice.png');

// Duel reveal -> result (boot clamps frame dt to 120 ms, so advance in slices).
harness.press('z');
harness.advances(8, 100);
writePreview(harness, 'duel-result.png');

// Sample-team frame: 5-type table, balanced vs anchors presets.
const teamHarness = createHarness({ mode: 'duel', duel: 'wheel', scoring: 'bonus', search: '?seed=21&matrix=5' });
teamHarness.advances(2);
teamHarness.setControl('team-you', 'balanced');
teamHarness.setControl('team-foe', 'anchors');
teamHarness.press('z');
teamHarness.advances(2);
writePreview(teamHarness, 'duel-teams.png');

// Mode E: pure RPS (same web, flat scoring) in its own harness.
// Mirror matchup (ember vs ember) so the trade frame is unmistakable on the card.
const pureHarness = createHarness({ mode: 'duel', duel: 'wheel', scoring: 'flat', search: '?seed=13' });
pureHarness.advances(2);
pureHarness.state().foeIndex = pureHarness.state().youIndex;
pureHarness.press('z');
pureHarness.advances(2);
writePreview(pureHarness, 'duel-pure-choice.png');
pureHarness.press('z');
pureHarness.state().duelState.picks = [0, 0]; // both fire -> trade
pureHarness.advances(8, 100);
writePreview(pureHarness, 'duel-pure-result.png');

// Mode F: creature-type bonus scoring, water attacker vs fire target so the x1.5 bonus fires.
const bonusHarness = createHarness({ mode: 'duel', duel: 'wheel', scoring: 'bonus', search: '?seed=14' });
bonusHarness.advances(2);
const bonusState = bonusHarness.state();
bonusState.youIndex = creatureIndex('bigsquid');
bonusState.foeIndex = creatureIndex('ember');
bonusHarness.press('z');
bonusHarness.advances(2);
writePreview(bonusHarness, 'duel-bonus-choice.png');
bonusHarness.press('z');
bonusHarness.state().duelState.picks = [0, 0]; // splash vs stoke: water beats fire, creature bonus
bonusHarness.advances(8, 100);
writePreview(bonusHarness, 'duel-bonus-result.png');

console.log('PASS: wrote previews and icons.png to docs/assets/battle-design-spike/');
