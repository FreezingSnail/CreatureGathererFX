#!/usr/bin/env node
// Convert fresh grayscale directions into exact 128x64 monochrome comparisons.
import fs from 'node:fs';
import { decodePng, encodePng } from './creature-sprite-export.mjs';

const root = 'docs/assets/overworld-identity-spike';
const playerSheet = decodePng(fs.readFileSync('images/Playerchibi_16x16.png'));
if (playerSheet.width !== 96 || playerSheet.height !== 32) {
  throw Error(`Unexpected player sheet dimensions ${playerSheet.width}x${playerSheet.height}`);
}

const artDirections = [
  { id: 'layered', label: 'Layered storybook', source: 'source.png', prompt: 'prompt.json' },
  { id: 'chunky', label: 'Chunky silhouettes', source: 'sources/chunky.png', prompt: 'prompt-chunky.json' },
  { id: 'inked', label: 'Hand-inked field guide', source: 'sources/inked.png', prompt: 'prompt-inked.json' },
];
const scenes = [
  { id: 'cave', label: 'Cave', x: 5, y: 0 },
  { id: 'forest', label: 'Forest', x: 905, y: 0 },
  { id: 'rural-town', label: 'Rural town', x: 5, y: 455 },
  { id: 'city', label: 'City', x: 905, y: 455 },
];
const treatments = [
  { id: 'clean', label: 'Clean threshold', method: 'single threshold', amplitude: 0 },
  { id: 'light', label: 'Light texture', method: '2x2 Bayer', amplitude: 72 },
  { id: 'heavy', label: 'Heavy texture', method: '4x4 Bayer', amplitude: 104 },
];
const sourceRect = { width: 864, height: 432 };
const thresholdCenter = 128;
const bayer2 = [0, 2, 3, 1];
const bayer4 = [
  0, 8, 2, 10, 12, 4, 14, 6,
  3, 11, 1, 9, 15, 7, 13, 5,
];
const makeImage = (width, height, value = 255) => {
  const pixels = new Uint8Array(width * height * 4);
  for (let i = 0; i < pixels.length; i += 4) pixels.set([value, value, value, 255], i);
  return { width, height, pixels };
};

function downsample(scene, source) {
  const pixels = new Float32Array(128 * 64);
  for (let y = 0; y < 64; y++) for (let x = 0; x < 128; x++) {
    const x0 = scene.x + x * sourceRect.width / 128;
    const x1 = scene.x + (x + 1) * sourceRect.width / 128;
    const y0 = scene.y + y * sourceRect.height / 64;
    const y1 = scene.y + (y + 1) * sourceRect.height / 64;
    let total = 0, area = 0;
    for (let sy = Math.floor(y0); sy < Math.ceil(y1); sy++) {
      const wy = Math.min(y1, sy + 1) - Math.max(y0, sy);
      for (let sx = Math.floor(x0); sx < Math.ceil(x1); sx++) {
        const wx = Math.min(x1, sx + 1) - Math.max(x0, sx);
        const weight = wx * wy;
        const index = (sy * source.width + sx) * 4;
        const luminance = source.pixels[index] * 0.2126 + source.pixels[index + 1] * 0.7152 + source.pixels[index + 2] * 0.0722;
        total += luminance * weight;
        area += weight;
      }
    }
    pixels[y * 128 + x] = total / area;
  }
  return pixels;
}

function makeStyleFrame(samples, style) {
  const image = makeImage(128, 64);
  let blackPixels = 0;
  const size = style.id === 'heavy' ? 4 : style.id === 'light' ? 2 : 0;
  for (let y = 0; y < 64; y++) for (let x = 0; x < 128; x++) {
    let offset = 0;
    if (size === 2) offset = ((bayer2[(y % 2) * 2 + x % 2] + 0.5) / 4 - 0.5) * style.amplitude;
    if (size === 4) offset = ((bayer4[(y % 4) * 4 + x % 4] + 0.5) / 16 - 0.5) * style.amplitude;
    const black = samples[y * 128 + x] < thresholdCenter + offset;
    if (black) blackPixels++;
    const value = black ? 0 : 255;
    image.pixels.set([value, value, value, 255], (y * 128 + x) * 4);
  }
  return { image, blackPixels };
}

