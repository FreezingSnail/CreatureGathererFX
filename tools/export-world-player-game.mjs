#!/usr/bin/env node
// Build the authored PNG input for make gen; no generated FX files are written.
import fs from 'node:fs';
import { decodePng, encodePng } from './creature-sprite-export.mjs';
const root = 'docs/assets/world-player';
// Matches Direction's permanent UP,RIGHT,DOWN,LEFT values: three poses each.
const frames = ['up', 'right', 'down', 'left'].flatMap(d => [`${d}.png`, `walk/${d}_a.png`, `walk/${d}_b.png`]);
const sheet = { width: 96, height: 32, pixels: new Uint8Array(96 * 32 * 4) };
frames.forEach((file, index) => {
  const frame = decodePng(fs.readFileSync(`${root}/${file}`));
  if (frame.width !== 16 || frame.height !== 16) throw Error(`${file}: expected16x16`);
  for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
    const si = (y * 16 + x) * 4, di = ((Math.floor(index / 6) * 16 + y) * 96 + index % 6 * 16 + x) * 4;
    sheet.pixels.set(frame.pixels.subarray(si, si + 4), di);
  }
});
fs.writeFileSync('images/Playerchibi_16x16.png', encodePng(sheet));
console.log('PASS: twelve masked16x16 player sourceframes,Direction order,96x32PNG. Run make gen to pack.');
