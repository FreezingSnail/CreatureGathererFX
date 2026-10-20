#!/usr/bin/env node
// Review-only world tileset comparison. Does not touch game assets or maps.
import fs from 'node:fs';
import path from 'node:path';
import { decodePng, encodePng } from './creature-sprite-export.mjs';

const root = 'docs/assets/overworld-tileset-round2';
const sources = [
  { id: 'layered', label: 'Layered storybook', file: 'sources/layered.png', prompt: 'sources/prompt-layered.json', note: 'Soft layered grayscale, clustered natural texture, and small illustrated accents.' },
  { id: 'chunky', label: 'Chunky silhouettes', file: 'sources/chunky.png', prompt: 'sources/prompt-chunky.json', note: 'Large, high-contrast forms with blockier edges and clearer ground shapes.' },
  { id: 'inked', label: 'Hand-inked field guide', file: 'sources/inked.png', prompt: 'sources/prompt-inked.json', note: 'Defined outlines and grouped marks with a more restrained, graphic texture.' },
];
const treatments = [
  { id: 'clean', label: 'Clean threshold', method: 'single threshold', amplitude: 0 },
  { id: 'light', label: 'Light texture', method: '2x2 Bayer', amplitude: 72 },
  { id: 'heavy', label: 'Heavy texture', method: '4x4 Bayer', amplitude: 104 },
];
const tileNames = [
  'open grass', 'sparse grass', 'flowering grass', 'gravel', 'cave floor', 'cultivated soil', 'rough stone', 'paved ground',
  'path horizontal', 'path vertical', 'path corner northwest', 'path corner northeast', 'path corner southwest', 'path corner southeast', 'path junction', 'path crossing',
  'tree canopy left', 'tree canopy right', 'tree canopy alternate', 'shrub', 'fallen log', 'flower cluster', 'stump', 'ground detail',
  'cave wall edge left', 'cave wall top', 'cave wall edge right', 'cave side wall', 'boulder', 'crystal cluster', 'stalagmite', 'cave entrance',
  'cottage roof left', 'cottage roof right', 'cottage wall window', 'cottage wall door', 'cottage wall variant', 'fence', 'garden rows', 'garden bed',
  'shop front left', 'shop front center-left', 'shop front center-right', 'shop front right', 'doorway', 'display window', 'curb', 'streetlamp',
  'open water', 'bank horizontal', 'bank vertical', 'bank corner', 'bank edge variant', 'bridge horizontal', 'bridge vertical', 'stepping stones',
  'well', 'signpost', 'crate', 'bench', 'vegetable patch', 'flowerpot', 'boulder variant', 'geode',
];
const areas = [
  {
    id: 'cave', label: 'Cave',
    description: 'A small cave-room study built from the shared floor, wall, entrance and mineral tiles.',
    data: [
      24,25,25,25,25,25,25,26,
      27, 4, 4, 4, 4, 4, 4,27,
      27, 4,28, 4, 4,29, 4,27,
      27, 4, 4, 4,30, 4, 4,31,
    ],
  },
  {
    id: 'forest', label: 'Forest',
    description: 'Grass variants, single-cell tree and shrub props, flowers and connected path pieces.',
    data: [
      16,17,16,17, 1,16,17, 1,
      18,19, 1, 9, 1,21, 1,18,
       1, 1, 1, 9, 1, 1, 1, 1,
      20, 1, 8,14, 8, 1,19, 1,
    ],
  },
  {
    id: 'rural-town', label: 'Rural town',
    description: 'Two cottages use the same roof and wall pieces, with reusable fences, paths and garden props.',
    data: [
      32,33, 1, 1,32,33, 1, 1,
      34,35, 1, 9,34,35, 1, 1,
      37,37, 8,14, 8,37,37,37,
      56, 1, 8, 9, 8,38,39, 1,
    ],
  },
  {
    id: 'city', label: 'City',
    description: 'A denser paved street with repeated shop frontage, curb, lamp and public seating.',
    data: [
      40,41,42,43, 7,40,41,42,
      44,45,46, 9,46,44,45,47,
      46,46, 8,14, 8,46,46,46,
       7,59, 8, 9, 8,47, 7, 7,
    ],
  },
];
const thresholdCenter = 128;
const bayer2 = [0, 2, 3, 1];
const bayer4 = [0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5];
const playerSheet = decodePng(fs.readFileSync('images/Playerchibi_16x16.png'));
if (playerSheet.width !== 96 || playerSheet.height !== 32) throw Error('Unexpected Playerchibi_16x16.png dimensions');