function compositePlayer(image) {
  // Direction order is UP, RIGHT, DOWN, LEFT; down is frame 6 (first frame of row 2).
  const frameX = 0, frameY = 16, placeX = 56, placeY = 24;
  for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
    const si = ((frameY + y) * playerSheet.width + frameX + x) * 4;
    if (playerSheet.pixels[si + 3] === 0) continue;
    const di = ((placeY + y) * image.width + placeX + x) * 4;
    const value = (playerSheet.pixels[si] + playerSheet.pixels[si + 1] + playerSheet.pixels[si + 2]) / 3 < 128 ? 0 : 255;
    image.pixels.set([value, value, value, 255], di);
  }
}

const results = [];
const contactSheets = [];
for (const direction of artDirections) {
  const source = decodePng(fs.readFileSync(`${root}/${direction.source}`));
  if (source.width !== 1774 || source.height !== 887) {
    throw Error(`${direction.source}: unexpected concept dimensions ${source.width}x${source.height}`);
  }
  const contact = makeImage(3 * 256 + 4 * 8, 4 * 128 + 5 * 8);
  for (const [row, scene] of scenes.entries()) {
    const samples = downsample(scene, source);
    for (const [column, treatment] of treatments.entries()) {
      const { image, blackPixels } = makeStyleFrame(samples, treatment);
      compositePlayer(image);
      const file = `${direction.id}/${treatment.id}/${scene.id}.png`;
      fs.writeFileSync(`${root}/${file}`, encodePng(image));
      const left = 8 + column * 264;
      const top = 8 + row * 136;
      for (let y = 0; y < 64; y++) for (let x = 0; x < 128; x++) {
        const si = (y * 128 + x) * 4;
        for (let dy = 0; dy < 2; dy++) for (let dx = 0; dx < 2; dx++) {
          const di = ((top + y * 2 + dy) * contact.width + left + x * 2 + dx) * 4;
          contact.pixels.set(image.pixels.subarray(si, si + 4), di);
        }
      }
      results.push({ direction: direction.id, scene: scene.id, treatment: treatment.id, file, pixels: [128, 64], tileGrid: [8, 4], player: { frame: 'down', size: [16, 16], position: [56, 24], center: [64, 32] }, threshold: thresholdCenter, method: treatment.method, ditherAmplitude: treatment.amplitude, terrainBlackPixels: blackPixels });
    }
  }
  const contactFile = `${direction.id}/contact-sheet-2x.png`;
  fs.writeFileSync(`${root}/${contactFile}`, encodePng(contact));
  contactSheets.push({ direction: direction.id, file: contactFile, size: [contact.width, contact.height], source: direction.source, prompt: direction.prompt });
}
fs.writeFileSync(`${root}/manifest.json`, JSON.stringify({ reviewOnly: true, artDirections, scenes: scenes.map(scene => ({ ...scene, crop: [scene.x, scene.y, sourceRect.width, sourceRect.height] })), viewport: [128, 64], mapTile: [16, 16], viewportTiles: [8, 4], player: { source: 'images/Playerchibi_16x16.png', frame: 'DOWN', size: [16, 16], position: [56, 24], center: [64, 32], compositedAfterDither: true }, treatments, contactSheets, export: 'Area-weighted grayscale reduction to native pixels, then monochrome threshold / ordered Bayer conversion; nearest-neighbor 2x contact sheet.', views: results }, null, 2) + '\n');
console.log(`PASS: ${results.length} exact128x64 views across ${artDirections.length} art directions and ${treatments.length} monochrome treatments.`);
