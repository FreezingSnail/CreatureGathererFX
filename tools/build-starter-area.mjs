#!/usr/bin/env node
// Build the first-beta starter town/route/collection area from the light-shadow
// fresh atlases. The shipping sheet is Soft blocks; noir and etched sheets use
// the same layout and GID mapping for comparison. Pinned first-beta GIDs
// (275/545/551/261/262) stay stable so the device and host fixtures keep
// meaning. Review-only unless the generated manifest is copied over the
// shipping cells; the shipping write is the final step of this script.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { decodePng, encodePng } from './creature-sprite-export.mjs';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const ASSET = path.join(ROOT, 'docs/assets/overworld-fresh');
const OUT = path.join(ASSET, 'starter-area');
const DIRECTIONS = ['rounded', 'noir', 'etched'];
const SHIPPING_DIRECTION = 'rounded';
const TREATMENT = 'light';
const TILE = 16;
const AREA = { x: 8, y: 0, width: 16, height: 12 };
const COLUMNS = 16;
const SHEET_ROWS = 48;
const SHEET_CELLS = COLUMNS * SHEET_ROWS;
const PINNED = { floor: 275, box: 545, plant: 551, westWall: 262, eastWall: 261 };
const VOID_CELL = { x: 5, y: 8 }; // local; device fixture requires GID 0 at world (13,8)
const start = { x: 3, y: 5 }; // local; world (11,5)

// ---------------------------------------------------------------------------
// Authored layout. Rows run v0..v11; columns u0..u15; world x = 8 + u.
// walk: 1 walkable. Everything walkable is an encounter tile, matching the
// first-beta contract and the four authored route chunks (extended to six).
// ---------------------------------------------------------------------------
const walkRows = [
  '0000000000000000',
  '0000111111000110',
  '0011110111000110',
  '0111111111111110',
  '0111111111111110',
  '0111111111111110',
  '0111111111111110',
  '0000101100000000',
  '0000101111111000',
  '0000111111111000',
  '0000000000000000',
  '0000000000000000',
];
const walk = walkRows.map((row) => [...row].map((c) => c === '1'));

// Ground: 0 grass, 1 dirt, 3 paving. These are catalog tile ids from the
// fresh atlas (not shipping GIDs); the flattener owns the GID mapping.
const ground = Array.from({ length: AREA.height }, () => Array(AREA.width).fill(0));
function groundRect(x, y, w, h, id) {
  for (let yy = y; yy < y + h; yy += 1) for (let xx = x; xx < x + w; xx += 1) ground[yy][xx] = id;
}
groundRect(1, 3, 5, 3, 1);   // town earth plaza
groundRect(2, 4, 3, 2, 3);   // small paved rest/start pad
groundRect(1, 6, 5, 1, 1);   // town south lane
groundRect(4, 6, 1, 4, 3);   // paved door corridor to the route
groundRect(6, 6, 2, 4, 191); // clearing connector
 groundRect(6, 8, 7, 1, 191); // route upper lane
 groundRect(4, 9, 9, 1, 191); // first route east to the boss
groundRect(1, 2, 2, 1, 3);   // box apron
groundRect(5, 2, 1, 1, 3);   // plant approach
groundRect(6, 2, 1, 1, 1);   // plant plot

const object = Array.from({ length: AREA.height }, () => Array(AREA.width).fill(null));
function putObject(x, y, rows) {
  for (let yy = 0; yy < rows.length; yy += 1) {
    for (let xx = 0; xx < rows[yy].length; xx += 1) {
      const id = rows[yy][xx];
      if (id === null) continue;
      const u = x + xx, v = y + yy;
      if (u < 0 || v < 0 || u >= AREA.width || v >= AREA.height) throw Error(`object out of bounds (${u},${v})`);
      if (object[v][u] !== null) throw Error(`object overlap at (${u},${v})`);
      object[v][u] = id;
    }
  }
}
const cottage = [[57, 58, 59], [60, 61, 62]];
const shop = [[63, 64, 65], [66, 67, 68], [69, 70, 71]];
const tree = [[49, 50], [51, 52]];
const pine = [[53], [54]];
const lamp = [[72], [73]];

putObject(1, 0, cottage);           // town cottage on the north edge
putObject(10, 0, shop);             // collection-area shop landmark
for (const [x, y] of [[0, 7], [2, 7], [14, 7],
                      [0, 10], [3, 10], [6, 10], [9, 10], [12, 10]]) putObject(x, y, tree);
