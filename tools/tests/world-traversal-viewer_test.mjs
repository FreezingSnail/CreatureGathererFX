import { test } from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
import { decodePng } from '../creature-sprite-export.mjs';

const duration = 16 * 1000 / 52;
function harness() {
  const scenes = JSON.parse(fs.readFileSync('docs/assets/world-environment-spike/traversal-scenes.json')).scenes;
  const elements = new Map(), frames = new Map(); let now = 0, serial = 0, draws = [];
  const element = id => {
    if (!elements.has(id)) elements.set(id, { listeners: {}, value: '0', textContent: '', focus() {}, addEventListener(type, fn) { this.listeners[type] = fn; } });
    return elements.get(id);
  };
  const ctx = { fillRect() { draws = []; }, drawImage(...args) { draws.push(args); } };
  element('#walk-preview').getContext = () => ctx;
  const document = { querySelector: element, listeners: {}, addEventListener(type, fn) { this.listeners[type] = fn; } };
  const window = { WORLD_TRAVERSAL_SCENES: scenes, listeners: {}, addEventListener(type, fn) { this.listeners[type] = fn; } };
  const context = { document, window, Image: class { complete = true; naturalWidth = 256; }, performance: { now: () => now }, requestAnimationFrame(fn) { frames.set(++serial, fn); return serial; }, cancelAnimationFrame(id) { frames.delete(id); } };
  vm.runInNewContext(fs.readFileSync('docs/assets/world-traversal-viewer.js', 'utf8'), context);
  return {
    scenes, frames, element,
    advance(ms) { now += ms; const pending = [...frames.values()]; frames.clear(); pending.forEach(fn => fn(now)); },
    click(dir) { element('#walk-' + dir).listeners.click(); },
    key(key) { element('#traversal').listeners.keydown({ key, target: element('#traversal'), preventDefault() {} }); },
    release(key) { window.listeners.keyup({ key }); },
    blur() { window.listeners.blur(); },
    select(index) { element('#walk-scene').value = String(index); element('#walk-scene').listeners.change(); },
    status() { return element('#walk-status').textContent; },
    terrain() { return draws.filter(args => args.length === 9); },
    player() { return draws.at(-1); }
  };
}

test('camera advances in native pixels while player stays anchored', () => {
  const h = harness(); h.click('down'); const initial = h.terrain();
  h.advance(duration / 2); const midway = h.terrain();
  // Same source tile moves up eight pixels at half of a16-pixel downward step.
  const tile = initial.find(a => a[6] === 24);
  assert.ok(midway.some(a => a[3] === tile[3] && a[5] === tile[5] && a[6] === 16));
  assert.deepEqual(h.player().slice(5), [56, 24, 16, 16]);
  h.advance(duration / 2); assert.equal(h.frames.size, 0);
});

test('blocked doorway changes facing and A detects door after tile completion', () => {
  const h = harness();
  for (const dir of ['down', 'left', 'left']) { h.click(dir); h.advance(duration + 1); }
  h.click('up'); assert.match(h.status(), /^Blocked/); assert.equal(h.frames.size, 0);
  h.click('enter'); assert.equal(h.status(), 'Cottage entrance reached');
});

test('held keys chain steps, release finishes current step, repeat does not duplicate frames', () => {
  const h = harness(); h.key('ArrowDown'); h.key('ArrowDown'); assert.equal(h.frames.size, 1);
  h.advance(duration + 1); assert.equal(h.frames.size, 1);
  h.release('ArrowDown'); h.advance(duration + 1); assert.equal(h.frames.size, 0);
});

test('scene switch cancels partial step and held state', () => {
  const h = harness(); h.key('ArrowDown'); h.advance(duration / 2); h.select(1);
  assert.equal(h.frames.size, 0); assert.equal(h.status(), 'One-tile footbridge');
  h.advance(duration + 1); assert.equal(h.frames.size, 0);
  h.click('right'); h.advance(duration + 1); h.click('right'); h.advance(duration + 1);
  h.click('up'); assert.match(h.status(), /^Blocked/);
});

