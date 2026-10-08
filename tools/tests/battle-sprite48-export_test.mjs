import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { decodePng, validateSprite } from '../creature-sprite-export.mjs';
import { mirrorHorizontal } from '../battle-sprite48-export.mjs';

test('reviewed front orientation is baked into exports and backs are never mirrored', () => {
  const manifest = JSON.parse(fs.readFileSync('docs/assets/battle-sprites48/manifest.json'));
  const fronts = JSON.parse(fs.readFileSync('data/json/battle_sprite_orientation.json')).fronts;
  for (const entry of manifest.sprites) {
    if (entry.view === 'back') { assert.equal(entry.mirrored, false); continue; }
    assert.equal(entry.sourceFacing, fronts[entry.name]);
    assert.equal(entry.mirrored, entry.sourceFacing === 'left');
    assert(['right', 'front'].includes(entry.facing));
  }
  // Reflection must preserve all RGBA pixel bytes including the transparent mask.
  const sprite = decodePng(fs.readFileSync(`docs/assets/battle-sprites48/${manifest.sprites[0].file}`));
  assert.deepEqual(mirrorHorizontal(mirrorHorizontal(sprite)).pixels, sprite.pixels);
  for (let y = 0; y < 48; y++) {
    const mirror = mirrorHorizontal(sprite);
    assert.deepEqual(mirror.pixels.subarray(y * 48 * 4, y * 48 * 4 + 4),
      sprite.pixels.subarray((y * 48 + 47) * 4, (y * 48 + 48) * 4));
  }
});

test('all64 species have validated48px front/back exports matching packed source slots', () => {
  const manifest = JSON.parse(fs.readFileSync('docs/assets/battle-sprites48/manifest.json'));
  const sheet = decodePng(fs.readFileSync('images/Battlecreatures_48x48.png'));
  assert.equal(manifest.sprites.length, 128);
  assert.equal(sheet.width, 96); assert.equal(sheet.height, 3072);
  const seen = new Set();
  for (const entry of manifest.sprites) {
    const sprite = decodePng(fs.readFileSync(`docs/assets/battle-sprites48/${entry.file}`));
    validateSprite(sprite, 48);
    const view = entry.view === 'front' ? 0 : 1;
    seen.add(`${entry.id}:${view}`);
    for (let y = 0; y < 48; y++)
      assert.deepEqual(sheet.pixels.subarray(((entry.id * 48 + y) * 96 + view * 48) * 4,
          ((entry.id * 48 + y) * 96 + view * 48 + 48) * 4), sprite.pixels.subarray(y * 48 * 4, (y + 1) * 48 * 4));
  }
  assert.equal(seen.size, 128);
});

test('compact options have four128x16 opaque binary frames and a full cell selection', () => {
  const im = decodePng(fs.readFileSync('images/Battleoptions48_128x16.png'));
  assert.equal(im.width, 128); assert.equal(im.height, 64);
  for (let i = 0; i < im.pixels.length; i += 4) {
    assert([0, 255].includes(im.pixels[i]));
    assert.equal(im.pixels[i], im.pixels[i + 1]); assert.equal(im.pixels[i], im.pixels[i + 2]);
    assert.equal(im.pixels[i + 3], 255);
  }
  for (let frame = 0; frame < 4; frame++) {
    const y = frame * 16 + Math.floor(frame / 2) * 8;
    const x = frame % 2 * 64;
    assert.equal(im.pixels[(y * 128 + x) * 4], 0);
    assert.equal(im.pixels[(y * 128 + (x + 64) % 128) * 4], 255);
  }
});

test('all128 packed battle frames match the native viewer sprites byte for byte', () => {
  const header = fs.readFileSync('src/fxdata.h', 'utf8');
  const address = Number(header.match(/battleSprites48\s*=\s*(0x[0-9A-Fa-f]+)/)[1]);
  const packed = fs.readFileSync('dist/fxdata.bin');
  assert.equal(packed.readUInt16BE(address), 48);
  assert.equal(packed.readUInt16BE(address + 2), 48);
  const manifest = JSON.parse(fs.readFileSync('docs/assets/battle-sprites48/manifest.json'));
  for (const entry of manifest.sprites) {
    const sprite = decodePng(fs.readFileSync(`docs/assets/battle-sprites48/${entry.file}`));
    const frame = entry.id * 2 + (entry.view === 'back' ? 1 : 0);
    for (let page = 0; page < 6; page++) for (let x = 0; x < 48; x++) {
      let pixel = 0, mask = 0;
      for (let bit = 0; bit < 8; bit++) {
        const i = ((page * 8 + bit) * 48 + x) * 4;
        if (sprite.pixels[i + 3]) mask |= 1 << bit;
        if (sprite.pixels[i + 3] && sprite.pixels[i]) pixel |= 1 << bit;
      }
      const i = address + 4 + frame * 576 + (page * 48 + x) * 2;
      assert.equal(packed[i], pixel); assert.equal(packed[i + 1], mask);
    }
  }
});