function blank(width, height, value = 255) {
  const pixels = new Uint8Array(width * height * 4);
  for (let i = 0; i < pixels.length; i += 4) pixels.set([value, value, value, 255], i);
  return { width, height, pixels };
}

function sampleCell(source, cell) {
  const inset = 3; // strip the generated black cell guides before native-size reduction
  const x0 = cell.x + inset, x1 = cell.x + cell.width - inset;
  const y0 = cell.y + inset, y1 = cell.y + cell.height - inset;
  const samples = new Float32Array(16 * 16);
  for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
    const ax = x0 + x * (x1 - x0) / 16, bx = x0 + (x + 1) * (x1 - x0) / 16;
    const ay = y0 + y * (y1 - y0) / 16, by = y0 + (y + 1) * (y1 - y0) / 16;
    let total = 0, area = 0;
    for (let sy = Math.floor(ay); sy < Math.ceil(by); sy++) {
      const wy = Math.min(by, sy + 1) - Math.max(ay, sy);
      for (let sx = Math.floor(ax); sx < Math.ceil(bx); sx++) {
        const wx = Math.min(bx, sx + 1) - Math.max(ax, sx), weight = wx * wy;
        const i = (sy * source.width + sx) * 4;
        const lum = 0.2126 * source.pixels[i] + 0.7152 * source.pixels[i + 1] + 0.0722 * source.pixels[i + 2];
        total += lum * weight; area += weight;
      }
    }
    samples[y * 16 + x] = total / area;
  }
  return samples;
}

function styleTile(samples, treatment) {
  const pixels = new Uint8Array(16 * 16 * 4), size = treatment.id === 'heavy' ? 4 : treatment.id === 'light' ? 2 : 0;
  for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
    let offset = 0;
    if (size === 2) offset = ((bayer2[(y % 2) * 2 + x % 2] + 0.5) / 4 - 0.5) * treatment.amplitude;
    if (size === 4) offset = ((bayer4[(y % 4) * 4 + x % 4] + 0.5) / 16 - 0.5) * treatment.amplitude;
    const v = samples[y * 16 + x] < thresholdCenter + offset ? 0 : 255;
    pixels.set([v, v, v, 255], (y * 16 + x) * 4);
  }
  return { width: 16, height: 16, pixels };
}

function copyTile(target, tile, dx, dy) {
  for (let y = 0; y < 16; y++) {
    const from = y * 16 * 4, to = ((dy + y) * target.width + dx) * 4;
    target.pixels.set(tile.pixels.subarray(from, from + 16 * 4), to);
  }
}

function buildAtlas(tiles) {
  const atlas = blank(128, 128);
  tiles.forEach((tile, id) => copyTile(atlas, tile, (id % 8) * 16, Math.floor(id / 8) * 16));
  return atlas;
}

function compositePlayer(image) {
  // Down-facing player (first frame of sheet row 2) at its runtime viewport position.
  const frameX = 0, frameY = 16, placeX = 56, placeY = 24;
  for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
    const si = ((frameY + y) * playerSheet.width + frameX + x) * 4;
    if (!playerSheet.pixels[si + 3]) continue;
    const di = ((placeY + y) * image.width + placeX + x) * 4;
    const v = (playerSheet.pixels[si] + playerSheet.pixels[si + 1] + playerSheet.pixels[si + 2]) / 3 < 128 ? 0 : 255;
    image.pixels.set([v, v, v, 255], di);
  }
}

function buildMap(tiles, area) {
  if (area.data.length !== 32) throw Error(`${area.id}: expected exactly 8×4 tile IDs`);
  const image = blank(128, 64);
  area.data.forEach((tileId, index) => {
    if (!tiles[tileId]) throw Error(`${area.id}: tile ${tileId} is outside the atlas`);
    copyTile(image, tiles[tileId], (index % 8) * 16, Math.floor(index / 8) * 16);
  });
  compositePlayer(image);
  return image;
}

