#!/usr/bin/env node
// Export generated review artwork to native cells; no installed sprite changes.
import fs from 'node:fs';
import { decodePng, encodePng } from './creature-sprite-export.mjs';
const player = process.argv.includes('--player');
const walk = player && process.argv.includes('--walk');
const frontBack = walk && process.argv.includes('--front-back');
const root = player ? 'docs/assets/world-player' + (walk ? '/walk' : '') + (frontBack ? '/front-back' : '') : 'docs/assets/world-npcs';
const source = decodePng(fs.readFileSync(`${root}/concept.png`));
const names = player ? walk ? (frontBack ? ['DOWN_A', 'DOWN_B', 'UP_A', 'UP_B'] : ['DOWN_A', 'DOWN_B', 'UP_A', 'UP_B', 'LEFT_A', 'LEFT_B', 'RIGHT_A', 'RIGHT_B']) : ['DOWN', 'UP', 'LEFT', 'RIGHT'] : ['Gatherer', 'Farmer', 'Shopkeeper', 'Herbalist', 'Fisher', 'Ranger', 'Miner', 'Child'];
const columns = player && (!walk || frontBack) ? 2 : 4;
const sheet = { width: columns * 16, height: 32, pixels: new Uint8Array(columns * 16 * 32 * 4) };
const entries = [];
for (let index = 0; index < names.length; index++) {
  const col = index % columns, row = Math.floor(index / columns);
  const x0 = Math.floor(col * source.width / columns), x1 = Math.floor((col + 1) * source.width / columns);
  const y0 = Math.floor(row * source.height / 2), y1 = Math.floor((row + 1) * source.height / 2);
  let left = x1, right = x0, top = y1, bottom = y0;
  for (let y = y0; y < y1; y++) for (let x = x0; x < x1; x++) {
    const i = (y * source.width + x) * 4;
    if (source.pixels[i + 3] >= 128 && source.pixels[i] + source.pixels[i + 1] + source.pixels[i + 2] < 384) {
      left = Math.min(left, x); right = Math.max(right, x); top = Math.min(top, y); bottom = Math.max(bottom, y);
    }
  }
  if (left > right) throw Error(`Missing sprite ${names[index]}`);
  const width = right - left + 1, height = bottom - top + 1;
  // Reserve one pixel on every side for an unclipped white outline.
  const scale = Math.min(14 / width, 14 / height), w = Math.max(1, Math.round(width * scale)), h = Math.max(1, Math.round(height * scale));
  const ox = Math.floor((16 - w) / 2), oy = 15 - h;
  let sprite = { width: 16, height: 16, pixels: new Uint8Array(16 * 16 * 4).fill(255) };
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
    let sum = 0, count = 0;
    for (let sy = Math.floor(top + y * height / h); sy < Math.ceil(top + (y + 1) * height / h); sy++)
      for (let sx = Math.floor(left + x * width / w); sx < Math.ceil(left + (x + 1) * width / w); sx++) {
        const si = (sy * source.width + sx) * 4, alpha = source.pixels[si + 3] / 255;
        sum += alpha * (source.pixels[si] + source.pixels[si + 1] + source.pixels[si + 2]) / 3 + (1 - alpha) * 255; count++;
      }
    const value = sum / count < 170 ? 0 : 255;
    sprite.pixels.set([value, value, value, 255], ((oy + y) * 16 + ox + x) * 4);
  }
  // Front/back walk poses use the native idle torso and foot columns so that
  // downsampling cannot resize the head or merge both feet into one blob.
  if (frontBack) {
    const direction = names[index].split('_')[0].toLowerCase();
    const idle = decodePng(fs.readFileSync(`docs/assets/world-player/${direction}.png`));
    sprite.pixels.fill(255);
    sprite.pixels.set(idle.pixels.subarray(0, 13 * 16 * 4));
    for (let x = 1; x < 15; x++) {
      const foot = (14 * 16 + x) * 4;
      if (idle.pixels[foot] !== 0 || !idle.pixels[foot + 3]) continue;
      const forward = (x < 8) === names[index].endsWith('_A');
      for (let y = 13; y <= (forward ? 13 : 14); y++)
        sprite.pixels.set([0, 0, 0, 255], (y * 16 + x) * 4);
    }
  }
  // Flood only exterior white: enclosed face/clothing pixels remain opaque.
  const exterior = new Uint8Array(256), queue = [];
  function visit(x, y) {
    if (x < 0 || x >= 16 || y < 0 || y >= 16) return;
    const i = y * 16 + x;
    if (!exterior[i] && sprite.pixels[i * 4] === 255) { exterior[i] = 1; queue.push(i); }
  }
  for (let i = 0; i < 16; i++) { visit(i, 0); visit(i, 15); visit(0, i); visit(15, i); }
  for (let q = 0; q < queue.length; q++) {
    const x = queue[q] % 16, y = Math.floor(queue[q] / 16);
    visit(x - 1, y); visit(x + 1, y); visit(x, y - 1); visit(x, y + 1);
  }
  // Eight-neighbor dilation includes corners and outlines every separate tool.
  for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
    const i = y * 16 + x; let opaque = !exterior[i];
    for (let dy = -1; dy <= 1 && !opaque; dy++) for (let dx = -1; dx <= 1; dx++) {
      const nx = x + dx, ny = y + dy;
      if (nx >= 0 && nx < 16 && ny >= 0 && ny < 16 && !exterior[ny * 16 + nx]) opaque = true;
    }
    sprite.pixels[i * 4 + 3] = opaque ? 255 : 0;
  }
  if (walk && !frontBack && index < 4) {
    const fixed = `${root}/front-back/${names[index].toLowerCase()}.png`;
    if (fs.existsSync(fixed)) sprite = decodePng(fs.readFileSync(fixed));
  }
  const file = names[index].toLowerCase() + '.png'; fs.writeFileSync(`${root}/${file}`, encodePng(sprite));
  for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
    const si = (y * 16 + x) * 4; sheet.pixels.set(sprite.pixels.subarray(si, si + 4), ((row * 16 + y) * sheet.width + col * 16 + x) * 4);
  }
  entries.push({ name: names[index], file, cell: [col, row], size: [16, 16], sourceBounds: [left, top, width, height] });
}
fs.writeFileSync(`${root}/sprites16.png`, encodePng(sheet));
const preview = { width: sheet.width * 8, height: sheet.height * 8, pixels: new Uint8Array(sheet.width * sheet.height * 64 * 4) };
for (let y = 0; y < preview.height; y++) for (let x = 0; x < preview.width; x++) {
  const si = (Math.floor(y / 8) * sheet.width + Math.floor(x / 8)) * 4;
  preview.pixels.set(sheet.pixels.subarray(si, si + 4), (y * preview.width + x) * 4);
}
fs.writeFileSync(`${root}/preview8x.png`, encodePng(preview));
fs.writeFileSync(`${root}/manifest.json`, JSON.stringify({ reviewOnly: true, palette: ['#000000', '#ffffff'], background: 'transparent', outline: { color: '#ffffff', pixels: 1, neighbors: 8 }, sheet: [sheet.width, sheet.height], facing: player ? names : 'DOWN', sprites: entries }, null, 2) + '\n');
console.log(`PASS: ${names.length} binary16x16 ${player ? 'player directions' : 'NPCs'} in one${sheet.width}x${sheet.height} sheet; review assets only.`);
