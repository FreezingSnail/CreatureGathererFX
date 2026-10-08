#!/usr/bin/env node
// Browser-only traversal/scale review scenes; no runtime game data changes.
import fs from 'node:fs';
import { decodePng, encodePng } from './creature-sprite-export.mjs';
const root = 'docs/assets/world-environment-spike';
const atlas = decodePng(fs.readFileSync(`${root}/atlas-native.png`));
const players = Object.fromEntries(['DOWN', 'UP', 'LEFT', 'RIGHT'].map(d => [d, decodePng(fs.readFileSync(`docs/assets/world-player/${d.toLowerCase()}.png`))]));
const npcSheet = decodePng(fs.readFileSync('docs/assets/world-npcs/sprites16.png'));
const gid = (x, y) => y * 16 + x + 1;
function scene(name, start) { return { name, width: 16, height: 8, data: Array(128).fill(1), walkable: Array(128).fill(true), player: start, facing: 'UP', doors: [] }; }
function put(s, x, y, tile, walkable = true) { if (x < 0 || x >= 16 || y < 0 || y >= 8) throw Error('Invalid scene cell'); s.data[y * 16 + x] = tile; s.walkable[y * 16 + x] = walkable; }
function stamp(s, x, y, sx, sy, width, height, walkable = false) { for (let dy = 0; dy < height; dy++) for (let dx = 0; dx < width; dx++) put(s, x + dx, y + dy, gid(sx + dx, sy + dy), walkable); }
const village = scene('Village lane', [8, 3]);
for (let x = 0; x < 16; x++) put(village, x, 4, 2);
stamp(village, 5, 2, 0, 18, 3, 2); village.doors.push({ name: 'Cottage', x: 6, y: 3, approach: [6, 4] });
stamp(village, 11, 1, 4, 18, 3, 2); village.doors.push({ name: 'Cabin', x: 12, y: 2, approach: [12, 3] });
const bridge = scene('One-tile footbridge', [4, 4]); bridge.facing = 'RIGHT';
for (let y = 0; y < 8; y++) for (let x = 6; x < 10; x++) put(bridge, x, y, 3, false);
for (let x = 0; x < 16; x++) if (x < 6 || x >= 10) put(bridge, x, 4, 2);
stamp(bridge, 6, 4, 0, 23, 4, 1, true);
const woodland = scene('Woodland gap', [3, 4]); woodland.facing = 'RIGHT';
for (const [x, y] of [[4,1],[8,1],[4,5],[8,5]]) stamp(woodland, x, y, 1, 26, 2, 2);
stamp(woodland, 11, 4, 9, 27, 2, 1);
const scenes = [village, bridge, woodland];
function npc(s, x, y, sprite, name, message) {
  (s.npcs ||= []).push({ x, y, sprite, name, message }); s.walkable[y * s.width + x] = false;
}
npc(village, 9, 4, 2, 'Shopkeeper', 'Supplies are just up the lane.');
npc(village, 4, 4, 1, 'Farmer', 'Check the meadow for gathering spots.');
npc(bridge, 11, 3, 4, 'Fisher', 'The bridge is one walking tile wide.');
npc(woodland, 10, 3, 5, 'Ranger', 'Follow the gap between the trees.');
npc(woodland, 2, 5, 7, 'Child', 'I saw a creature near the grove.');
function render(s) {
  const out = { width: 256, height: 128, pixels: new Uint8Array(256 * 128 * 4) };
  s.data.forEach((id, i) => { const sx = (id - 1) % 16 * 16, sy = Math.floor((id - 1) / 16) * 16;
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) { const si = ((sy + y) * 256 + sx + x) * 4; out.pixels.set(atlas.pixels.subarray(si, si + 4), ((Math.floor(i / 16) * 16 + y) * 256 + i % 16 * 16 + x) * 4); }
  });
  for (const n of s.npcs || []) for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
    const si = ((Math.floor(n.sprite / 4) * 16 + y) * 64 + n.sprite % 4 * 16 + x) * 4;
    if (npcSheet.pixels[si + 3]) out.pixels.set(npcSheet.pixels.subarray(si, si + 4), ((n.y * 16 + y) * 256 + n.x * 16 + x) * 4);
  }
  return out;
}
for (let index = 0; index < scenes.length; index++) {
  const s = scenes[index], player = players[s.facing], full = render(s), frame = { width: 128, height: 64, pixels: new Uint8Array(128 * 64 * 4).fill(255) };
  const cx = s.player[0] * 16 - 56, cy = s.player[1] * 16 - 24;
  for (let y = 0; y < 64; y++) for (let x = 0; x < 128; x++) if (x + cx >= 0 && x + cx < 256 && y + cy >= 0 && y + cy < 128) {
    const si = ((y + cy) * 256 + x + cx) * 4; frame.pixels.set(full.pixels.subarray(si, si + 4), (y * 128 + x) * 4);
  }
  for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) { const si = (y * 16 + x) * 4; if (player.pixels[si + 3]) frame.pixels.set(player.pixels.subarray(si, si + 4), ((24 + y) * 128 + 56 + x) * 4); }
  fs.writeFileSync(`${root}/traversal-${index}.png`, encodePng(frame));
}
fs.writeFileSync(`${root}/traversal-scenes.json`, JSON.stringify({ reviewOnly: true, screen: [128, 64], tileSize: 16, playerScreenOrigin: [56, 24], scenes }, null, 2) + '\n');
fs.writeFileSync(`${root}/traversal-scenes.js`, 'window.WORLD_TRAVERSAL_SCENES = ' + JSON.stringify(scenes) + ';\n');
console.log('PASS: three16x8 traversal scenes, collision/door review data, native128x64 camera previews and local browser data.');
