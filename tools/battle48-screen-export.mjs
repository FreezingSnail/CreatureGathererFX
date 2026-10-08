#!/usr/bin/env node
// Convert actual device framebuffer captures to native and nearest-neighbor previews.
import fs from 'node:fs';
import path from 'node:path';
import { encodePng } from './creature-sprite-export.mjs';
const input = process.argv[2] ?? 'build/battle48-device/screens.log';
const output = 'docs/assets/battle48-spike';
fs.mkdirSync(output, { recursive: true });
let count = 0;
for (const match of fs.readFileSync(input, 'utf8').matchAll(/SCREEN:([a-z-]+):([0-9A-F]{2048})/g)) {
  const bytes = Buffer.from(match[2], 'hex');
  for (const scale of [1, 6]) {
    const width = 128 * scale, height = 64 * scale;
    const pixels = Buffer.alloc(width * height * 4);
    for (let y = 0; y < height; y++) for (let x = 0; x < width; x++) {
      const sx = Math.floor(x / scale), sy = Math.floor(y / scale);
      const value = bytes[(sy >> 3) * 128 + sx] & (1 << (sy & 7)) ? 255 : 0;
      pixels.set([value, value, value, 255], (y * width + x) * 4);
    }
    fs.writeFileSync(path.join(output, `${match[1]}${scale === 1 ? '' : '-6x'}.png`), encodePng({ width, height, pixels }));
  }
  count++;
}
if (count !== 4) throw Error(`Expected four framebuffer captures, found ${count}`);
console.log(`PASS: ${count} actual device screens exported at native128x64 and6x.`);