test('blur stops held motion and A waits for step completion', () => {
  const h = harness(); h.key('ArrowDown'); const status = h.status(); h.key('a');
  assert.equal(h.status(), status); h.blur(); h.advance(duration + 1); assert.equal(h.frames.size, 0);
});

test('NPCs occupy a cell, scroll with terrain, and respond to A', () => {
  const h = harness(); h.click('down'); h.advance(duration + 1);
  h.click('right'); assert.match(h.status(), /^Blocked/); assert.equal(h.frames.size, 0);
  h.click('enter'); assert.match(h.status(), /^Shopkeeper · /);
  h.click('left'); const initial = h.terrain().filter(a => a[0].src.endsWith('sprites16.png'));
  h.advance(duration / 2);
  const moved = h.terrain().filter(a => a[0].src.endsWith('sprites16.png'));
  assert.equal(moved[0][5] - initial[0][5], 8);
});

for (const [root, columns, count] of [['docs/assets/world-npcs', 4, 8], ['docs/assets/world-player', 2, 4], ['docs/assets/world-player/walk', 4, 8]])
test(root + ' binary16x16 cells, outline, alpha and sheet parity', () => {
  const manifest = JSON.parse(fs.readFileSync(root + '/manifest.json'));
  const sheet = decodePng(fs.readFileSync(root + '/sprites16.png'));
  assert.deepEqual([sheet.width, sheet.height], [columns * 16, 32]); assert.equal(manifest.sprites.length, count);
  for (const sprite of manifest.sprites) {
    const image = decodePng(fs.readFileSync(root + '/' + sprite.file));
    assert.deepEqual([image.width, image.height], [16, 16]); let black = 0, transparent = 0;
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const i = (y * 16 + x) * 4, j = ((sprite.cell[1] * 16 + y) * sheet.width + sprite.cell[0] * 16 + x) * 4;
      assert.ok(image.pixels[i] === 0 || image.pixels[i] === 255);
      assert.equal(image.pixels[i + 1], image.pixels[i]); assert.equal(image.pixels[i + 2], image.pixels[i]);
      assert.ok(image.pixels[i + 3] === 0 || image.pixels[i + 3] === 255);
      assert.deepEqual(image.pixels.subarray(i, i + 4), sheet.pixels.subarray(j, j + 4));
      if (!image.pixels[i + 3]) transparent++;
      if (image.pixels[i] === 0) {
        black++; assert.equal(image.pixels[i + 3], 255);
        assert.ok(x > 0 && x < 15 && y > 0 && y < 15, 'outline fits cell');
        for (let dy = -1; dy <= 1; dy++) for (let dx = -1; dx <= 1; dx++)
          assert.equal(image.pixels[((y + dy) * 16 + x + dx) * 4 + 3], 255, 'one-pixel border includes corners');
      }
      if (x === 0 || x === 15) assert.equal(image.pixels[i], 255);
    }
    assert.ok(black > 20 && black < 180, sprite.name + ' visible silhouette');
    assert.ok(transparent > 40, sprite.name + ' background is transparent');
  }
});

test('player uses all four direction frames, turns on blocked steps and resets scene facing', () => {
  const h = harness(); h.select(2);
  for (const [dir, crop, walkCrop] of [['up', [16, 0], [32, 0]], ['down', [0, 0], [0, 0]], ['left', [0, 16], [0, 16]], ['right', [16, 16], [32, 16]]]) {
    h.click(dir); assert.deepEqual(h.player().slice(1, 3), walkCrop);
    h.advance(duration + 1); assert.deepEqual(h.player().slice(1, 3), crop);
  }
  h.select(0); assert.deepEqual(h.player().slice(1, 3), [16, 0]);
  h.click('left'); assert.match(h.status(), /^Blocked/); assert.deepEqual(h.player().slice(1, 3), [0, 16]);
});

