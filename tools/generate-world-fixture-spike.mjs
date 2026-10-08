#!/usr/bin/env node
// Review-only fixture atlas and tilemap. Does not write installed game assets.
import fs from 'node:fs';
import path from 'node:path';
import { decodePng, encodePng } from './creature-sprite-export.mjs';
const parent = 'docs/assets/world-environment-spike', root = `${parent}/fixtures`;
fs.mkdirSync(root, { recursive: true });
const source = process.argv[2] || `${root}/atlas-concept.png`;
const raw = decodePng(fs.readFileSync(source));
if (raw.width !== raw.height) throw Error('Fixture atlas must be square');
const classic = process.argv.includes('--classic');
const correctionPath = process.argv[3] === '--classic' ? null : process.argv[3], correction = correctionPath ? decodePng(fs.readFileSync(correctionPath)) : null;
if (correction && correction.width !== correction.height) throw Error('Building correction must be square');
if (correctionPath && path.resolve(correctionPath) !== path.resolve(`${root}/buildings-correction.png`)) fs.copyFileSync(correctionPath, `${root}/buildings-correction.png`);
if (path.resolve(source) !== path.resolve(`${root}/atlas-concept.png`)) fs.copyFileSync(source, `${root}/atlas-concept.png`);
const atlas = { width: 256, height: 256, pixels: new Uint8Array(256 * 256 * 4) };
for (let y = 0; y < 256; y++) for (let x = 0; x < 256; x++) {
  const input = correction && y < 64 ? correction : raw;
  const sx = Math.floor((x + .5) * input.width / 256), sy = Math.floor((y + .5) * input.height / 256), si = (sy * input.width + sx) * 4;
  const v = input.pixels[si] * 299 + input.pixels[si + 1] * 587 + input.pixels[si + 2] * 114 >= 128000 ? 255 : 0;
  atlas.pixels.set([v, v, v, 255], (y * 256 + x) * 4);
}
const footprints = [[3,2,0,2],[3,2,0,2],[3,2,0,2],[3,2,0,2],[4,1,0,3],[1,4,1,0],[3,2,0,2],[2,2,1,2],[2,2,1,2],[3,2,0,2],[2,1,1,3],[2,2,1,2],[2,2,1,2],[1,4,1,0],[2,2,1,2],[3,2,0,2]];
if (classic) {
  // Generated row spacing is not an ABI. Locate complete subject bands before
  // fitting each original into its declared native footprint; never cut a roof.
  const sourcePixels = atlas.pixels.slice(), bands = []; let start = -1;
  for (let y = 0; y <= 256; y++) {
    let occupied = false;
    for (let x = 0; x < 256 && y < 256; x++) if (sourcePixels[(y * 256 + x) * 4] === 0) { occupied = true; break; }
    if (occupied && start < 0) start = y;
    if (!occupied && start >= 0) { bands.push([start, y]); start = -1; }
  }
  if (bands.length !== 4) throw Error(`Expected four separated fixture rows, found ${bands.length}`);
  const water = decodePng(fs.readFileSync(`${parent}/v3/atlas-native.png`));
  atlas.pixels.fill(255);
  for (let index = 0; index < 16; index++) {
    const bx = index % 4 * 64, by = Math.floor(index / 4) * 64, band = bands[Math.floor(index / 4)];
    let left = bx + 64, right = -1, top = band[1], bottom = -1;
    for (let y = band[0]; y < band[1]; y++) for (let x = bx; x < bx + 64; x++) if (sourcePixels[(y * 256 + x) * 4] === 0) {
      left = Math.min(left, x); right = Math.max(right, x); top = Math.min(top, y); bottom = Math.max(bottom, y);
    }
    if (right < left || bottom < top) throw Error(`Empty fixture ${index}`);
    if ([4,5,12,13].includes(index)) for (let y = 0; y < 64; y++) for (let x = 0; x < 64; x++) {
      const si = ((y % 16) * 256 + 32 + x % 16) * 4;
      atlas.pixels.set(water.pixels.subarray(si, si + 4), ((by + y) * 256 + bx + x) * 4);
    }
    if ([4,5,13].includes(index)) {
      // Extract the white deck from the water, retaining rails and plank art.
      const horizontal = index === 4, low = horizontal ? top : left, high = horizontal ? bottom : right;
      const majors = [];
      for (let n = low; n <= high; n++) {
        let white = 0; const length = horizontal ? right - left + 1 : bottom - top + 1;
        for (let k = horizontal ? left : top; k <= (horizontal ? right : bottom); k++) {
          const x = horizontal ? k : n, y = horizontal ? n : k;
          if (sourcePixels[(y * 256 + x) * 4] === 255) white++;
        }
        if (white >= length * .45) majors.push(n);
      }
      if (!majors.length) throw Error(`Missing bridge deck ${index}`);
      if (horizontal) { top = majors[0]; bottom = majors.at(-1); } else { left = majors[0]; right = majors.at(-1); }
    }
    const [tw, th, ox, oy] = footprints[index];
    for (let y = 0; y < th * 16; y++) for (let x = 0; x < tw * 16; x++) {
      const sx = left + Math.min(right - left, Math.floor((x + .5) * (right - left + 1) / (tw * 16)));
      const sy = top + Math.min(bottom - top, Math.floor((y + .5) * (bottom - top + 1) / (th * 16)));
      const si = (sy * 256 + sx) * 4;
      atlas.pixels.set(sourcePixels.subarray(si, si + 4), ((by + oy * 16 + y) * 256 + bx + ox * 16 + x) * 4);
    }
  }
}
fs.writeFileSync(`${root}/atlas-native.png`, encodePng(atlas));
// Source snapshot makes the refinement reproducible without altering earlier art.
const base = decodePng(fs.readFileSync(`${parent}/v3/atlas-native.png`));
for (let y = 0; y < 64; y++) base.pixels.set(atlas.pixels.subarray(y * 256 * 4, (y + 1) * 256 * 4), (128 + y) * 256 * 4);
fs.writeFileSync(`${parent}/atlas-native.png`, encodePng(base));
const names = ['Cottage', 'Woodland cabin', 'Village shop', 'Ruined watchtower', 'Horizontal bridge', 'Vertical bridge', 'Pond and shoreline', 'Rock cascade', 'Broadleaf tree', 'Tree grove', 'Boulder cluster', 'Shrine arch', 'Fishing dock', 'Marsh boardwalk', 'Gathering camp', 'Well and garden'];
const assemblies = names.map((name, index) => ({ name, x: index % 4 * 4, y: Math.floor(index / 4) * 4, width: 4, height: 4,
  ...(classic ? { targetFootprintTiles: footprints[index].slice(0, 2), footprintOffsetTiles: footprints[index].slice(2) } : {}) }));
