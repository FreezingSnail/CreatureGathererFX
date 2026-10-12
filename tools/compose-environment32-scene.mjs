#!/usr/bin/env node
// Pixel composition from authored placements; no generated scene illustration.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import { decodePng, encodePng } from './creature-sprite-export.mjs';
import { habitatGround, habitatProp } from './habitat-terrain.mjs';
const base = 'docs/assets/world-environment-32-spike';
const sheets = ['atlas32.png', 'expansion/atlas32.png', 'habitats/atlas32.png'].map(file => decodePng(fs.readFileSync(`${base}/${file}`)));
const slugs = ['river', 'town', 'city', 'cave', 'cave-dark', 'huskfield', 'dewcap', 'shellstone', 'cinderflint'];
const collection = [];
for (const folder of slugs.map(slug => slug === 'river' ? '' : 'scenes/' + slug)) {
const root = folder ? `${base}/${folder}` : base;
const dark = folder === 'scenes/cave-dark';
const scene = JSON.parse(fs.readFileSync(`${dark ? base + '/scenes/cave' : root}/scene.json`));
if (dark) {
  scene.name = 'Crystal cave · dark'; scene.darkTerrain = true;
  scene.description = 'Inverted cave scenery with the original player and white outline unchanged.';
  fs.mkdirSync(root, { recursive: true });
  fs.writeFileSync(`${root}/scene.json`, JSON.stringify(scene, null, 2) + '\n');
}
collection.push({ ...scene, folder });
const [width, height] = scene.size;
const terrain = scene.terrain ? habitatGround(scene, decodePng(fs.readFileSync(`${base}/habitats/terrain/atlas32.png`))) : { width, height, pixels: new Uint8Array(width * height * 4).fill(255) };
function blit(target, source, sx, sy, w, h, dx, dy) {
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
    const si = ((sy + y) * source.width + sx + x) * 4, di = ((dy + y) * target.width + dx + x) * 4;
    if (!source.pixels[si + 3]) continue;
    target.pixels.set(source.pixels.subarray(si, si + 4), di);
  }
}
const occupied = new Set();
function enlarged(image, scale, file) {
  const output = { width: image.width * scale, height: image.height * scale, pixels: new Uint8Array(image.pixels.length * scale * scale) };
  for (let y = 0; y < output.height; y++) for (let x = 0; x < output.width; x++) {
    const i = (Math.floor(y / scale) * image.width + Math.floor(x / scale)) * 4;
    output.pixels.set(image.pixels.subarray(i, i + 4), (y * output.width + x) * 4);
  }
  fs.writeFileSync(`${root}/${file}`, encodePng(output));
}
for (const [id, x, y] of scene.placements) {
  assert.ok(id >= 1 && id <= 144);
  assert.ok(x >= 0 && y >= 0 && x + 32 <= width && y + 32 <= height);
  assert.equal(x % 16, 0); assert.equal(y % 16, 0);
  assert.ok(!occupied.has(`${x},${y}`), `Duplicate placement ${x},${y}`); occupied.add(`${x},${y}`);
  const sheet = sheets[id > 80 ? 2 : id > 64 ? 1 : 0], index = id - (id > 80 ? 81 : id > 64 ? 65 : 1), columns = sheet.width / 32;
  if(scene.terrain) blit(terrain, habitatProp(sheet,index),0,0,32,32,x,y);
  else blit(terrain, sheet, index % columns * 32, Math.floor(index / columns) * 32, 32, 32, x, y);
}
if (dark) for (let i = 0; i < terrain.pixels.length; i += 4)
  for (let c = 0; c < 3; c++) terrain.pixels[i + c] = 255 - terrain.pixels[i + c];
fs.writeFileSync(`${root}/scene-terrain.png`, encodePng(terrain));
for (const [index, view] of scene.views.entries()) {
  const player = decodePng(fs.readFileSync(`docs/assets/world-player/${view.facing}.png`));
  assert.equal(player.width, 16); assert.equal(player.height, 16);
  const full = { width, height, pixels: terrain.pixels.slice() };
  blit(full, player, 0, 0, 16, 16, ...view.player);
  if (index === 0) { fs.writeFileSync(`${root}/scene.png`, encodePng(full)); enlarged(full, 2, 'scene-preview2x.png'); }
  const crop = { width: 128, height: 64, pixels: new Uint8Array(128 * 64 * 4) };
  assert.ok(view.camera[0] >= 0 && view.camera[1] >= 0 && view.camera[0] + 128 <= width && view.camera[1] + 64 <= height);
  blit(crop, full, ...view.camera, 128, 64, 0, 0);
  fs.writeFileSync(`${root}/viewport-${index + 1}.png`, encodePng(crop));
  enlarged(crop, 4, `viewport-${index + 1}-4x.png`);
}
fs.writeFileSync(`${root}/scene-data.js`, 'window.environment32Scene = ' + JSON.stringify(scene) + ';\n');
console.log(`PASS: ${scene.name}: ${scene.placements.length} exact authored 32px placements; ${width}x${height} scene; ${scene.views.length} native128x64 crops with original16x16 masked player.`);
}
fs.writeFileSync(`${base}/scene-collection.js`, 'window.environment32Scenes = ' + JSON.stringify(collection) + ';\n');
const cards = collection.map((scene, index) => {
  const folder = 'assets/world-environment-32-spike/' + (scene.folder ? scene.folder + '/' : '');
  const flip = '';
  const residents = scene.creatures ? '<div class="habitat-residents">' + scene.creatures.map(c => `<figure><img src="${c.sprite}" width="96" height="96" alt="${c.name} front sprite"><figcaption>${c.name}<br><small>${c.types.join(' / ')}</small></figcaption></figure>`).join('') + '</div><p><strong>Scenery motifs:</strong> ' + scene.motifs + '</p><p>Creature references use 2× battle-sprite scale; the world player remains 16×16.</p>' : '';
  const crops = scene.views.map((view, i) => `<figure><figcaption>${view.name}</figcaption><p>1× · 128×64</p><div class="device-frame"><img class="native-crop${flip}" src="${folder}viewport-${i + 1}.png" width="128" height="64" alt="${scene.name}: ${view.name} at native size"></div><p>2× inspection</p><img class="large-crop${flip}" src="${folder}viewport-${i + 1}.png" width="256" height="128" alt="${scene.name}: ${view.name} enlarged"></figure>`).join('\n');
  return `<article class="example-card" id="${slugs[index]}"><h3>${scene.name}</h3><p>${scene.description || 'Compact cottage, river crossing and grove.'}</p>${residents}<img class="example-map${flip}" src="${folder}scene.png" width="384" height="256" alt="Full ${scene.name} arrangement"><div class="example-crops">${crops}</div><p><a href="?scene=${slugs[index]}#scene">Inspect and pan this scene</a></p></article>`;
}).join('\n');
const page = 'docs/world-environment-32-spike.html';
const html = fs.readFileSync(page, 'utf8');
assert.ok(html.includes('<!-- all-examples-start -->'), 'Missing scene gallery insertion point');
fs.writeFileSync(page, html.replace(/<!-- all-examples-start -->[\s\S]*?<!-- all-examples-end -->/,
  '<!-- all-examples-start -->\n' + cards + '\n<!-- all-examples-end -->'));
