import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { decodePng, encodePng, bounds, exportView, validateSprite, atlasCells } from '../creature-sprite-export.mjs';

function image(width, height) { return { width, height, pixels: new Uint8Array(width * height * 4) }; }
function rect(im, x, y, w, h, shade = 255, alpha = 255) {
  for (let yy = y; yy < y + h; yy++) for (let xx = x; xx < x + w; xx++) im.pixels.set([shade, shade, shade, alpha], (yy * im.width + xx) * 4);
}
test('PNG preserves visible black cutouts and transparent background; rejects corruption', () => {
  const source = image(12, 12);
  rect(source, 2, 2, 8, 8); rect(source, 4, 4, 4, 4, 0);
  const bytes = encodePng(source), copy = decodePng(bytes);
  assert.deepEqual(copy.pixels, source.pixels);
  bytes[45] ^= 1;
  assert.throws(() => decodePng(bytes), /CRC/);
});
test('crop uses alpha, retaining black body pixels; ignores labels outside its cell', () => {
  const source = image(32, 16);
  rect(source, 2, 3, 9, 10, 0); rect(source, 5, 5, 2, 2);
  rect(source, 20, 1, 4, 4);
  assert.deepEqual(bounds(source, { x: 0, y: 0, width: 16, height: 16 }), { x: 2, y: 3, width: 9, height: 10 });
  assert.throws(() => bounds(source, { x: 28, y: 8, width: 4, height: 8 }), /Empty/);
});
test('both exports contain binary color/mask with transparent margins and black details', () => {
  const source = image(16, 16);
  rect(source, 2, 2, 12, 12, 220, 210); rect(source, 5, 5, 6, 6, 20, 255);
  for (const size of [32, 48]) {
    const result = exportView(source, { x: 2, y: 2, width: 12, height: 12 }, size, 12);
    const stats = validateSprite(result, size);
    assert(stats.white < stats.opaque);
    const center = ((size / 2) * size + size / 2) * 4;
    assert.equal(result.pixels[center], 0); assert.equal(result.pixels[center + 3], 255);
  }
});
test('common pair scale preserves smaller back-view proportions', () => {
  const source = image(16, 16); rect(source, 2, 2, 8, 8);
  const front = exportView(source, { x: 2, y: 2, width: 8, height: 8 }, 32, 8);
  const back = exportView(source, { x: 2, y: 2, width: 4, height: 4 }, 32, 8);
  const cell = { x: 0, y: 0, width: 32, height: 32 };
  assert.equal(bounds(front, cell).width, 28); assert.equal(bounds(back, cell).width, 14);
  assert.equal(bounds(back, cell).y + bounds(back, cell).height, 30);
});
test('validation rejects gray, partial alpha, clipped and empty exports', () => {
  const source = image(32, 32); rect(source, 2, 2, 8, 8);
  source.pixels[(3 * 32 + 3) * 4] = 123;
  assert.throws(() => validateSprite(source, 32), /Nonbinary/);
  rect(source, 2, 2, 8, 8, 255, 127);
  assert.throws(() => validateSprite(source, 32), /Nonbinary/);
  rect(source, 2, 2, 8, 8); rect(source, 0, 0, 1, 1);
  assert.throws(() => validateSprite(source, 32), /edge/);
  assert.throws(() => validateSprite(image(32, 32), 32), /Empty/);
});
test('nonuniform atlas gutters isolate all16 views and reject overlapping rows', () => {
  const source = image(101, 101);
  for (let row = 0; row < 4; row++) for (let col = 0; col < 4; col++) {
    rect(source, col * 25 + 3, row * 25 + 3, 20, 20);
  }
  const cells = atlasCells(source);
  assert.equal(cells.length, 16);
  for (const cell of cells) assert.equal(bounds(source, cell).width, 20);
  rect(source, 0, 0, 101, 101);
  assert.throws(() => atlasCells(source), /gutter/);
});
test('all128 delivered PNGs match their named size, binary masks and manifest coverage', () => {
  const root = new URL('../../docs/assets/creature-expansion/native/', import.meta.url);
  const manifest = JSON.parse(fs.readFileSync(new URL('manifest.json', root)));
  assert.equal(manifest.sprites.length, 128);
  const keys = new Set();
  for (const entry of manifest.sprites) {
    const key = `${entry.conceptId}/${entry.view}/${entry.size}`;
    assert(!keys.has(key)); keys.add(key);
    const sprite = decodePng(fs.readFileSync(new URL(entry.file, root)));
    const stats = validateSprite(sprite, entry.size);
    assert.equal(stats.opaque, entry.opaque); assert.equal(stats.white, entry.white);
    const box = bounds(sprite, { x: 0, y: 0, width: entry.size, height: entry.size });
    const margin = entry.size === 32 ? 2 : 3;
    assert(box.x >= margin && box.y >= margin);
    assert(box.x + box.width <= entry.size - margin && box.y + box.height <= entry.size - margin);
  }
  for (let id = 32; id < 64; id++) for (const view of ['front', 'back']) for (const size of [32, 48]) {
    assert(keys.has(`${id}/${view}/${size}`));
  }
  for (const size of [32, 48]) assert.equal(fs.readdirSync(new URL(`${size}x${size}/`, root)).filter(p => p.endsWith('.png')).length, 64);
});
test('paired sheets preserve every individual view in front/back and concept order', () => {
  const root = new URL('../../docs/assets/creature-expansion/native/', import.meta.url);
  const manifest = JSON.parse(fs.readFileSync(new URL('manifest.json', root)));
  for (const size of [32, 48]) {
    const sheet = decodePng(fs.readFileSync(new URL(`expansion_${size}x${size}.png`, root)));
    assert.equal(sheet.width, size * 2); assert.equal(sheet.height, size * 32);
    for (const entry of manifest.sprites.filter(p => p.size === size)) {
      const sprite = decodePng(fs.readFileSync(new URL(entry.file, root)));
      const ox = entry.view === 'front' ? 0 : size, oy = (entry.conceptId - 32) * size;
      for (let y = 0; y < size; y++) assert.deepEqual(
        sheet.pixels.subarray(((oy + y) * sheet.width + ox) * 4, ((oy + y) * sheet.width + ox + size) * 4),
        sprite.pixels.subarray(y * size * 4, (y + 1) * size * 4)
      );
    }
  }
});
