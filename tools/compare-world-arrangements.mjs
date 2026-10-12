#!/usr/bin/env node
// Contact sheets from exact existing scene pixels; no art resampling at native size.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import { decodePng, encodePng } from './creature-sprite-export.mjs';
const root = 'docs/assets/world-arranged-comparison';
fs.mkdirSync(root, { recursive: true });
const slugs = ['town', 'city', 'cave'];
const sets = [
  { slug: 'simple32', name: '32PX SIMPLE', assetRoot: 'assets/world-environment-32-spike/simple/scenes', scenes: slugs.map(slug => ({ slug, ...JSON.parse(fs.readFileSync(`docs/assets/world-environment-32-spike/simple/scenes/${slug}/scene.json`)) })) },
  { slug: 'dither32', name: '32PX DITHER', assetRoot: 'assets/world-environment-32-spike/scenes', scenes: slugs.map(slug => ({ slug, ...JSON.parse(fs.readFileSync(`docs/assets/world-environment-32-spike/scenes/${slug}/scene.json`)) })) },
  ...['classic16', 'redraw-v4', 'redraw-v5'].map(slug => ({ slug, assetRoot: `assets/world-arranged-comparison/${slug}`, ...JSON.parse(fs.readFileSync(`${root}/${slug}/manifest.json`)) })),
];
sets.forEach(set => { set.name ||= set.setName; });
const font = {
  A:'010101111101101',B:'110101110101110',C:'011100100100011',D:'110101101101110',E:'111100110100111',F:'111100110100100',G:'011100101101011',H:'101101111101101',I:'111010010010111',J:'001001001101010',K:'101101110101101',L:'100100100100111',M:'101111111101101',N:'101111111111101',O:'010101101101010',P:'110101110100100',Q:'010101101111011',R:'110101110101101',S:'011100010001110',T:'111010010010010',U:'101101101101111',V:'101101101101010',W:'101101111111101',X:'101101010101101',Y:'101101010010010',Z:'111001010100111',
  '0':'111101101101111','1':'010110010010111','2':'110001010100111','3':'110001010001110','4':'101101111001001','5':'111100110001110','6':'011100111101111','7':'111001010010010','8':'111101111101111','9':'111101111001110','-':'000000111000000',
};
const blank = (width, height) => ({ width, height, pixels: new Uint8Array(width * height * 4).fill(255) });
function text(image, label, x, y, scale = 1) {
  for (const character of label.toUpperCase()) {
    const bits = font[character] || '0'.repeat(15);
    for (let i = 0; i < 15; i++) if (bits[i] === '1')
      for (let dy = 0; dy < scale; dy++) for (let dx = 0; dx < scale; dx++) image.pixels.set([0, 0, 0, 255], ((y + Math.floor(i / 3) * scale + dy) * image.width + x + i % 3 * scale + dx) * 4);
    x += 4 * scale;
  }
}
function copy(target, source, x, y, scale = 1, inverted = false) {
  for (let sy = 0; sy < source.height; sy++) for (let sx = 0; sx < source.width; sx++) {
    const si = (sy * source.width + sx) * 4;
    assert.ok(source.pixels[si] === 0 || source.pixels[si] === 255);
    const v = inverted ? 255 - source.pixels[si] : source.pixels[si];
    for (let dy = 0; dy < scale; dy++) for (let dx = 0; dx < scale; dx++)
      target.pixels.set([v, v, v, 255], ((y + sy * scale + dy) * target.width + x + sx * scale + dx) * 4);
  }
}
const read = file => decodePng(fs.readFileSync('docs/' + file));
for (const set of sets) {
  if (set.slug === 'dither32' || set.slug === 'simple32') { set.darkAssetRoot = set.assetRoot; continue; }
  set.darkAssetRoot = `assets/world-arranged-comparison/dark/${set.slug}`;
  const destination = 'docs/' + set.darkAssetRoot + '/cave-dark';
  fs.mkdirSync(destination, { recursive: true });
  const cave = set.scenes.find(scene => scene.slug === 'cave');
  const terrain = read(`${set.assetRoot}/cave/terrain.png`);
  const dark = blank(terrain.width, terrain.height); copy(dark, terrain, 0, 0, 1, true);
  fs.writeFileSync(`${destination}/terrain.png`, encodePng(dark));
  for (const [index, view] of cave.views.entries()) {
    const full = { ...dark, pixels: dark.pixels.slice() };
    const player = read(`assets/world-player/${view.facing}.png`);
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const si = (y * 16 + x) * 4;
      if (player.pixels[si + 3]) full.pixels.set(player.pixels.subarray(si, si + 4), ((view.player[1] + y) * full.width + view.player[0] + x) * 4);
    }
    if (!index) fs.writeFileSync(`${destination}/scene.png`, encodePng(full));
    const crop = blank(128, 64);
    for (let y = 0; y < 64; y++) for (let x = 0; x < 128; x++) {
      const si = ((view.camera[1] + y) * full.width + view.camera[0] + x) * 4;
      crop.pixels.set(full.pixels.subarray(si, si + 4), (y * 128 + x) * 4);
    }
    fs.writeFileSync(`${destination}/viewport-${index + 1}.png`, encodePng(crop));
  }
}
const sheet = blank(72 + sets.length * 136, 328);
const cells = [];
sets.forEach((set, column) => text(sheet, set.slug.toUpperCase(), 72 + column * 136, 8));
for (const [row, slug] of [...slugs, 'cave-dark'].entries()) {
  text(sheet, slug.toUpperCase(), 4, 40 + row * 72);
  for (const [column, set] of sets.entries()) {
    const source = `${slug === 'cave-dark' ? set.darkAssetRoot : set.assetRoot}/${slug}/viewport-1.png`;
    const image = read(source); assert.deepEqual([image.width, image.height], [128, 64]);
    const x = 72 + column * 136, y = 32 + row * 72;
    copy(sheet, image, x, y);
    cells.push({ set: set.slug, scene: slug, source, x, y, size: [128, 64], sceneryInverted: slug === 'cave-dark' });
  }
}
fs.writeFileSync(`${root}/comparison-native.png`, encodePng(sheet));
const enlarged = blank(sheet.width * 2, sheet.height * 2); copy(enlarged, sheet, 0, 0, 2);
fs.writeFileSync(`${root}/comparison-2x.png`, encodePng(enlarged));
for (const set of sets) {
  const arranged = blank(1176, 280);
  slugs.forEach((slug, index) => { text(arranged, slug.toUpperCase(), 8 + index * 392, 4); copy(arranged, read(`${set.assetRoot}/${slug}/scene.png`), 8 + index * 392, 20); });
  fs.writeFileSync(`${root}/${set.slug}-scenes.png`, encodePng(arranged));
}
fs.writeFileSync(`${root}/comparison-manifest.json`, JSON.stringify({ reviewOnly: true, sets, sheet: [sheet.width, sheet.height], cells }, null, 2) + '\n');
const rows = [...slugs, 'cave-dark'].map(slug => `<tr><th scope="row">${slug}</th>${sets.map(set => {
  const folder = `${slug === 'cave-dark' ? set.darkAssetRoot : set.assetRoot}/${slug}`;
  const inverted = '';
  return `<td><p>1× · 128×64</p><div class="device-frame"><img class="native-crop${inverted}" src="${folder}/viewport-1.png" width="128" height="64" alt="${set.name} ${slug} native viewport"></div><p>2× inspection</p><img class="large-crop${inverted}" src="${folder}/viewport-1.png" width="256" height="128" alt="${set.name} ${slug} enlarged"><p><a href="${folder}/scene.png">Full arrangement</a></p></td>`;
}).join('')}</tr>`).join('\n');
const section = `<h2>All arranged asset sets</h2><p>Luna-arranged examples from the existing 16px black-and-white sets alongside the new 32px set. Every crop is 128×64 with the actual 16px player. These compare each set's interpretation of town, city and cave, rather than identical tile layouts.</p><p><a href="assets/world-arranged-comparison/comparison-native.png">Download comparison sheet · native</a> · <a href="assets/world-arranged-comparison/comparison-2x.png">Download comparison sheet · 2×</a></p><div class="scroll"><table class="set-comparison"><thead><tr><th>Scene</th>${sets.map(set => `<th scope="col">${set.name}</th>`).join('')}</tr></thead><tbody>${rows}</tbody></table></div><h3>Full arranged sheets</h3>${sets.map(set => `<details open><summary>${set.name} · town, city and cave</summary><div class="scroll"><img src="assets/world-arranged-comparison/${set.slug}-scenes.png" width="1176" height="280" alt="All three ${set.name} arrangements"></div></details>`).join('\n')}`;
const page = 'docs/world-environment-32-spike.html', html = fs.readFileSync(page, 'utf8');
assert.ok(html.includes('<!-- asset-comparison-start -->'));
fs.writeFileSync(page, html.replace(/<!-- asset-comparison-start -->[\s\S]*?<!-- asset-comparison-end -->/, '<!-- asset-comparison-start -->\n' + section + '\n<!-- asset-comparison-end -->'));
console.log(`PASS: ${sets.length} arranged sets; ${cells.length} exact128x64 comparison cells; ${sets.length} full scene sheets.`);