for (const u of [8, 9, 10, 11, 12]) putObject(u, 7, [[55]]);
putObject(0, 1, lamp);
putObject(15, 1, lamp);
for (const [x, y] of [[0, 3], [15, 3], [0, 5], [15, 5]]) putObject(x, y, pine);
for (const u of [0, 4, 5, 6, 7, 8, 9, 13, 14, 15]) putObject(u, 0, [[55]]);
for (const u of [0, 1, 2, 3, 14, 15]) putObject(u, 9, [[55]]);
putObject(1, 2, [[56]]);            // lure box (pinned GID 545)
putObject(6, 2, [[48]]);            // plant plot (pinned GID 551)
putObject(5, 7, [[56]]);            // east door jamb boulder (pinned GID 261)
putObject(13, 9, [[74]]);           // boss landmark crystal

// Walkable decoration: flowers/crops overlay only; they never change collision.
for (const [u, v] of [[2, 3], [5, 5], [3, 6], [7, 3], [9, 4], [12, 3], [13, 3], [14, 4],
                      [6, 9], [9, 9], [11, 5]]) putObject(u, v, [[47]]);
for (const [u, v] of [[7, 4], [8, 4], [9, 4], [7, 5], [8, 5]]) {
  if (object[v][u] === null) object[v][u] = 48;
}

// Pinned floor cells must share the plain paving combo; clear decoration there.
for (const [u, v] of [[start.x, start.y], [4, 6], [4, 7], [4, 8]]) {
  object[v][u] = null;
  ground[v][u] = 3;
}
if (object[2][1] !== 56) throw Error('box cell must carry the boulder art');
if (object[2][6] !== 48) throw Error('plant cell must carry the crops art');

// ---------------------------------------------------------------------------
// Validation over the authored layout.
// ---------------------------------------------------------------------------
if (walk.length !== AREA.height) throw Error('walk rows do not cover the area');
walk.forEach((row, v) => { if (row.length !== AREA.width) throw Error(`walk row ${v} has ${row.length} cells`); });
const walkable = new Set();
for (let v = 0; v < AREA.height; v += 1) {
  for (let u = 0; u < AREA.width; u += 1) {
    if (walk[v][u]) walkable.add(`${u},${v}`);
  }
}
const voidKey = `${VOID_CELL.x},${VOID_CELL.y}`;
if (walk[VOID_CELL.y][VOID_CELL.x]) throw Error('void fixture cell cannot be walkable');
const queue = [[start.x, start.y]];
const seen = new Set([`${start.x},${start.y}`]);
while (queue.length) {
  const [u, v] = queue.shift();
  for (const [du, dv] of [[1, 0], [-1, 0], [0, 1], [0, -1]]) {
    const nu = u + du, nv = v + dv, key = `${nu},${nv}`;
    if (seen.has(key) || !walkable.has(key)) continue;
    seen.add(key);
    queue.push([nu, nv]);
  }
}
if (seen.size !== walkable.size) throw Error(`walkable graph splits: ${seen.size}/${walkable.size}`);
for (const key of walkable) {
  const [u, v] = key.split(',').map(Number);
  if (u < 0 || v < 0 || u >= AREA.width || v >= AREA.height) throw Error(`walk escapes bounds at ${key}`);
}
const pinned = {
  start: [3, 5], box: [1, 2], plant: [6, 2], rest: [2, 4],
  floorNorth: [4, 6], floor: [4, 7], floorSouth: [4, 8],
  westWall: [3, 7], eastWall: [5, 7], empty: [5, 8], boss: [13, 9],
};
const nonWalkablePins = new Set(['box', 'plant', 'westWall', 'eastWall', 'empty', 'boss']);
for (const [name, [u, v]] of Object.entries(pinned)) {
  if (nonWalkablePins.has(name)) {
    if (walk[v][u]) throw Error(`pinned ${name} (${u},${v}) must be non-walkable`);
  } else if (!walk[v][u]) {
    throw Error(`pinned ${name} (${u},${v}) must be walkable`);
  }
}
for (const [name, [u, v]] of [['box approach', [2, 2]], ['plant approach', [5, 2]]]) {
  if (!walk[v][u]) throw Error(`${name} (${u},${v}) must be walkable`);
}
for (const [u, v] of [[start.x - 1, start.y], [start.x + 1, start.y], [start.x, start.y - 1], [start.x, start.y + 1]]) {
  if (!walk[v][u]) throw Error(`start neighbour (${u},${v}) must be walkable`);
}

// ---------------------------------------------------------------------------
// Load the three light atlases.
// ---------------------------------------------------------------------------
const atlases = new Map();
for (const direction of DIRECTIONS) {
  const file = path.join(ASSET, direction, TREATMENT, 'atlas.png');
  const image = decodePng(fs.readFileSync(file));
  if (image.width % TILE || image.height % TILE) throw Error(`${file} is not a 16px atlas`);
  atlases.set(direction, image);
}

