#!/usr/bin/env node
// Arrange original 16px world-environment atlas tiles into classic overworld examples.
// Preview-only: source atlas and player assets remain untouched.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { decodePng, encodePng } from './creature-sprite-export.mjs';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const output = path.join(repo, 'docs/assets/world-arranged-comparison/classic16');
const atlasPath = 'docs/assets/world-environment-spike/atlas-native.png';
const atlas = decodePng(fs.readFileSync(path.join(repo, atlasPath)));
if (atlas.width !== 256 || atlas.height !== 512) throw Error('Expected the 256x512 source atlas');

const sceneSize = [384, 256], tileSize = 16;
const fill = (id) => Array.from({ length: 16 }, (_, row) =>
  Array.from({ length: 24 }, (_, col) => [id, col * tileSize, row * tileSize])).flat();
const overlay = (id, x, y) => [id, x, y];
function assembly(firstId, x, y) {
  return Array.from({ length: 4 }, (_, row) =>
    Array.from({ length: 4 }, (_, col) => [firstId + row * 16 + col, x + col * tileSize, y + row * tileSize])).flat();
}
const scenes = [
  {
    slug: 'town', name: 'Village lane', description: 'Compact cottages, gardens, and a quiet lane.',
    placements: [
      ...fill(97),
      // Clean white walking lanes sit over the patterned village ground.
      ...Array.from({ length: 16 }, (_, row) => overlay(129, 160, row * tileSize)),
      ...assembly(257, 64, 32), ...assembly(261, 272, 32),
      ...assembly(265, 32, 160),
      ...assembly(413, 272, 160),
      ...assembly(385, 208, 96), ...assembly(389, 112, 32),
      ...assembly(401, 0, 192),
    ],
    views: [
      { name: 'House and lane', player: [144, 80], facing: 'up', camera: [64, 32] },
      { name: 'Garden walk', player: [224, 192], facing: 'right', camera: [128, 144] },
      { name: 'Cottage square', player: [72, 120], facing: 'left', camera: [16, 96] },
    ],
  },
  {
    slug: 'city', name: 'Market street', description: 'A denser street of shops, a stone landmark, and small courtyards.',
    placements: [
      ...fill(97),
      ...Array.from({ length: 16 }, (_, row) => overlay(129, 160, row * tileSize)),
      ...Array.from({ length: 24 }, (_, col) => overlay(129, col * tileSize, 112)),
      ...assembly(265, 16, 16), ...assembly(265, 96, 16), ...assembly(265, 272, 16),
      ...assembly(269, 192, 16),
      ...assembly(257, 16, 160), ...assembly(265, 144, 160), ...assembly(261, 288, 160),
      ...assembly(413, 96, 96), ...assembly(409, 224, 96),
      ...assembly(397, 208, 192),
      // The foreground market street stays visible beneath the shop fronts.
      ...Array.from({ length: 24 }, (_, col) => overlay(129, col * tileSize, 64)),
    ],
    views: [
      { name: 'Shopfront and street', player: [128, 48], facing: 'up', camera: [32, 16] },
      { name: 'Market square', player: [240, 128], facing: 'left', camera: [128, 80] },
      { name: 'South shops', player: [88, 208], facing: 'right', camera: [32, 160] },
    ],
  },
  {
    slug: 'cave', name: 'Crystal cavern', description: 'A rocky passage winds past a cave entrance and crystal formations.',
    placements: [
      ...fill(65),
      // Dark stone passage edges and a wider chamber at the center.
      ...Array.from({ length: 24 }, (_, col) => overlay(3, col * tileSize, 0)),
      ...Array.from({ length: 24 }, (_, col) => overlay(3, col * tileSize, 240)),
      ...Array.from({ length: 16 }, (_, row) => overlay(3, 0, row * tileSize)),
      ...Array.from({ length: 16 }, (_, row) => overlay(3, 368, row * tileSize)),
      ...assembly(449, 32, 32),
      ...assembly(393, 96, 64), ...assembly(397, 288, 32),
      ...assembly(333, 32, 160), ...assembly(393, 240, 160),
      // Mountain boulders and the cave entrance supply the authored interior landmarks.
      ...assembly(393, 96, 112), ...assembly(397, 304, 128),
    ],
    views: [
      { name: 'Cave entrance and rocks', player: [80, 80], facing: 'down', camera: [32, 32] },
      { name: 'Rock passage', player: [72, 192], facing: 'right', camera: [0, 160] },
      { name: 'Crystal chamber', player: [288, 192], facing: 'up', camera: [256, 144] },
    ],
  },
];