test('walking poses change at half-step and idle resumes on rest, blocked turns and reset', () => {
  const h = harness(); h.click('down');
  assert.ok(h.player()[0].src.includes('/walk/')); assert.deepEqual(h.player().slice(1, 3), [0, 0]);
  h.advance(duration / 2); assert.deepEqual(h.player().slice(1, 3), [16, 0]);
  assert.deepEqual(h.player().slice(5), [56, 24, 16, 16]);
  h.advance(duration / 2 + 1); assert.ok(!h.player()[0].src.includes('/walk/'));
  h.click('right'); assert.match(h.status(), /^Blocked/); assert.ok(!h.player()[0].src.includes('/walk/'));
  h.click('down'); h.advance(duration / 4); h.select(2); assert.ok(!h.player()[0].src.includes('/walk/'));
});

test('held walking chains full cycles without returning to idle at the boundary', () => {
  const h = harness(); h.key('ArrowDown'); h.advance(duration / 2);
  assert.deepEqual(h.player().slice(1, 3), [16, 0]);
  h.advance(duration / 2 + 1); assert.ok(h.player()[0].src.includes('/walk/'));
  assert.deepEqual(h.player().slice(1, 3), [0, 0]);
  h.release('ArrowDown'); h.advance(duration + 1); assert.ok(!h.player()[0].src.includes('/walk/'));
});

test('each walking direction has two distinct leg poses and correct half-step crop', () => {
  const h = harness(); h.select(2);
  for (const [index, dir] of ['down', 'up', 'left', 'right'].entries()) {
    const a = decodePng(fs.readFileSync(`docs/assets/world-player/walk/${dir}_a.png`));
    const b = decodePng(fs.readFileSync(`docs/assets/world-player/walk/${dir}_b.png`));
    assert.notDeepEqual(a.pixels.subarray(10 * 16 * 4), b.pixels.subarray(10 * 16 * 4), dir + ' feet move');
    h.click(dir);
    assert.deepEqual(h.player().slice(1, 3), [(index * 2) % 4 * 16, Math.floor(index / 2) * 16]);
    h.advance(duration / 2 + 1);
    assert.deepEqual(h.player().slice(1, 3), [(index * 2 + 1) % 4 * 16, Math.floor(index / 2) * 16]);
    h.advance(duration / 2 + 1);
  }
});

test('four player poses differ and static previews use each scene starting direction', () => {
  const poses = ['down', 'up', 'left', 'right'].map(d => decodePng(fs.readFileSync(`docs/assets/world-player/${d}.png`)));
  assert.equal(new Set(poses.map(p => Buffer.from(p.pixels).toString('base64'))).size, 4);
  const root = 'docs/assets/world-environment-spike';
  const scenes = JSON.parse(fs.readFileSync(root + '/traversal-scenes.json')).scenes;
  scenes.forEach((scene, index) => {
    const frame = decodePng(fs.readFileSync(`${root}/traversal-${index}.png`));
    const pose = poses[['DOWN', 'UP', 'LEFT', 'RIGHT'].indexOf(scene.facing)];
    for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
      const i = (y * 16 + x) * 4, j = ((24 + y) * 128 + 56 + x) * 4;
      if (pose.pixels[i + 3]) assert.deepEqual(frame.pixels.subarray(j, j + 4), pose.pixels.subarray(i, i + 4));
    }
  });
});