function atlasTile(image, id) {
  const col = id % COLUMNS, row = Math.floor(id / COLUMNS);
  const x0 = col * TILE, y0 = row * TILE;
  if (x0 + TILE > image.width || y0 + TILE > image.height) throw Error(`catalog id ${id} outside atlas`);
  const out = { width: TILE, height: TILE, pixels: new Uint8Array(TILE * TILE * 4) };
  for (let y = 0; y < TILE; y += 1) {
    for (let x = 0; x < TILE; x += 1) {
      const si = ((y0 + y) * image.width + x0 + x) * 4;
      out.pixels.set(image.pixels.subarray(si, si + 4), (y * TILE + x) * 4);
    }
  }
  return out;
}
function whiteTile() {
  const out = { width: TILE, height: TILE, pixels: new Uint8Array(TILE * TILE * 4) };
  for (let i = 0; i < out.pixels.length; i += 4) {
    out.pixels[i] = out.pixels[i + 1] = out.pixels[i + 2] = 255;
    out.pixels[i + 3] = 255;
  }
  return out;
}
function composite(base, top) {
  const out = { width: TILE, height: TILE, pixels: base.pixels.slice() };
  for (let i = 0; i < out.pixels.length; i += 4) {
    if (top.pixels[i + 3] < 128) continue;
    out.pixels.set(top.pixels.subarray(i, i + 4), i);
  }
  return out;
}
function binary(image) {
  const out = { width: image.width, height: image.height, pixels: new Uint8Array(image.pixels.length) };
  for (let i = 0; i < image.pixels.length; i += 4) {
    const opaque = image.pixels[i + 3] >= 128;
    const luma = 77 * image.pixels[i] + 150 * image.pixels[i + 1] + 29 * image.pixels[i + 2];
    const value = opaque && luma < 32768 ? 0 : 255;
    out.pixels[i] = out.pixels[i + 1] = out.pixels[i + 2] = value;
    out.pixels[i + 3] = 255;
  }
  return out;
}
function composeCell(direction, groundId, objectId) {
  const atlas = atlases.get(direction);
  let image = whiteTile();
  image = composite(image, atlasTile(atlas, groundId));
  if (objectId !== null) image = composite(image, atlasTile(atlas, objectId));
  return binary(image);
}

// ---------------------------------------------------------------------------
// Flatten every authored ground/object combination to one GID.
// Reserved pins are assigned by meaning; all other combos take fresh IDs.
// ---------------------------------------------------------------------------
const cells = [];
const combos = new Map();
for (let v = 0; v < AREA.height; v += 1) {
  for (let u = 0; u < AREA.width; u += 1) {
    if (u === VOID_CELL.x && v === VOID_CELL.y) {
      cells.push({ u, v, gid: 0, ground: null, object: null, walkable: false });
      continue;
    }
    const key = `${ground[v][u]}:${object[v][u] ?? -1}:${walk[v][u] ? 1 : 0}`;
    if (!combos.has(key)) {
      combos.set(key, { ground: ground[v][u], object: object[v][u], walkable: walk[v][u], cells: [] });
    }
    combos.get(key).cells.push([u, v]);
    cells.push({ u, v, key, gid: null, ground: ground[v][u], object: object[v][u], walkable: walk[v][u] });
  }
}
const comboKey = (v, u) => `${ground[v][u]}:${object[v][u] ?? -1}:${walk[v][u] ? 1 : 0}`;
const reservedByKey = new Map([
  [comboKey(pinned.start[1], pinned.start[0]), PINNED.floor],
  [comboKey(2, 1), PINNED.box],
  [comboKey(2, 6), PINNED.plant],
  [comboKey(7, 3), PINNED.westWall],
  [comboKey(7, 5), PINNED.eastWall],
]);
for (const [key, gid] of reservedByKey) {
  if (!combos.has(key)) throw Error(`reserved combo ${key} is not authored`);
  combos.get(key).gid = gid;
}
const usedGids = new Set(reservedByKey.values());
let nextGid = 600;
for (const [key, combo] of combos) {
  if (combo.gid !== undefined) continue;
  while (usedGids.has(nextGid)) nextGid += 1;
  if (nextGid >= SHEET_CELLS) throw Error(`no sheet cells left at combo ${key}`);
  combo.gid = nextGid;
  usedGids.add(nextGid);
  nextGid += 1;
}
for (const cell of cells) {
  if (cell.gid === 0) continue;
  cell.gid = combos.get(cell.key).gid;
}
const byGid = new Map([...combos.values()].map((combo) => [combo.gid, combo]));

