// Starter-area art/content agreement. The generator owns the shipping sheet,
// the TSX property entries and the authored 16x12 map cells; this test keeps a
// one-sided edit from silently splitting them. Run `make gen` first.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { decodePng } from '../creature-sprite-export.mjs';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const readText = (rel) => fs.readFileSync(path.join(ROOT, rel), 'utf8');
const readJson = (rel) => JSON.parse(readText(rel));
const image = (rel) => decodePng(fs.readFileSync(path.join(ROOT, rel)));
const OUT = 'docs/assets/overworld-fresh/starter-area';
const manifest = readJson(`${OUT}/manifest.json`);
const map = readJson('maps/world_map.json');
const tsx = readText('maps/tiles.tsx');
const columns = manifest.columns;
const tile = 16;

function tsxProps(gid) {
  const match = tsx.match(new RegExp(`<tile id="${gid - 1}">([\\s\\S]*?)</tile>`));
  if (!match) return null;
  const props = {};
  for (const property of match[1].matchAll(/<property name="([^"]+)"[^>]*value="([^"]*)"/g)) {
    props[property[1]] = property[2];
  }
  return props;
}
function cellPixels(sheet, gid) {
  const index = gid - 1;
  const col = index % columns, row = Math.floor(index / columns);
  return { x: col * tile, y: row * tile };
}

test('manifest pins the authored area, six chunks and 93 walkable cells', () => {
  assert.equal(manifest.reviewOnly, false);
  assert.deepEqual(manifest.area, { x: 8, y: 0, width: 16, height: 12 });
  assert.equal(manifest.treatment, 'light');
  assert.equal(manifest.shippingDirection, 'rounded');
  assert.deepEqual(manifest.directions, ['rounded', 'noir', 'etched']);
  assert.equal(manifest.walkableCells, 93);
  assert.deepEqual(manifest.zones, [1, 2, 33, 34, 65, 66]);
  assert.deepEqual(manifest.pinnedGids, { floor: 275, box: 545, plant: 551, westWall: 262, eastWall: 261 });
});

test('shipping map cells use the generated combos and preserve pinned fixtures', () => {
  const byGid = new Map(manifest.combos.map((combo) => [combo.gid, combo]));
  for (const combo of manifest.combos) {
    for (const [u, v] of combo.cells) {
      assert.equal(map.tiles[manifest.area.y + v][manifest.area.x + u], combo.gid,
        `combo ${combo.gid} at local (${u},${v})`);
    }
    assert.ok(combo.gid >= 1 && combo.gid < columns * (manifest.sheet[1] / tile),
      `combo ${combo.gid} is inside the sheet`);
    assert.equal(byGid.get(combo.gid), combo, 'one combo owns each GID');
  }
  assert.equal(map.tiles[8][13], 0, 'void fixture (13,8) stays GID 0');
  assert.equal(map.tiles[2][9], 545, 'lure box pin');
  assert.equal(map.tiles[2][14], 551, 'plant pin');
  assert.equal(map.tiles[7][12], 275, 'pinned floor');
  assert.equal(map.tiles[7][11], 262, 'west wall pin');
  assert.equal(map.tiles[7][13], 261, 'east wall pin');
  assert.equal(map.tiles[5][11], 275, 'spawn floor');
  // Unrelated authored fixtures outside the starter area are preserved.
  assert.equal(map.tiles[2][3], 8, 'west-room demo cell preserved');
  assert.equal(map.tiles[255][252], 530, 'sealed border fixture preserved');
});

test('TSX properties match every generated combo', () => {
  for (const combo of manifest.combos) {
    const props = tsxProps(combo.gid);
    assert.ok(props, `GID ${combo.gid} has a TSX property entry`);
    assert.equal(props.walkable, combo.walkable ? 'true' : 'false', `GID ${combo.gid} walkable`);
    if (combo.walkable) assert.equal(props.encounter, 'true', `GID ${combo.gid} encounter`);
    else assert.equal(props.encounter, undefined, `GID ${combo.gid} does not invent encounter`);
    if (combo.gid === manifest.pinnedGids.box) assert.equal(props.spare, 'true');
    if (combo.gid === manifest.pinnedGids.plant) assert.equal(props.plantable, 'true');
  }
});