test('front/back gait keeps native head and torso fixed and alternates two separate feet', () => {
  for (const dir of ['down', 'up']) {
    const idle = decodePng(fs.readFileSync(`docs/assets/world-player/${dir}.png`));
    const a = decodePng(fs.readFileSync(`docs/assets/world-player/walk/${dir}_a.png`));
    const b = decodePng(fs.readFileSync(`docs/assets/world-player/walk/${dir}_b.png`));
    for (let i = 0; i < 13 * 16; i++) {
      assert.equal(a.pixels[i * 4], idle.pixels[i * 4], 'head/face/chest/backpack black-white pixels remain fixed');
      assert.equal(b.pixels[i * 4], idle.pixels[i * 4], 'head/face/chest/backpack black-white pixels remain fixed');
    }
    assert.deepEqual(a.pixels.subarray(0, 12 * 16 * 4), idle.pixels.subarray(0, 12 * 16 * 4));
    assert.deepEqual(b.pixels.subarray(0, 12 * 16 * 4), idle.pixels.subarray(0, 12 * 16 * 4));
    for (const [x, longInA] of [[5, false], [8, true]]) {
      assert.equal(a.pixels[(14 * 16 + x) * 4] === 0, longInA);
      assert.equal(b.pixels[(14 * 16 + x) * 4] === 0, !longInA);
    }
    for (const pose of [a, b]) for (let y = 13; y < 15; y++) assert.equal(pose.pixels[(y * 16 + 7) * 4], 255);
  }
});

test('static scene compositing preserves path pixels outside transparent NPC silhouette', () => {
  const root = 'docs/assets/world-environment-spike';
  const scene = JSON.parse(fs.readFileSync(root + '/traversal-scenes.json')).scenes[0];
  const atlas = decodePng(fs.readFileSync(root + '/atlas-native.png'));
  const frame = decodePng(fs.readFileSync(root + '/traversal-0.png'));
  const npc = scene.npcs.find(n => n.name === 'Shopkeeper');
  const image = decodePng(fs.readFileSync('docs/assets/world-npcs/shopkeeper.png'));
  const tile = scene.data[npc.y * scene.width + npc.x] - 1;
  const cx = scene.player[0] * 16 - 56, cy = scene.player[1] * 16 - 24;
  for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
    const i = (y * 16 + x) * 4;
    const j = ((npc.y * 16 + y - cy) * 128 + npc.x * 16 + x - cx) * 4;
    const t = ((Math.floor(tile / 16) * 16 + y) * atlas.width + tile % 16 * 16 + x) * 4;
    const expected = image.pixels[i + 3] ? image.pixels.subarray(i, i + 4) : atlas.pixels.subarray(t, t + 4);
    assert.deepEqual(frame.pixels.subarray(j, j + 4), expected);
  }
});

test('shipping source and packed masked frames match all twelve approved poses', () => {
  const sheet = decodePng(fs.readFileSync('images/Playerchibi_16x16.png'));
  assert.deepEqual([sheet.width, sheet.height], [96, 32]);
  const header = fs.readFileSync('src/fxdata.h', 'utf8');
  const base = parseInt(header.match(/worldPlayerSprites = (0x[0-9A-Fa-f]+)/)[1]);
  assert.match(header, /worldPlayerSpritesFrames = 12/);
  const packed = fs.readFileSync('dist/fxdata-data.bin');
  ['up', 'right', 'down', 'left'].forEach((dir, d) => {
    [dir + '.png', `walk/${dir}_a.png`, `walk/${dir}_b.png`].forEach((file, pose) => {
      const image = decodePng(fs.readFileSync('docs/assets/world-player/' + file)), frame = d * 3 + pose;
      for (let y = 0; y < 16; y++) for (let x = 0; x < 16; x++) {
        const si = (y * 16 + x) * 4, di = ((Math.floor(frame / 6) * 16 + y) * 96 + frame % 6 * 16 + x) * 4;
        assert.deepEqual(sheet.pixels.subarray(di, di + 4), image.pixels.subarray(si, si + 4));
        const bit = 1 << (y % 8), pi = base + 4 + frame * 64 + (Math.floor(y / 8) * 16 + x) * 2;
        const opaque = image.pixels[si + 3] === 255;
        assert.equal(Boolean(packed[pi + 1] & bit), opaque, file + ' packed coverage');
        assert.equal(Boolean(packed[pi] & packed[pi + 1] & bit), opaque && image.pixels[si] === 255, file + ' packed pixel');
      }
    });
  });
});
