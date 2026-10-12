import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { decodePng } from '../creature-sprite-export.mjs';
import { habitatGround, habitatProp } from '../habitat-terrain.mjs';
const base = 'docs/assets/world-environment-32-spike';
for (const folder of ['', 'scenes/town', 'scenes/city', 'scenes/cave', 'scenes/cave-dark', ...['huskfield','dewcap','shellstone','cinderflint'].map(s => 'scenes/' + s)]) {
const root = folder ? base + '/' + folder : base;
const read = file => decodePng(fs.readFileSync(file));
const scene = JSON.parse(fs.readFileSync(`${root}/scene.json`));

test(scene.name + ': native binary pixels', () => {
  const atlas = read(`${base}/expansion/atlas32.png`);
  assert.deepEqual([atlas.width, atlas.height], [128, 128]);
  for (const file of ['scene-terrain.png', 'scene.png', 'viewport-1.png', 'viewport-2.png', 'viewport-3.png']) {
    const image = read(`${root}/${file}`);
    for (let i = 0; i < image.pixels.length; i += 4) {
      assert.ok(image.pixels[i] === 0 || image.pixels[i] === 255);
      assert.equal(image.pixels[i + 1], image.pixels[i]); assert.equal(image.pixels[i + 2], image.pixels[i]);
      assert.equal(image.pixels[i + 3], 255);
    }
  }
});

test(scene.name + ': exact authored tile composition', () => {
  const terrain = read(`${root}/scene-terrain.png`);
  assert.deepEqual([terrain.width, terrain.height], scene.size);
  const original = read(`${base}/atlas32.png`), expansion = read(`${base}/expansion/atlas32.png`), habitats = read(`${base}/habitats/atlas32.png`);
  const expected = scene.terrain ? habitatGround(scene,read(`${base}/habitats/terrain/atlas32.png`)).pixels : new Uint8Array(terrain.pixels.length).fill(255);
  for (const [id, dx, dy] of scene.placements) {
    const image = id > 80 ? habitats : id <= 64 ? original : expansion, index = id - (id > 80 ? 81 : id <= 64 ? 1 : 65), columns = image.width / 32;
    const prop = scene.terrain ? habitatProp(image,index) : null;
    for (let y = 0; y < 32; y++) for (let x = 0; x < 32; x++) {
      const si = ((Math.floor(index / columns) * 32 + y) * image.width + index % columns * 32 + x) * 4;
      if(prop) {const pi=(y*32+x)*4;if(prop.pixels[pi+3])expected.set(prop.pixels.subarray(pi,pi+4),((dy+y)*terrain.width+dx+x)*4);}
      else expected.set(image.pixels.subarray(si, si + 4), ((dy + y) * terrain.width + dx + x) * 4);
    }
  }
  if (scene.darkTerrain) for (let i = 0; i < expected.length; i += 4) for (let c = 0; c < 3; c++) expected[i + c] = 255 - expected[i + c];
  assert.deepEqual(terrain.pixels, expected);
});

test(scene.name + ': exact 128x64 crops and 16px masked player', () => {
  const terrain = read(`${root}/scene-terrain.png`);
  scene.views.forEach((view, index) => {
    const player = read(`docs/assets/world-player/${view.facing}.png`);
    const crop = read(`${root}/viewport-${index + 1}.png`);
    assert.deepEqual([player.width, player.height], [16, 16]);
    assert.deepEqual([crop.width, crop.height], [128, 64]);
    const [px, py] = view.player, [cx, cy] = view.camera;
    assert.ok(px >= cx && py >= cy && px + 16 <= cx + 128 && py + 16 <= cy + 64);
    for (let y = 0; y < 64; y++) for (let x = 0; x < 128; x++) {
      const wx = cx + x, wy = cy + y, terrainIndex = (wy * terrain.width + wx) * 4;
      const inside = wx >= px && wx < px + 16 && wy >= py && wy < py + 16;
      const playerIndex = ((wy - py) * 16 + wx - px) * 4;
      const source = inside && player.pixels[playerIndex + 3] ? player : terrain;
      const offset = source === player ? playerIndex : terrainIndex;
      assert.deepEqual(crop.pixels.subarray((y * 128 + x) * 4, (y * 128 + x) * 4 + 4), source.pixels.subarray(offset, offset + 4));
    }
  });
});

}