function wholeWorld() {
  const data = new Array(16 * 8);
  const place = (area, ox, oy) => area.data.forEach((id, index) => { data[(oy + Math.floor(index / 8)) * 16 + ox + index % 8] = id; });
  const cave = areas.find(area => area.id === 'cave');
  const forest = areas.find(area => area.id === 'forest');
  const rural = areas.find(area => area.id === 'rural-town');
  const city = areas.find(area => area.id === 'city');
  place(cave, 0, 0); place(forest, 8, 0); place(rural, 0, 4); place(city, 8, 4);
  // Join each 8×4 location at the edge paths so this becomes one traversable
  // 16×8 study rather than four panels placed next to each other.
  data[2 * 16 + 7] = 31; // cave opening on the east edge
  data[3 * 16 + 7] = 8;  // cave-to-forest route
  data[3 * 16 + 8] = 8;
  data[3 * 16 + 3] = 9;  // cave-to-rural passage
  data[4 * 16 + 3] = 9;
  data[3 * 16 + 12] = 9; // forest-to-city passage
  data[4 * 16 + 12] = 9;
  data[7 * 16 + 7] = 8;  // rural-to-city road
  data[7 * 16 + 8] = 8;
  return { id: 'whole-world', width: 16, height: 8, data };
}

function contactSheet(columns) {
  const image = blank(800, 552);
  columns.forEach((column, col) => column.forEach((scene, row) => {
    for (let y = 0; y < 64; y++) for (let x = 0; x < 128; x++) {
      const si = (y * 128 + x) * 4;
      for (let dy = 0; dy < 2; dy++) for (let dx = 0; dx < 2; dx++) {
        const di = ((8 + row * 136 + y * 2 + dy) * image.width + 8 + col * 264 + x * 2 + dx) * 4;
        image.pixels.set(scene.pixels.subarray(si, si + 4), di);
      }
    }
  }));
  return image;
}

function nearest4(image) {
  const scaled = blank(image.width * 4, image.height * 4);
  for (let y = 0; y < scaled.height; y++) for (let x = 0; x < scaled.width; x++) {
    const si = (Math.floor(y / 4) * image.width + Math.floor(x / 4)) * 4;
    scaled.pixels.set(image.pixels.subarray(si, si + 4), (y * scaled.width + x) * 4);
  }
  return scaled;
}

function tiledMap(area) {
  return {
    version: '1.10', type: 'map', orientation: 'orthogonal', renderorder: 'right-down',
    width: area.width ?? 8, height: area.height ?? 4, tilewidth: 16, tileheight: 16, infinite: false,
    nextlayerid: 2, nextobjectid: 1,
    tilesets: [{ firstgid: 1, source: '../tileset.tsx' }],
    layers: [{ id: 1, name: 'World tiles', type: 'tilelayer', width: area.width ?? 8, height: area.height ?? 4, x: 0, y: 0, opacity: 1, visible: true, data: area.data.map(id => id + 1) }],
  };
}

function tiledTileset() {
  const tileNodes = tileNames.map((name, id) => `  <tile id="${id}"><properties><property name="name" value="${name}"/></properties></tile>`).join('\n');
  return `<?xml version="1.0" encoding="UTF-8"?>\n<tileset version="1.10" name="Overworld World Kit" tilewidth="16" tileheight="16" tilecount="64" columns="8">\n  <image source="atlas-128.png" width="128" height="128"/>\n${tileNodes}\n</tileset>\n`;
}

const manifest = {
  reviewOnly: true,
  title: 'Overworld reusable world-kit round 2',
  sourcePolicy: 'Freshly generated tile sheets. No existing spike artwork used as a visual reference.',
  viewport: [128, 64], mapTile: [16, 16], viewportTiles: [8, 4],
  player: { source: 'images/Playerchibi_16x16.png', frame: 'DOWN', size: [16, 16], position: [56, 24], center: [64, 32], compositedAfterMap: true },
  treatments, tileGrid: [8, 8], tileNames: tileNames.map((name, id) => ({ id, name })),
  tileFamilies: [
    { range: [0, 7], family: 'ground and base terrain' }, { range: [8, 15], family: 'path segments and junctions' },
    { range: [16, 23], family: 'trees, shrubs and forest props' }, { range: [24, 31], family: 'cave wall, floor and minerals' },
    { range: [32, 39], family: 'cottage pieces, fences and gardens' }, { range: [40, 47], family: 'shops, street edges and lighting' },
    { range: [48, 55], family: 'water, banks and crossings' }, { range: [56, 63], family: 'shared landmarks and props' },
  ],
  areas: areas.map(({ id, label, description, data }) => ({ id, label, description, width: 8, height: 4, tileIds: data })),
  wholeWorld: { width: 16, height: 8, tilePixels: [256, 128], viewportCount: [2, 2], description: 'Four locations joined as a traversable 16×8 tile study. Viewport guides divide it into four 128×64 screens.', tileIds: wholeWorld().data },
  artDirections: sources.map(({ id, label, note, file, prompt }) => ({ id, label, note, source: file, prompt })),
  export: 'Each 8×8 source cell is cropped independently, area-weighted to native 16×16, monochrome converted per tile, and reused by tile ID in every example map.',
};