// ---------------------------------------------------------------------------
// Shipping sheet mutations: copy the base sheet, then install the fresh cells.
// ---------------------------------------------------------------------------
const baseSheet = decodePng(fs.readFileSync(path.join(ROOT, 'images/tiles.png')));
if (baseSheet.width !== COLUMNS * TILE || baseSheet.height !== SHEET_ROWS * TILE) {
  throw Error(`images/tiles.png must stay ${COLUMNS * TILE}x${SHEET_ROWS * TILE}`);
}
const sheetCache = new Map();

function sheetFor(direction) {
  if (sheetCache.has(direction)) return sheetCache.get(direction);
  const sheet = { width: baseSheet.width, height: baseSheet.height, pixels: baseSheet.pixels.slice() };
  for (const [gid, combo] of byGid) {
    const cell = composeCell(direction, combo.ground, combo.object);
    const index = gid - 1;
    const cx = (index % COLUMNS) * TILE, cy = Math.floor(index / COLUMNS) * TILE;
    for (let y = 0; y < TILE; y += 1) {
      for (let x = 0; x < TILE; x += 1) {
        const si = (y * TILE + x) * 4;
        const di = ((cy + y) * sheet.width + cx + x) * 4;
        sheet.pixels.set(cell.pixels.subarray(si, si + 4), di);
      }
    }
  }
  // The Tiled source sheet stays binary 0/255 RGB with opaque cells for
  // authored combos; every other cell copies the existing binary base.
  sheetCache.set(direction, sheet);
  return sheet;
}

// ---------------------------------------------------------------------------
// TSX: keep every unrelated property entry, replace the authored set.
// ---------------------------------------------------------------------------
const authoredProps = new Map();
for (const [gid, combo] of byGid) {
  const props = [];
  props.push(`<property name="walkable" type="bool" value="${combo.walkable ? 'true' : 'false'}"/>`);
  if (combo.walkable) props.push('<property name="encounter" type="bool" value="true"/>');
  if (gid === PINNED.box) props.push('<property name="spare" type="bool" value="true"/>');
  if (gid === PINNED.plant) props.push('<property name="plantable" type="bool" value="true"/>');
  authoredProps.set(gid, `<tile id="${gid - 1}"><properties>${props.join('')}</properties></tile>`);
}
if (!authoredProps.get(PINNED.floor)?.includes('walkable" type="bool" value="true"')) {
  throw Error('pinned floor must be walkable');
}
if (!authoredProps.get(PINNED.box)?.includes('spare')) throw Error('pinned box lost spare');
if (!authoredProps.get(PINNED.plant)?.includes('plantable')) throw Error('pinned plant lost plantable');

let tsx = fs.readFileSync(path.join(ROOT, 'maps/tiles.tsx'), 'utf8');
for (const gid of authoredProps.keys()) {
  tsx = tsx.replace(new RegExp(`\\s*<tile id="${gid - 1}">[\\s\\S]*?<\\/tile>`, 'g'), '');
}
const authoredXml = [...authoredProps.entries()]
  .sort((a, b) => a[0] - b[0])
  .map(([, xml]) => ` ${xml}`)
  .join('\n');
tsx = tsx.replace(/(\s*<\/tileset>\s*)$/, `\n${authoredXml}\n</tileset>\n`);
fs.writeFileSync(path.join(ROOT, 'maps/tiles.tsx'), tsx);

// ---------------------------------------------------------------------------
// World map: replace only the 16x12 authored area; preserve every other cell,
// NPC, script and metadata field.
// ---------------------------------------------------------------------------
const mapPath = path.join(ROOT, 'maps/world_map.json');
const world = JSON.parse(fs.readFileSync(mapPath, 'utf8'));
for (const cell of cells) {
  world.tiles[AREA.y + cell.v][AREA.x + cell.u] = cell.gid;
}
fs.writeFileSync(mapPath, `${JSON.stringify(world, null, 2)}\n`);

// ---------------------------------------------------------------------------
// Review exports: one sheet per direction plus native crops.
// ---------------------------------------------------------------------------
fs.mkdirSync(OUT, { recursive: true });
const player = decodePng(fs.readFileSync(path.join(ROOT, 'images/Playerchibi_16x16.png')));
const downFrame = { width: 16, height: 16, pixels: new Uint8Array(16 * 16 * 4) };
for (let y = 0; y < 16; y += 1) {
  const si = ((16 + y) * player.width) * 4;
  downFrame.pixels.set(player.pixels.subarray(si, si + 16 * 4), y * 16 * 4);
}

