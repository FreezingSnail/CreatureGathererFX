#!/usr/bin/env node
// Review-only environment atlas/map exports. Never writes installed game assets.
import fs from 'node:fs';
import path from 'node:path';
import { decodePng, encodePng } from './creature-sprite-export.mjs';

const root = 'docs/assets/world-environment-spike';
fs.mkdirSync(root, { recursive: true });
// Actual first player frame, inverted to black ink on transparent white terrain.
const playerSource = decodePng(fs.readFileSync('images/characterSheet_16x16.png'));
const playerReference = { width: 16, height: 16, pixels: new Uint8Array(16 * 16 * 4) };
for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
  const si = (y * playerSource.width + x) * 4;
  const ink = playerSource.pixels[si + 3] > 0 && playerSource.pixels[si] + playerSource.pixels[si + 1] + playerSource.pixels[si + 2] > 384;
  playerReference.pixels.set([0, 0, 0, ink ? 255 : 0], (y * 16 + x) * 4);
}
fs.writeFileSync(`${root}/player-reference.png`, encodePng(playerReference));
const conceptPath = process.argv[2] || `${root}/atlas-concept.png`;
const raw = decodePng(fs.readFileSync(conceptPath));
if (raw.width !== raw.height) throw Error('Environment concept must be square');
const correctionPath = process.argv[3];
const correction = correctionPath ? decodePng(fs.readFileSync(correctionPath)) : null;
if (correction && correction.width !== correction.height) throw Error('Terrain correction must be square');
if (correctionPath && path.resolve(correctionPath) !== path.resolve(`${root}/atlas-terrain-correction.png`)) {
  fs.copyFileSync(correctionPath, `${root}/atlas-terrain-correction.png`);
}
if (path.resolve(conceptPath) !== path.resolve(`${root}/atlas-concept.png`)) {
  fs.copyFileSync(conceptPath, `${root}/atlas-concept.png`);
}
const atlas = { width: 256, height: 256, pixels: new Uint8Array(256 * 256 * 4) };
for (let y = 0; y < 256; y++) for (let x = 0; x < 256; x++) {
  // Only accept corrected terrain; keep generated props and assemblies bit-identical.
  const source = correction && x < 64 && y < 128 ? correction : raw;
  const sx = Math.min(source.width - 1, Math.floor((x + .5) * source.width / 256));
  const sy = Math.min(source.height - 1, Math.floor((y + .5) * source.height / 256));
  const si = (sy * source.width + sx) * 4;
  const value = source.pixels[si] * 299 + source.pixels[si + 1] * 587 + source.pixels[si + 2] * 114 >= 128000 ? 255 : 0;
  atlas.pixels.set([value, value, value, 255], (y * 256 + x) * 4);
}
fs.writeFileSync(`${root}/atlas-native.png`, encodePng(atlas));
const buildingPath = process.argv[4];
if (buildingPath) {
  const buildings = decodePng(fs.readFileSync(buildingPath));
  if (buildings.width !== buildings.height) throw Error('Building sheet must be square');
  if (path.resolve(buildingPath) !== path.resolve(`${root}/buildings-simple-concept.png`)) fs.copyFileSync(buildingPath, `${root}/buildings-simple-concept.png`);
  for (let building = 0; building < 4; building++) for (let y = 0; y < 64; y++) for (let x = 0; x < 64; x++) {
    if (y < 16) { atlas.pixels.set([255, 255, 255, 255], ((128 + y) * 256 + building * 64 + x) * 4); continue; }
    // Exterior uses 4x3 tiles in its existing 4x4 slot. Preserve the full door row.
    const localY = y - 16;
    const sourceY = localY < 32 ? localY * 48 / 32 : 48 + localY - 32;
    const lx = (building % 2) * 64 + x, ly = Math.floor(building / 2) * 64 + sourceY;
    const sx = Math.floor((lx + .5) * buildings.width / 128), sy = Math.floor((ly + .5) * buildings.height / 128);
    const source = (sy * buildings.width + sx) * 4;
    const value = buildings.pixels[source] * 299 + buildings.pixels[source + 1] * 587 + buildings.pixels[source + 2] * 114 >= 128000 ? 255 : 0;
    atlas.pixels.set([value, value, value, 255], ((128 + y) * 256 + building * 64 + x) * 4);
  }
  fs.writeFileSync(`${root}/atlas-native.png`, encodePng(atlas));
}
const biomes = ['Meadow', 'Woodland', 'Coast', 'Marsh', 'Mountain', 'Snow', 'Village', 'Ruins'];
const buildings = ['Cottage', 'Woodland cabin', 'Village shop', 'Stone tower'];
const rooms = ['Cottage room', 'Shop room', 'Ruined shrine', 'Crystal cave'];
const id = (column, row) => row * 16 + column + 1; // Tiled: 0 empty, firstgid 1.
function map(width, height) {
  return { type: 'map', version: '1.10', tiledversion: '1.10.2', orientation: 'orthogonal', renderorder: 'right-down', infinite: false,
    width, height, tilewidth: 16, tileheight: 16, nextlayerid: 3, nextobjectid: 1,
    tilesets: [{ firstgid: 1, name: 'Environment spike', image: 'atlas-native.png', imagewidth: 256, imageheight: 256,
      tilewidth: 16, tileheight: 16, columns: 16, tilecount: 256, margin: 0, spacing: 0 }],
    layers: [{ id: 1, name: 'Environment', type: 'tilelayer', x: 0, y: 0, width, height, opacity: 1, visible: true, data: Array(width * height).fill(id(0, 0)) },
      { id: 2, name: 'Locations', type: 'objectgroup', opacity: 1, visible: true, objects: [] }] };
}
function put(m, x, y, tile) {
  if (x < 0 || y < 0 || x >= m.width || y >= m.height || tile < 1 || tile > 256) throw Error('Out-of-bounds map stamp');
  m.layers[0].data[y * m.width + x] = tile;
}
function block(m, x, y, column, row, width = 4, height = 4) {
  for (let dy = 0; dy < height; dy++) for (let dx = 0; dx < width; dx++) put(m, x + dx, y + dy, id(column + dx, row + dy));
}
function location(m, name, x, y, width, height) {
  m.layers[1].objects.push({ id: m.nextobjectid++, name, type: 'Review location', x: x * 16, y: y * 16, width: width * 16, height: height * 16, rotation: 0, visible: true });
}
function entrance(m, name, x, y, approachX, approachY) {
  m.layers[1].objects.push({ id: m.nextobjectid++, name, type: 'Entrance review', x: x * 16, y: y * 16, width: 16, height: 16, rotation: 0, visible: true,
    properties: [{ name: 'approachX', type: 'int', value: approachX }, { name: 'approachY', type: 'int', value: approachY },
      { name: 'facing', type: 'string', value: 'UP' }, { name: 'interaction', type: 'string', value: 'A while facing door; not installed' }] });
}
const overworld = map(32, 32);
const zones = biomes.map((name, row) => ({ name, x: (row % 2) * 16, y: Math.floor(row / 2) * 8, width: 16, height: 8, atlasRow: row }));
for (const zone of zones) {
  const { x, y, atlasRow: row } = zone;
  for (let dy = 0; dy < 8; dy++) for (let dx = 0; dx < 16; dx++) {
    // A continuous road crosses each pair of environments; central route joins rows.
    let tile = id(0, row);
    if (dy === 5 || dx === (x ? 0 : 15)) tile = id(1, row);
    if (dy === 0 && dx % 3 === 0 && row >= 4) tile = id(3, row);
    if (row === 2 && dx >= 9 && dy !== 5) tile = id(2, row);
    if ([0, 3, 4].includes(row) && dx >= 7 && dx <= 9 && dy >= 1 && dy <= 3) tile = id(2, row);
    put(overworld, x + dx, y + dy, tile);
  }
  for (const [dx, dy, column] of [[1, 1, 4], [3, 2, 5], [5, 1, 6], [1, 3, 7], [4, 4, 8], [6, 6, 9], [2, 6, 10], [10, 6, 11], [12, 6, 12], [14, 6, 13], [6, 3, 14], [4, 6, 15]]) {
    put(overworld, x + dx, y + dy, id(column, row));
  }
  location(overworld, zone.name, x, y, 16, 8);
  if (row === 0 || row === 1 || row === 6 || row === 7) {
    const building = row === 0 ? 0 : row === 1 ? 1 : row === 6 ? 2 : 3;
    block(overworld, x + 10, y + 1, building * 4, 8);
    location(overworld, buildings[building], x + 10, y + (buildingPath ? 2 : 1), 4, buildingPath ? 3 : 4);
    if (buildingPath) { put(overworld, x + 11, y + 5, id(1, row)); entrance(overworld, `${buildings[building]} door`, x + 11, y + 4, x + 11, y + 5); }
  }
  if (row === 6) { block(overworld, x + 1, y + 1, 0, 8); location(overworld, 'Village cottage', x + 1, y + (buildingPath ? 2 : 1), 4, buildingPath ? 3 : 4);
    if (buildingPath) { put(overworld, x + 2, y + 5, id(1, row)); entrance(overworld, 'Village cottage door', x + 2, y + 4, x + 2, y + 5); } }
}
const interiors = map(32, 8);
for (let room = 0; room < 4; room++) {
  for (let y = 0; y < 8; y++) for (let x = 0; x < 8; x++) put(interiors, room * 8 + x, y, id(0, [0, 6, 7, 4][room]));
  block(interiors, room * 8 + 2, 2, room * 4, 12);
  location(interiors, rooms[room], room * 8 + 2, 2, 4, 4);
}
function render(m) {
  const image = { width: m.width * 16, height: m.height * 16, pixels: new Uint8Array(m.width * m.height * 256 * 4) };
  m.layers[0].data.forEach((gid, index) => {
    const tile = gid - 1, sx = tile % 16 * 16, sy = Math.floor(tile / 16) * 16;
    const dx = index % m.width * 16, dy = Math.floor(index / m.width) * 16;
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const source = ((sy + y) * 256 + sx + x) * 4;
      image.pixels.set(atlas.pixels.subarray(source, source + 4), ((dy + y) * image.width + dx + x) * 4);
    }
  });
  return image;
}
for (const [name, m] of [['overworld', overworld], ['interiors', interiors]]) {
  fs.writeFileSync(`${root}/${name}.tmj`, JSON.stringify(m, null, 2) + '\n');
  fs.writeFileSync(`${root}/${name}.png`, encodePng(render(m)));
}
fs.writeFileSync(`${root}/manifest.json`, JSON.stringify({ reviewOnly: true, tileSize: 16, atlasSize: [256, 256], tileCount: 256, biomes: zones, buildings, rooms,
  conceptSize: [raw.width, raw.height], terrainCorrection: correction ? { source: 'atlas-terrain-correction.png', nativeRectangle: [0, 0, 64, 128] } : null,
  buildingRevision: buildingPath ? { source: 'buildings-simple-concept.png', exteriorTiles: [4, 3], atlasSlotTiles: [4, 4], exteriorOffsetTiles: [0, 1], playerTileSize: 16, doorTile: [1, 3], approachTile: [1, 4], interaction: 'A facing UP; review only' } : null,
  export: 'Centered nearest-neighbor sample and threshold128; opaque binary. Generated cell alignment and seams require review.',
  map: 'overworld.tmj', interiors: 'interiors.tmj', installation: 'Not installed; no collision, transitions or gameplay content assigned.' }, null, 2) + '\n');
console.log('PASS: native atlas256x256; overworld32x32 tiles/512x512px; interiors32x8 tiles/512x128px; Tiled maps and provenance.');
