#!/usr/bin/env node
// Arrange comparable 16px world-tile examples from the pre-existing redraw sheets.
// This exports review material only; it never edits canonical game assets.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { decodePng, encodePng } from './creature-sprite-export.mjs';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const TILE = 16, WIDTH = 384, HEIGHT = 256;
const playerRoot = path.join(root, 'docs/assets/world-player');
const outputRoot = path.join(root, 'docs/assets/world-arranged-comparison');

function blank(width, height, color = 255) {
  const pixels = new Uint8Array(width * height * 4);
  for (let i = 0; i < width * height; i++) {
    pixels[i * 4] = pixels[i * 4 + 1] = pixels[i * 4 + 2] = color;
    pixels[i * 4 + 3] = 255;
  }
  return { width, height, pixels };
}

function pasteTile(target, source, tileId, x, y) {
  const sourceX = ((tileId - 1) % 16) * TILE;
  const sourceY = Math.floor((tileId - 1) / 16) * TILE;
  for (let py = 0; py < TILE; py++) for (let px = 0; px < TILE; px++) {
    const from = ((sourceY + py) * source.width + sourceX + px) * 4;
    const to = ((y + py) * target.width + x + px) * 4;
    target.pixels.set(source.pixels.subarray(from, from + 4), to);
  }
}

function pasteMasked(target, source, x, y) {
  if (source.width !== 16 || source.height !== 16) throw Error('Player sprite must be native 16x16');
  for (let py = 0; py < 16; py++) for (let px = 0; px < 16; px++) {
    const from = (py * 16 + px) * 4;
    if (source.pixels[from + 3] === 0) continue;
    const to = ((y + py) * target.width + x + px) * 4;
    target.pixels.set(source.pixels.subarray(from, from + 4), to);
  }
}

function crop(image, x, y, width, height) {
  if (x < 0 || y < 0 || x + width > image.width || y + height > image.height) throw Error('Camera crop exceeds scene');
  const pixels = new Uint8Array(width * height * 4);
  for (let row = 0; row < height; row++) {
    const from = ((y + row) * image.width + x) * 4;
    pixels.set(image.pixels.subarray(from, from + width * 4), row * width * 4);
  }
  return { width, height, pixels };
}

function tileIdAt(sx, sy) { return (sy / TILE) * 16 + sx / TILE + 1; }

function makeScene(source, slug) {
  const terrain = blank(WIDTH, HEIGHT), placements = [];
  const putTile = (id, tx, ty) => {
    if (tx < 0 || ty < 0 || tx >= WIDTH / TILE || ty >= HEIGHT / TILE) throw Error(`Out of scene tile: ${slug} ${tx},${ty}`);
    pasteTile(terrain, source, id, tx * TILE, ty * TILE);
    placements.push([id, tx * TILE, ty * TILE]);
  };
  const putBlock = (sx, sy, columns, rows, tx, ty) => {
    if (sx % TILE || sy % TILE) throw Error('Source block must align to the 16px tile grid');
    for (let row = 0; row < rows; row++) for (let col = 0; col < columns; col++) {
      putTile(tileIdAt(sx + col * TILE, sy + row * TILE), tx + col, ty + row);
    }
  };

  if (slug === 'town') {
    // A townhouse face, garden patches, and a short brick lane meet in one camera view.
    putBlock(0, 192, 4, 4, 4, 0);
    putBlock(0, 0, 4, 4, 0, 2);
    putBlock(0, 0, 4, 4, 20, 2);
    putBlock(0, 112, 4, 1, 4, 4);
  } else if (slug === 'city') {
    // Repeat the intact townhouse face; the open lower half is a quiet street.
    for (const tx of [0, 4, 8, 12, 16, 20]) putBlock(0, 192, 4, 4, tx, 0);
  } else if (slug === 'cave') {
    // Isolated round boulder and pebble tiles flank a quiet white passage.
    for (const [id, tx, ty] of [
      [56, 2, 3], [38, 3, 3], [56, 2, 4], [38, 4, 4],
      [56, 8, 3], [38, 9, 3], [56, 8, 4], [38, 9, 4],
      [56, 14, 3], [38, 15, 3], [56, 14, 4], [38, 15, 4],
      [56, 20, 3], [38, 21, 3], [56, 20, 4], [38, 21, 4],
      [56, 2, 9], [38, 3, 9], [56, 20, 9], [38, 21, 9]
    ]) putTile(id, tx, ty);
  } else throw Error(`Unknown scene: ${slug}`);

  // Player start and three view positions are pixel coordinates in the 384x256 scene.
  const starts = {
    town: { x: 104, y: 64, facing: 'down', camera: [48, 16] },
    city: { x: 184, y: 64, facing: 'up', camera: [128, 16] },
    cave: { x: 184, y: 96, facing: 'up', camera: [128, 64] }
  };
  const start = starts[slug];
  const points = slug === 'town'
    ? [[start.x, start.y, start.facing, start.camera], [72, 96, 'right', [0, 64]], [296, 96, 'left', [256, 64]]]
    : slug === 'city'
      ? [[start.x, start.y, start.facing, start.camera], [72, 64, 'right', [0, 16]], [296, 64, 'left', [256, 16]]]
      : [[start.x, start.y, start.facing, start.camera], [72, 96, 'down', [0, 64]], [296, 96, 'up', [256, 64]]];
  return { terrain, placements, start, points };
}