const map = { type: 'map', version: '1.10', tiledversion: '1.10.2', orientation: 'orthogonal', renderorder: 'right-down', infinite: false,
  width: 32, height: 32, tilewidth: 16, tileheight: 16, nextlayerid: 3, nextobjectid: 1,
  tilesets: [{ firstgid: 1, name: 'Environment', image: '../atlas-native.png', imagewidth: 256, imageheight: 256, tilewidth: 16, tileheight: 16, columns: 16, tilecount: 256, margin: 0, spacing: 0 },
    { firstgid: 257, name: 'Larger fixtures', image: 'atlas-native.png', imagewidth: 256, imageheight: 256, tilewidth: 16, tileheight: 16, columns: 16, tilecount: 256, margin: 0, spacing: 0 }],
  layers: [{ id: 1, name: 'Assemblies', type: 'tilelayer', x: 0, y: 0, width: 32, height: 32, visible: true, opacity: 1, data: Array(1024).fill(1) },
    { id: 2, name: 'Fixture locations', type: 'objectgroup', visible: true, opacity: 1, objects: [] }] };
function put(x, y, gid) { if (x < 0 || x >= 32 || y < 0 || y >= 32 || gid < 1 || gid > 512) throw Error('Invalid fixture tile'); map.layers[0].data[y * 32 + x] = gid; }
for (let index = 0; index < 16; index++) {
  const x = index % 4 * 8, y = Math.floor(index / 4) * 8;
  for (let dy = 0; dy < 8; dy++) for (let dx = 0; dx < 8; dx++) {
    let gid = [7, 10, 11].includes(index) ? 65 : index === 13 ? 49 : 1;
    if (index < 4 && dy === 5) gid = 2;
    if (index === 4) { if (classic ? dy === 5 : dy === 3 || dy === 4) gid = 2; if (dx >= 2 && dx <= 5) gid = 3; }
    if (index === 5) { if (classic ? dx === 3 : dx === 3 || dx === 4) gid = 2; if (dy >= 2 && dy <= 5) gid = 3; }
    if (index === 12 && dx >= 4) gid = 3;
    if (index === 13 && dx >= 1 && dx <= 6 && dy >= 1 && dy <= 6) gid = 51;
    put(x + dx, y + dy, gid);
  }
  const assembly = assemblies[index], stampY = index < 4 ? 1 : 2;
  for (let dy = 0; dy < 4; dy++) for (let dx = 0; dx < 4; dx++) put(x + 2 + dx, y + stampY + dy, 257 + (assembly.y + dy) * 16 + assembly.x + dx);
  map.layers[1].objects.push({ id: map.nextobjectid++, name: names[index], type: 'Fixture review', x: x * 16, y: y * 16, width: 128, height: 128, rotation: 0, visible: true });
}
function render(m) {
  const image = { width: m.width * 16, height: m.height * 16, pixels: new Uint8Array(m.width * m.height * 256 * 4) };
  m.layers[0].data.forEach((gid, index) => {
    const fixture = gid >= 257, tile = gid - (fixture ? 257 : 1), src = fixture ? atlas : base;
    const sx = tile % 16 * 16, sy = Math.floor(tile / 16) * 16, dx = index % m.width * 16, dy = Math.floor(index / m.width) * 16;
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const si = ((sy + y) * 256 + sx + x) * 4;
      image.pixels.set(src.pixels.subarray(si, si + 4), ((dy + y) * image.width + dx + x) * 4);
    }
  });
  return image;
}
fs.writeFileSync(`${root}/overworld.tmj`, JSON.stringify(map, null, 2) + '\n');
fs.writeFileSync(`${root}/overworld.png`, encodePng(render(map)));
for (const name of ['overworld', 'interiors']) {
  const m = JSON.parse(fs.readFileSync(`${parent}/${classic ? 'v5/' : ''}${name}.tmj`));
  if (classic && name === 'overworld') for (const object of m.layers[1].objects) {
    if (['Cottage', 'Woodland cabin', 'Village shop', 'Stone tower', 'Village cottage'].includes(object.name)) {
      object.y += 16; object.width = 48; object.height = 32;
    }
  }
  fs.writeFileSync(`${parent}/${name}.tmj`, JSON.stringify(m, null, 2) + '\n');
  fs.writeFileSync(`${parent}/${name}.png`, encodePng(render(m)));
}
const mainManifest = JSON.parse(fs.readFileSync(`${parent}/manifest.json`));
mainManifest.buildingRevision = { source: 'fixtures/atlas-native.png', sourceRectangle: [0, 0, 256, 64], exteriorTiles: classic ? [3, 2] : [4, 3], atlasSlotTiles: [4, 4], exteriorOffsetTiles: classic ? [0, 2] : [0, 1], doorTile: [1, 3], approachTile: [1, 4], interaction: 'A facing UP; review only' };
fs.writeFileSync(`${parent}/manifest.json`, JSON.stringify(mainManifest, null, 2) + '\n');
fs.writeFileSync(`${root}/manifest.json`, JSON.stringify({ reviewOnly: true, tileSize: 16, atlasSize: [256, 256], tileCount: 256, assemblies, map: 'overworld.tmj', firstgid: 257,
  buildingCorrection: correction ? { source: 'buildings-correction.png', acceptedRectangle: [0, 0, 256, 64] } : null,
  export: 'Centered nearest-neighbor to256x256; binary threshold128. Assembly boundaries and bridge repeat seams require art review.', rawPixelBytes1bpp: 8192 }, null, 2) + '\n');