test('shipping sheet and the three candidate sheets carry the same GID cells', () => {
  const shipping = image('images/tiles.png');
  const packedSource = image('images/tiles_16x16.png');
  assert.deepEqual([shipping.width, shipping.height], [columns * tile, manifest.sheet[1]]);
  assert.deepEqual(packedSource.pixels, shipping.pixels, 'Tiled and packed source sheets agree');
  const rounded = image(`${OUT}/rounded/tiles.png`);
  assert.deepEqual(rounded.pixels, shipping.pixels, 'shipping sheet is the rounded candidate');
  for (const direction of manifest.directions) {
    const sheet = image(`${OUT}/${direction}/tiles.png`);
    assert.deepEqual([sheet.width, sheet.height], [shipping.width, shipping.height]);
    for (const combo of manifest.combos) {
      const { x, y } = cellPixels(sheet, combo.gid);
      for (let py = 0; py < tile; py += 1) {
        for (let px = 0; px < tile; px += 1) {
          const i = ((y + py) * sheet.width + x + px) * 4;
          const [r, g, b, a] = sheet.pixels.subarray(i, i + 4);
          assert.ok(a === 255 && r === g && g === b && (r === 0 || r === 255),
            `${direction} GID ${combo.gid} is binary opaque art`);
        }
      }
    }
  }
  const noir = image(`${OUT}/noir/tiles.png`);
  const etched = image(`${OUT}/etched/tiles.png`);
  assert.notDeepEqual(noir.pixels, shipping.pixels, 'noir is a distinct art option');
  assert.notDeepEqual(etched.pixels, shipping.pixels, 'etched is a distinct art option');
});

test('editor sheet indexes shipping GID art at the map GID, with a blank frame 0', () => {
  const shipping = image('images/tiles.png');
  const editor = image(manifest.editorSheet);
  assert.deepEqual([editor.width, editor.height], [shipping.width, shipping.height + tile]);
  const project = readJson('cgfx-project.json');
  assert.equal(project.map_tileset_png, manifest.editorSheet);
  assert.equal(project.tileset_png, 'images/tiles.png');
  for (let gid = 1; gid < columns * (manifest.sheet[1] / tile); gid += 1) {
    const sx = ((gid - 1) % columns) * tile;
    const sy = Math.floor((gid - 1) / columns) * tile;
    const ex = (gid % columns) * tile;
    const ey = Math.floor(gid / columns) * tile;
    for (let y = 0; y < tile; y += 1) {
      for (let x = 0; x < tile; x += 1) {
        const si = ((sy + y) * shipping.width + sx + x) * 4;
        const ei = ((ey + y) * editor.width + ex + x) * 4;
        assert.deepEqual(editor.pixels.subarray(ei, ei + 4), shipping.pixels.subarray(si, si + 4),
          `editor index ${gid} mirrors shipping GID ${gid}`);
      }
    }
  }
  for (let y = 0; y < tile; y += 1) {
    for (let x = 0; x < tile; x += 1) {
      const i = (y * editor.width + x) * 4;
      assert.deepEqual(editor.pixels.subarray(i, i + 4), new Uint8Array([255, 255, 255, 255]),
        'editor frame 0 is blank');
    }
  }
});

test('native previews exist for all three directions and are 128x64', () => {
  for (const direction of manifest.directions) {
    for (const view of manifest.views) {
      const preview = image(`${OUT}/${direction}/views/${view.name}.png`);
      assert.deepEqual([preview.width, preview.height], [128, 64], `${direction}/${view.name}`);
    }
  }
});