function assertBinaryOpaque(image, label) {
  for (let i = 0; i < image.pixels.length; i += 4) {
    const [r, g, b, a] = image.pixels.subarray(i, i + 4);
    if (r !== g || g !== b || (r !== 0 && r !== 255) || a !== 255) throw Error(`${label} is not opaque binary B&W`);
  }
}

function runSet(version) {
  const setName = `redraw-v${version}`;
  const atlasRelative = `docs/assets/world-tiles/tiles-bw-redraw-v${version}-native.png`;
  const source = decodePng(fs.readFileSync(path.join(root, atlasRelative)));
  if (source.width !== 256 || source.height !== 768) throw Error(`${atlasRelative} must be 256x768`);
  const output = path.join(outputRoot, setName);
  const manifest = { setName, atlas: atlasRelative, tileSize: TILE, scenes: [] };
  for (const [slug, name] of [['town', 'Town / garden lane'], ['city', 'City / dense storefronts'], ['cave', 'Cave / rock passage']]) {
    const { terrain, placements, start, points } = makeScene(source, slug);
    const sceneDir = path.join(output, slug);
    fs.mkdirSync(sceneDir, { recursive: true });
    assertBinaryOpaque(terrain, `${setName}/${slug}/terrain`);
    fs.writeFileSync(path.join(sceneDir, 'terrain.png'), encodePng(terrain));
    const startPlayer = decodePng(fs.readFileSync(path.join(playerRoot, `${start.facing}.png`)));
    const composed = blank(WIDTH, HEIGHT);
    composed.pixels.set(terrain.pixels);
    pasteMasked(composed, startPlayer, start.x, start.y);
    assertBinaryOpaque(composed, `${setName}/${slug}/scene`);
    fs.writeFileSync(path.join(sceneDir, 'scene.png'), encodePng(composed));
    const views = [];
    points.forEach(([playerX, playerY, facing, camera], index) => {
      const [cameraX, cameraY] = camera;
      if (cameraX < 0 || cameraY < 0 || cameraX + 128 > WIDTH || cameraY + 64 > HEIGHT) throw Error('Invalid 128x64 camera');
      if (playerX < cameraX || playerY < cameraY || playerX + 16 > cameraX + 128 || playerY + 16 > cameraY + 64) throw Error('Player outside camera crop');
      const viewImage = crop(terrain, cameraX, cameraY, 128, 64);
      pasteMasked(viewImage, decodePng(fs.readFileSync(path.join(playerRoot, `${facing}.png`))), playerX - cameraX, playerY - cameraY);
      assertBinaryOpaque(viewImage, `${setName}/${slug}/viewport-${index + 1}`);
      fs.writeFileSync(path.join(sceneDir, `viewport-${index + 1}.png`), encodePng(viewImage));
      views.push({ name: `viewport-${index + 1}`, player: [playerX, playerY], facing, camera });
    });
    manifest.scenes.push({ slug, name, size: [WIDTH, HEIGHT], placements, views });
  }
  fs.writeFileSync(path.join(output, 'manifest.json'), JSON.stringify(manifest, null, 2) + '\n');
  console.log(`${setName}: ${manifest.scenes.length} scenes, ${manifest.scenes.reduce((n, s) => n + s.views.length, 0)} viewport crops`);
}

runSet(4);
runSet(5);