function worldImage(direction) {
  const atlas = atlases.get(direction);
  const image = { width: AREA.width * TILE, height: AREA.height * TILE, pixels: new Uint8Array(AREA.width * TILE * AREA.height * TILE * 4).fill(255) };
  for (let v = 0; v < AREA.height; v += 1) {
    for (let u = 0; u < AREA.width; u += 1) {
      if (u === VOID_CELL.x && v === VOID_CELL.y) continue;
      const tile = composeCell(direction, ground[v][u], object[v][u]);
      for (let y = 0; y < TILE; y += 1) {
        for (let x = 0; x < TILE; x += 1) {
          const si = (y * TILE + x) * 4;
          const di = ((v * TILE + y) * image.width + u * TILE + x) * 4;
          image.pixels.set(tile.pixels.subarray(si, si + 4), di);
        }
      }
    }
  }
  return image;
}
function cropView(world, playerX, playerY) {
  const cameraX = playerX * TILE - 56;
  const cameraY = playerY * TILE - 24;
  const out = { width: 128, height: 64, pixels: new Uint8Array(128 * 64 * 4).fill(255) };
  for (let y = 0; y < 64; y += 1) {
    for (let x = 0; x < 128; x += 1) {
      const sx = x + cameraX, sy = y + cameraY;
      if (sx < 0 || sy < 0 || sx >= world.width || sy >= world.height) continue;
      const si = (sy * world.width + sx) * 4;
      const di = (y * 128 + x) * 4;
      out.pixels.set(world.pixels.subarray(si, si + 4), di);
    }
  }
  for (let y = 0; y < 16; y += 1) {
    for (let x = 0; x < 16; x += 1) {
      const si = (y * 16 + x) * 4;
      if (downFrame.pixels[si + 3] < 128) continue;
      const di = ((24 + y) * 128 + 56 + x) * 4;
      const black = 77 * downFrame.pixels[si] + 150 * downFrame.pixels[si + 1] + 29 * downFrame.pixels[si + 2] < 32768;
      out.pixels[di] = out.pixels[di + 1] = out.pixels[di + 2] = black ? 0 : 255;
      out.pixels[di + 3] = 255;
    }
  }
  return out;
}
const views = [
  { name: 'town', x: start.x, y: start.y },
  { name: 'route', x: 8, y: 9 },
  { name: 'collection', x: 11, y: 4 },
];
const manifest = {
  reviewOnly: false,
  description: 'First-beta starter town, first route and collection area rebuilt from the light-shadow fresh atlases.',
  area: { x: AREA.x, y: AREA.y, width: AREA.width, height: AREA.height },
  treatment: TREATMENT,
  shippingDirection: SHIPPING_DIRECTION,
  directions: DIRECTIONS,
  columns: COLUMNS,
  sheet: [COLUMNS * TILE, SHEET_ROWS * TILE],
  pinnedGids: PINNED,
  walkableCells: walkable.size,
  zones: [1, 2, 33, 34, 65, 66],
  layout: {
    walk: walkRows.map((row) => [...row].map((c) => (c === '1' ? 1 : 0))),
    ground,
    objects: object,
  },
  combos: [...byGid.entries()].sort((a, b) => a[0] - b[0]).map(([gid, combo]) => ({
    gid, ground: combo.ground, object: combo.object, walkable: combo.walkable, cells: combo.cells,
  })),
  views,
};
for (const direction of DIRECTIONS) {
  const dirOut = path.join(OUT, direction);
  fs.mkdirSync(dirOut, { recursive: true });
  const sheet = sheetFor(direction);
  fs.writeFileSync(path.join(dirOut, 'tiles.png'), encodePng(sheet));
  const image = worldImage(direction);
  fs.writeFileSync(path.join(dirOut, 'world.png'), encodePng(image));
  const viewDir = path.join(dirOut, 'views');
  fs.mkdirSync(viewDir, { recursive: true });
  for (const view of views) {
    fs.writeFileSync(path.join(viewDir, `${view.name}.png`), encodePng(cropView(image, view.x, view.y)));
  }
}
const shipping = sheetFor(SHIPPING_DIRECTION);
fs.writeFileSync(path.join(ROOT, 'images/tiles.png'), encodePng(shipping));
fs.writeFileSync(path.join(ROOT, 'images/tiles_16x16.png'), encodePng(shipping));
fs.writeFileSync(path.join(OUT, 'manifest.json'), `${JSON.stringify(manifest, null, 2)}\n`);
console.log(`PASS: ${walkable.size} walkable cells, ${byGid.size} combos; shipping ${SHIPPING_DIRECTION}; sheets ${DIRECTIONS.join('/')}`);