fs.mkdirSync(root, { recursive: true });
for (const direction of sources) {
  const source = decodePng(fs.readFileSync(path.join(root, direction.file)));
  if (source.width !== 1254 || source.height !== 1254) throw Error(`${direction.file}: expected 1254×1254 atlas`);
  const cellSamples = [];
  for (let row = 0; row < 8; row++) for (let col = 0; col < 8; col++) {
    const x0 = Math.round(col * source.width / 8), x1 = Math.round((col + 1) * source.width / 8);
    const y0 = Math.round(row * source.height / 8), y1 = Math.round((row + 1) * source.height / 8);
    cellSamples.push(sampleCell(source, { x: x0, y: y0, width: x1 - x0, height: y1 - y0 }));
  }
  const styleViews = [];
  const treatmentScenes = [];
  for (const treatment of treatments) {
    const output = path.join(root, direction.id, treatment.id);
    fs.mkdirSync(path.join(output, 'maps'), { recursive: true });
    const tiles = cellSamples.map(samples => styleTile(samples, treatment));
    const atlas = buildAtlas(tiles);
    fs.writeFileSync(path.join(output, 'atlas-128.png'), encodePng(atlas));
    fs.writeFileSync(path.join(output, 'atlas-preview-4x.png'), encodePng(nearest4(atlas)));
    fs.writeFileSync(path.join(output, 'tileset.tsx'), tiledTileset());
    const sceneImages = [];
    for (const area of areas) {
      const image = buildMap(tiles, area);
      fs.writeFileSync(path.join(output, `${area.id}.png`), encodePng(image));
      fs.writeFileSync(path.join(output, 'maps', `${area.id}.tmj`), JSON.stringify(tiledMap(area), null, 2) + '\n');
      sceneImages.push(image);
    }
    const world = wholeWorld();
    const worldImage = blank(world.width * 16, world.height * 16);
    world.data.forEach((tileId, index) => copyTile(worldImage, tiles[tileId], (index % world.width) * 16, Math.floor(index / world.width) * 16));
    fs.writeFileSync(path.join(output, 'whole-world.png'), encodePng(worldImage));
    const worldScaled = nearest4(worldImage);
    // One-pixel split at the 4× enlarged 128×64 boundaries shows four camera windows.
    const gridColor = [128,128,128,255];
    for (let y = 0; y < worldScaled.height; y++) worldScaled.pixels.set(gridColor, (y * worldScaled.width + 511) * 4);
    for (let x = 0; x < worldScaled.width; x++) worldScaled.pixels.set(gridColor, (255 * worldScaled.width + x) * 4);
    fs.writeFileSync(path.join(output, 'whole-world-viewport-grid-4x.png'), encodePng(worldScaled));
    fs.writeFileSync(path.join(output, 'maps', 'whole-world.tmj'), JSON.stringify(tiledMap(world), null, 2) + '\n');
    treatmentScenes.push(sceneImages);
    styleViews.push({ treatment: treatment.id, atlas: `${direction.id}/${treatment.id}/atlas-128.png`, preview: `${direction.id}/${treatment.id}/atlas-preview-4x.png`, world: `${direction.id}/${treatment.id}/whole-world.png`, worldPreview: `${direction.id}/${treatment.id}/whole-world-viewport-grid-4x.png`, worldTiledMap: `${direction.id}/${treatment.id}/maps/whole-world.tmj`, maps: areas.map(area => ({ area: area.id, image: `${direction.id}/${treatment.id}/${area.id}.png`, tiledMap: `${direction.id}/${treatment.id}/maps/${area.id}.tmj` })) });
  }
  const views = contactSheet(treatmentScenes);
  fs.writeFileSync(path.join(root, direction.id, 'contact-sheet-2x.png'), encodePng(views));
  const manifestDirection = manifest.artDirections.find(item => item.id === direction.id);
  manifestDirection.outputs = styleViews;
  manifestDirection.contactSheet = `${direction.id}/contact-sheet-2x.png`;
}
fs.writeFileSync(path.join(root, 'manifest.json'), JSON.stringify(manifest, null, 2) + '\n');
console.log(`Generated ${sources.length} art directions, ${treatments.length} treatments, ${tileNames.length} reusable tile IDs, ${areas.length} viewport maps and one joined world map.`);