function blit(target, source, sx, sy, width, height, dx, dy) {
  for (let y = 0; y < height; y++) for (let x = 0; x < width; x++) {
    const si = ((sy + y) * source.width + sx + x) * 4;
    const di = ((dy + y) * target.width + dx + x) * 4;
    const alpha = source.pixels[si + 3];
    if (!alpha) continue;
    if (alpha === 255) target.pixels.set(source.pixels.subarray(si, si + 4), di);
    else {
      const inverse = 255 - alpha;
      for (let channel = 0; channel < 3; channel++) target.pixels[di + channel] = Math.round((source.pixels[si + channel] * alpha + target.pixels[di + channel] * inverse) / 255);
      target.pixels[di + 3] = Math.min(255, alpha + Math.round(target.pixels[di + 3] * inverse / 255));
    }
  }
}
function blank(width, height) {
  const pixels = new Uint8Array(width * height * 4);
  for (let i = 0; i < pixels.length; i += 4) pixels.set([255, 255, 255, 255], i);
  return { width, height, pixels };
}
function tile(sourceId) {
  if (!Number.isInteger(sourceId) || sourceId < 1 || sourceId > 512) throw Error(`Bad source tile id ${sourceId}`);
  const index = sourceId - 1;
  return { x: (index % 16) * tileSize, y: Math.floor(index / 16) * tileSize };
}
function drawPlacements(target, placements) {
  for (const [id, x, y] of placements) {
    if (x % tileSize || y % tileSize || x < 0 || y < 0 || x + tileSize > target.width || y + tileSize > target.height) throw Error(`Out-of-bounds or unaligned tile ${id}@${x},${y}`);
    const source = tile(id);
    blit(target, atlas, source.x, source.y, tileSize, tileSize, x, y);
  }
}
function playerAt(target, view) {
  const sprite = decodePng(fs.readFileSync(path.join(repo, `docs/assets/world-player/${view.facing}.png`)));
  if (sprite.width !== 16 || sprite.height !== 16) throw Error(`Expected native masked player ${view.facing}`);
  blit(target, sprite, 0, 0, 16, 16, view.player[0], view.player[1]);
}

fs.mkdirSync(output, { recursive: true });
for (const scene of scenes) {
  const folder = path.join(output, scene.slug);
  fs.mkdirSync(folder, { recursive: true });
  const terrain = blank(...sceneSize);
  drawPlacements(terrain, scene.placements);
  fs.writeFileSync(path.join(folder, 'terrain.png'), encodePng(terrain));
  const main = { ...terrain, pixels: terrain.pixels.slice() };
  playerAt(main, scene.views[0]);
  fs.writeFileSync(path.join(folder, 'scene.png'), encodePng(main));
  for (let index = 0; index < scene.views.length; index++) {
    const view = scene.views[index];
    const [px, py] = view.player, [cx, cy] = view.camera;
    if (px < 0 || py < 0 || px + 16 > sceneSize[0] || py + 16 > sceneSize[1]) throw Error(`${scene.slug}: player outside scene`);
    if (cx < 0 || cy < 0 || cx + 128 > sceneSize[0] || cy + 64 > sceneSize[1]) throw Error(`${scene.slug}: camera outside scene`);
    if (px < cx || py < cy || px + 16 > cx + 128 || py + 16 > cy + 64) throw Error(`${scene.slug}: player outside viewport ${index + 1}`);
    const frame = { ...terrain, pixels: terrain.pixels.slice() };
    playerAt(frame, view);
    const crop = blank(128, 64);
    blit(crop, frame, cx, cy, 128, 64, 0, 0);
    fs.writeFileSync(path.join(folder, `viewport-${index + 1}.png`), encodePng(crop));
  }
}

const manifest = {
  setName: 'Classic 16px world arrangements',
  atlas: atlasPath,
  tileSize,
  scenes: scenes.map(({ slug, name, placements, views }) => ({
    slug, name, size: sceneSize, placements, views,
  })),
};
fs.writeFileSync(path.join(output, 'manifest.json'), JSON.stringify(manifest, null, 2) + '\n');
console.log(`PASS: ${scenes.length} scenes from original16px tiles; each is384x256 with terrain, player composite, and three128x64 views.`);
