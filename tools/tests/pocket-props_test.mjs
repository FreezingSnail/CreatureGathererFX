// Permanent source-art check for the first-beta pocket props (jp8.11.33).
// The native packer derives 1bpp + the 4-byte header from this opaque 2-colour PNG.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {decodePng} from '../creature-sprite-export.mjs';

const SOURCE = 'images/pocketProps_16x16.png';
const CELL = 16;
const image = decodePng(fs.readFileSync(SOURCE));
const frames = image.width / CELL;

function value(frame, x, y) {
  const i = ((y * image.width) + frame * CELL + x) * 4;
  return image.pixels[i];
}

function ink(frame, y0 = 0, y1 = CELL) {
  let total = 0;
  for (let y = y0; y < y1; y++) for (let x = 0; x < CELL; x++) if (value(frame, x, y) === 0) total++;
  return total;
}

function inkBounds(frame) {
  let minX = CELL, maxX = -1, minY = CELL, maxY = -1;
  for (let y = 0; y < CELL; y++) for (let x = 0; x < CELL; x++) {
    if (value(frame, x, y) !== 0) continue;
    if (x < minX) minX = x;
    if (x > maxX) maxX = x;
    if (y < minY) minY = y;
    if (y > maxY) maxY = y;
  }
  return {minX, maxX, minY, maxY};
}

// Frozen frame order: 0..3 plant stages, then the lure box. Frame 4 is optional: if the
// producer rejects the box the sheet ships 64x16 with frames 0..3 and no reordering.
test('pocket props source is the frozen 80x16 five-frame strip', () => {
  assert.deepEqual([image.width, image.height], [80, 16]);
  assert.equal(frames, 5);
});

test('every pixel is opaque and strictly black or white', () => {
  for (let i = 0; i < image.pixels.length; i += 4) {
    const [r, g, b, a] = image.pixels.subarray(i, i + 4);
    assert.equal(a, 255, `alpha at pixel ${i / 4}`);
    assert.equal(r, g, b, `gray pixel ${i / 4}`);
    assert.ok(r === 0 || r === 255, `non-binary pixel ${i / 4} = ${r}`);
  }
});

test('plot stages 0..3 are distinct and accumulate ink in growth order', () => {
  const counts = [0, 1, 2, 3].map(f => ink(f));
  for (let f = 1; f < 4; f++) {
    assert.ok(counts[f] > counts[f - 1], `frame ${f} ink ${counts[f]} must exceed frame ${f - 1} ink ${counts[f - 1]}`);
  }
  for (let a = 0; a < 4; a++) for (let b = a + 1; b < 4; b++) {
    let differing = 0;
    for (let y = 0; y < CELL; y++) for (let x = 0; x < CELL; x++) if (value(a, x, y) !== value(b, x, y)) differing++;
    assert.ok(differing >= 8, `frames ${a} and ${b} differ in only ${differing} pixels`);
  }
});

test('stage 0 is an empty plot and stage 3 carries harvest heads', () => {
  // The tilled bed starts at row 8; stage 0 has nothing above it.
  assert.equal(ink(0, 0, 8), 0, 'empty plot must not draw growth above the bed');
  // Ripe heads occupy the top of the mature frame; the growing frame has none there.
  assert.ok(ink(3, 0, 4) >= 12, 'mature crop needs headed growth in its top rows');
  assert.equal(ink(2, 0, 2), 0, 'growing crop must not already show ripe heads');
  const stage3 = inkBounds(3);
  assert.ok(stage3.minY <= 2, 'mature crop should reach its top rows');
});

test('lure box frame 4 is distinct from every plot stage', () => {
  for (let f = 0; f < 4; f++) {
    let differing = 0;
    for (let y = 0; y < CELL; y++) for (let x = 0; x < CELL; x++) if (value(f, x, y) !== value(4, x, y)) differing++;
    assert.ok(differing >= 32, `box differs from frame ${f} in only ${differing} pixels`);
  }
  // A lidded box is taller than the low plot bed and carries no stalk heads.
  assert.ok(inkBounds(4).minY <= 6, 'box lid should start above the plot bed row');
  assert.equal(ink(4, 0, 5), 0, 'box frame has no stalk growth');
  assert.ok(ink(4) >= 30, 'box reads as a filled container, not a sparse outline');
});