// Publish one atlas. Appending preserves every existing global tile ID.
const combined = { width: 256, height: 512, pixels: new Uint8Array(256 * 512 * 4) };
combined.pixels.set(base.pixels);
combined.pixels.set(atlas.pixels, base.pixels.length);
fs.writeFileSync(`${parent}/atlas-native.png`, encodePng(combined));
for (const file of ['overworld', 'interiors', 'fixtures/overworld']) {
  const m = JSON.parse(fs.readFileSync(`${parent}/${file}.tmj`));
  m.tilesets = [{ firstgid: 1, name: 'Environment and fixtures', image: file.startsWith('fixtures/') ? '../atlas-native.png' : 'atlas-native.png',
    imagewidth: 256, imageheight: 512, tilewidth: 16, tileheight: 16, columns: 16, tilecount: 512, margin: 0, spacing: 0 }];
  fs.writeFileSync(`${parent}/${file}.tmj`, JSON.stringify(m, null, 2) + '\n');
}
mainManifest.atlasSize = [256, 512]; mainManifest.tileCount = 512;
mainManifest.fixtureStartId = 257;
mainManifest.fixtureAssemblies = assemblies.map(a => ({ ...a, y: a.y + 16 }));
mainManifest.buildingRevision.source = 'atlas-native.png';
mainManifest.buildingRevision.sourceRectangle = [0, 256, 256, 64];
fs.writeFileSync(`${parent}/manifest.json`, JSON.stringify(mainManifest, null, 2) + '\n');
const fixtureManifest = JSON.parse(fs.readFileSync(`${root}/manifest.json`));
fixtureManifest.style = classic ? 'Compact classic top-down, player-relative footprints' : 'Refined larger fixtures';
fixtureManifest.atlas = '../atlas-native.png'; fixtureManifest.atlasSize = [256, 512]; fixtureManifest.tileCount = 512;
fixtureManifest.assemblies = assemblies.map(a => ({ ...a, y: a.y + 16 }));
fixtureManifest.componentPreview = 'atlas-native.png';
fs.writeFileSync(`${root}/manifest.json`, JSON.stringify(fixtureManifest, null, 2) + '\n');
console.log('PASS: one256x512 atlas,512tiles; all three maps use one tileset with unchanged global IDs.');
