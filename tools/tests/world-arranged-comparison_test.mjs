import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { decodePng } from '../creature-sprite-export.mjs';
const root = 'docs/assets/world-arranged-comparison';
const read = file => decodePng(fs.readFileSync(file));
for (const slug of ['classic16', 'redraw-v4', 'redraw-v5']) {
  test(slug + ': exact existing16px tiles and unchanged16px player camera views', () => {
    const set = JSON.parse(fs.readFileSync(`${root}/${slug}/manifest.json`));
    const atlas = read(set.atlas), columns = atlas.width / 16;
    assert.equal(set.tileSize, 16); assert.equal(set.scenes.length, 3);
    for (const scene of set.scenes) {
      const terrain = read(`${root}/${slug}/${scene.slug}/terrain.png`);
      assert.deepEqual([terrain.width, terrain.height], [384, 256]);
      const expected = new Uint8Array(terrain.pixels.length).fill(255);
      for (const [id, dx, dy] of scene.placements) {
        assert.ok(id > 0 && id <= atlas.width * atlas.height / 256);
        assert.ok(dx >= 0 && dy >= 0 && dx + 16 <= terrain.width && dy + 16 <= terrain.height);
        for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
          const si = ((Math.floor((id - 1) / columns) * 16 + y) * atlas.width + (id - 1) % columns * 16 + x) * 4;
          expected.set(atlas.pixels.subarray(si, si + 4), ((dy + y) * terrain.width + dx + x) * 4);
        }
      }
      assert.deepEqual(terrain.pixels, expected);
      assert.equal(scene.views.length, 3);
      for (const [index, view] of scene.views.entries()) {
        const crop = read(`${root}/${slug}/${scene.slug}/viewport-${index + 1}.png`);
        const player = read(`docs/assets/world-player/${view.facing}.png`);
        assert.deepEqual([crop.width, crop.height], [128, 64]);
        assert.deepEqual([player.width, player.height], [16, 16]);
        const [px, py] = view.player, [cx, cy] = view.camera;
        assert.ok(px >= cx && py >= cy && px + 16 <= cx + 128 && py + 16 <= cy + 64);
        for (let y = 0; y < 64; y++) for (let x = 0; x < 128; x++) {
          const wx = cx + x, wy = cy + y, ti = (wy * terrain.width + wx) * 4;
          const pi = ((wy - py) * 16 + wx - px) * 4;
          const inside = wx >= px && wx < px + 16 && wy >= py && wy < py + 16;
          const source = inside && player.pixels[pi + 3] ? player : terrain;
          const offset = source === player ? pi : ti;
          assert.deepEqual(crop.pixels.subarray((y * 128 + x) * 4, (y * 128 + x) * 4 + 4), source.pixels.subarray(offset, offset + 4));
        }
      }
    }
  });
}

test('one sheet contains all20 exact native comparison crops across five sets', () => {
  const manifest = JSON.parse(fs.readFileSync(`${root}/comparison-manifest.json`));
  const sheet = read(`${root}/comparison-native.png`);
  assert.equal(manifest.sets.length, 5); assert.equal(manifest.cells.length, 20);
  assert.deepEqual([sheet.width, sheet.height], manifest.sheet);
  for (const cell of manifest.cells) {
    const source = read('docs/' + cell.source);
    for (let y = 0; y < 64; y++) for (let x = 0; x < 128; x++) {
      const si = (y * 128 + x) * 4, ti = ((cell.y + y) * sheet.width + cell.x + x) * 4;
      assert.equal(sheet.pixels[ti], source.pixels[si]);
      assert.equal(sheet.pixels[ti + 1], sheet.pixels[ti]); assert.equal(sheet.pixels[ti + 2], sheet.pixels[ti]);
      assert.equal(sheet.pixels[ti + 3], 255);
    }
  }
});

test('all five dark cave sets invert scenery while preserving every masked player pixel', () => {
  const manifest = JSON.parse(fs.readFileSync(`${root}/comparison-manifest.json`));
  for (const set of manifest.sets) {
    const cave = set.scenes.find(scene => scene.slug === 'cave');
    cave.views.forEach((view, index) => {
      const light = read(`docs/${set.assetRoot}/cave/viewport-${index + 1}.png`);
      const dark = read(`docs/${set.darkAssetRoot}/cave-dark/viewport-${index + 1}.png`);
      const player = read(`docs/assets/world-player/${view.facing}.png`);
      const [px, py] = view.player, [cx, cy] = view.camera;
      for (let y = 0; y < 64; y++) for (let x = 0; x < 128; x++) {
        const i = (y * 128 + x) * 4, wx = cx + x, wy = cy + y;
        const pi = ((wy - py) * 16 + wx - px) * 4;
        const opaquePlayer = wx >= px && wx < px + 16 && wy >= py && wy < py + 16 && player.pixels[pi + 3];
        const expected = opaquePlayer ? player.pixels[pi] : 255 - light.pixels[i];
        assert.equal(dark.pixels[i], expected);
        assert.equal(dark.pixels[i + 1], expected); assert.equal(dark.pixels[i + 2], expected); assert.equal(dark.pixels[i + 3], 255);
      }
    });
  }
});
