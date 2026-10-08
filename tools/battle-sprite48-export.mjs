#!/usr/bin/env node
// Export the reviewed paired boards and expansion tiles; packing remains make gen.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { decodePng, encodePng, bounds, exportView, validateSprite } from './creature-sprite-export.mjs';

const rowCache = new WeakMap();
function boardRows(board) {
  if (rowCache.has(board)) return rowCache.get(board);
  const bands = [];
  for (let y = 0; y < board.height; y++) {
    let white = 0;
    for (let x = 0; x < board.width; x++) white += board.pixels[(y * board.width + x) * 4] >= 128;
    if (white < board.width * 0.95) continue;
    if (bands.length && bands.at(-1).end === y) bands.at(-1).end++;
    else bands.push({ start: y, end: y + 1 });
  }
  if (bands.length < 3 || bands.length > 4) throw Error('Expected three row separators and optional bottom rule');
  const rows = Array.from({ length: 4 }, (_, row) => ({ start: row ? bands[row - 1].end : 0, end: bands[row]?.start ?? board.height }));
  rowCache.set(board, rows); return rows;
}

export function maskBoardCell(board, column, row) {
  // Keep tall anatomy beside the ID, removing the label by its own rectangle.
  const span = boardRows(board)[row];
  const y = span.start + 2;
  const height = span.end - 2 - y;
  const cuts = [0];
  for (let c = 1; c < 4; c++) {
    const expected = c * board.width / 4, gaps = [];
    for (let xx = Math.floor(expected - board.width / 12); xx < expected + board.width / 12; xx++) {
      let occupied = false;
      for (let yy = 0; yy < height && !occupied; yy++)
        occupied = board.pixels[((y + yy) * board.width + xx) * 4] >= 128;
      if (!occupied) gaps.push(xx);
    }
    if (!gaps.length) throw Error(`Sprites have no safe vertical gutter: ${c}/${row}`);
    cuts.push(gaps.reduce((best, at) => Math.abs(at - expected) < Math.abs(best - expected) ? at : best));
  }
  cuts.push(board.width - 1);
  const x = cuts[column], width = cuts[column + 1] - x + 1;
  const im = { width, height, pixels: new Uint8Array(width * height * 4) };
  for (let yy = 0; yy < height; yy++) for (let xx = 0; xx < width; xx++) {
    const src = ((y + yy) * board.width + x + xx) * 4;
    const isLabel = column % 2 === 0 && xx < width * 0.38 && yy < board.height / 4 * 0.15;
    const value = !isLabel && board.pixels[src] >= 128 ? 255 : 0;
    im.pixels.set([value, value, value, 255], (yy * width + xx) * 4);
  }
  // Remove detached, thin grounding strokes; retain feet and all body clusters.
  const visited = new Uint8Array(width * height);
  for (let n = 0; n < visited.length; n++) {
    if (visited[n] || !im.pixels[n * 4]) continue;
    const queue = [n]; visited[n] = 1;
    let left = width, right = 0, top = height, bottom = 0;
    for (let q = 0; q < queue.length; q++) {
      const at = queue[q], cx = at % width, cy = Math.floor(at / width);
      left = Math.min(left, cx); right = Math.max(right, cx);
      top = Math.min(top, cy); bottom = Math.max(bottom, cy);
      for (const next of [cx ? at - 1 : -1, cx + 1 < width ? at + 1 : -1,
                         cy ? at - width : -1, cy + 1 < height ? at + width : -1]) {
        if (next < 0 || visited[next] || !im.pixels[next * 4]) continue;
        visited[next] = 1; queue.push(next);
      }
    }
    if (top > height * 0.8 && right - left > width * 0.65 && bottom - top < height * 0.04)
      for (const at of queue) im.pixels[at * 4] = im.pixels[at * 4 + 1] = im.pixels[at * 4 + 2] = 0;
  }
  // Border-connected black is backdrop; enclosed black remains opaque detail.
  const exterior = new Uint8Array(width * height), queue = [];
  function seed(at) {
    if (!exterior[at] && !im.pixels[at * 4]) { exterior[at] = 1; queue.push(at); }
  }
  for (let xx = 0; xx < width; xx++) { seed(xx); seed((height - 1) * width + xx); }
  for (let yy = 0; yy < height; yy++) { seed(yy * width); seed(yy * width + width - 1); }
  for (let q = 0; q < queue.length; q++) {
    const at = queue[q], cx = at % width, cy = Math.floor(at / width);
    if (cx) seed(at - 1);
    if (cx + 1 < width) seed(at + 1);
    if (cy) seed(at - width);
    if (cy + 1 < height) seed(at + width);
  }
  for (let n = 0; n < exterior.length; n++) if (exterior[n]) im.pixels[n * 4 + 3] = 0;
  const crop = bounds(im, { x: 0, y: 0, width, height });
  if (!crop.x || !crop.y || crop.x + crop.width === width || crop.y + crop.height === height)
    throw Error(`Board sprite crosses extraction boundary: ${column}/${row} ${JSON.stringify({crop,width,height})}`);
  return { im, crop, sourceCell: { x, y, width, height } };
}

function paste(to, from, x, y, scale = 1) {
  for (let yy = 0; yy < from.height * scale; yy++) for (let xx = 0; xx < from.width * scale; xx++) {
    const src = (Math.floor(yy / scale) * from.width + Math.floor(xx / scale)) * 4;
    to.pixels.set(from.pixels.subarray(src, src + 4), ((y + yy) * to.width + x + xx) * 4);
  }
}

export function exportBattle48() {
  const creatures = JSON.parse(fs.readFileSync('data/json/creatures.json'));
  const orientations = JSON.parse(fs.readFileSync('data/json/battle_sprite_orientation.json')).fronts;
  if (Object.keys(orientations).length !== creatures.length) throw Error('Orientation roster must cover every creature');
  const root = 'docs/assets/battle-sprites48';
  fs.mkdirSync(`${root}/48x48`, { recursive: true });
  const sheet = { width: 96, height: 48 * 64, pixels: new Uint8Array(96 * 48 * 64 * 4) };
  const manifest = { size: 48, method: 'reviewed boards: label-band removal, exterior flood mask, common paired area sampling; expansion: existing native tiles', sprites: [] };
  for (let group = 0; group < 8; group++) {
    const contact = { width: 192, height: 192, pixels: new Uint8Array(192 * 192 * 4) };
    const source = group < 4 ? `docs/assets/creature-roster/mockup-v2-${group + 1}.png` : null;
    const board = source ? decodePng(fs.readFileSync(source)) : null;
    for (let offset = 0; offset < 8; offset++) {
      const creature = creatures[group * 8 + offset];
      const pair = board ? [0, 1].map(view => maskBoardCell(board, (offset % 2) * 2 + view, Math.floor(offset / 2))) : null;
      const extent = pair ? Math.max(...pair.flatMap(p => [p.crop.width, p.crop.height])) : 0;
      for (let view = 0; view < 2; view++) {
        const pose = view ? 'back' : 'front';
        const name = `${creature.id}-${creature.name.toLowerCase()}-${pose}_48x48.png`;
        let sprite = pair ? exportView(pair[view].im, pair[view].crop, 48, extent)
          : decodePng(fs.readFileSync(`docs/assets/creature-expansion/native/48x48/${name}`));
        const sourceFacing = orientations[creature.name];
        if (!['left', 'right', 'front'].includes(sourceFacing)) throw Error(`Missing orientation for ${creature.name}`);
        const mirrored = !view && sourceFacing === 'left';
        if (mirrored) sprite = mirrorHorizontal(sprite);
        const stats = validateSprite(sprite, 48);
        fs.writeFileSync(`${root}/48x48/${name}`, encodePng(sprite));
        paste(sheet, sprite, view * 48, creature.id * 48);
        const slot = offset * 2 + view;
        paste(contact, sprite, slot % 4 * 48, Math.floor(slot / 4) * 48);
        manifest.sprites.push({ id: creature.id, name: creature.name, view: pose, file: `48x48/${name}`, source: source || 'expansion/native/48x48', ...(view ? {} : { sourceFacing, facing: sourceFacing === 'front' ? 'front' : 'right' }), mirrored, ...(pair ? { sourceCell: pair[view].sourceCell, crop: pair[view].crop, pairExtent: extent } : {}), ...stats });
      }
    }
    fs.writeFileSync(`${root}/contact-${group + 1}.png`, encodePng(contact));
    const preview = { width: 768, height: 768, pixels: new Uint8Array(768 * 768 * 4) };
    paste(preview, contact, 0, 0, 4);
    fs.writeFileSync(`${root}/contact-${group + 1}-4x.png`, encodePng(preview));
  }
  fs.writeFileSync(`${root}/manifest.json`, JSON.stringify(manifest, null, 2) + '\n');
  fs.writeFileSync('images/Battlecreatures_48x48.png', encodePng(sheet));
  // Four native 128x16 option-menu frames, using the established 5x6 glyphs.
  const font = decodePng(fs.readFileSync('images/ArduFontTrimmed_5x6.png'));
  const menu = { width: 128, height: 64, pixels: new Uint8Array(128 * 64 * 4).fill(255) };
  const labels = ['move', 'gather', 'change', 'escape'];
  for (let frame = 0; frame < 4; frame++) for (let slot = 0; slot < 4; slot++) {
    const x = (slot % 2) * 64, y = frame * 16 + Math.floor(slot / 2) * 8;
    if (slot === frame) for (let yy = 0; yy < 8; yy++) for (let xx = 0; xx < 64; xx++) menu.pixels.set([0, 0, 0, 255], ((y + yy) * 128 + x + xx) * 4);
    for (let letter = 0; letter < labels[slot].length; letter++) for (let yy = 0; yy < 6; yy++) for (let xx = 0; xx < 5; xx++) {
      const src = (yy * font.width + (labels[slot].charCodeAt(letter) - 48) * 5 + xx) * 4;
      if (font.pixels[src] < 128) continue;
      const value = slot === frame ? 255 : 0;
      menu.pixels.set([value, value, value, 255], ((y + 1 + yy) * 128 + x + 5 + letter * 6 + xx) * 4);
    }
  }
  fs.writeFileSync('images/Battleoptions48_128x16.png', encodePng(menu));
  console.log('PASS: 128 validated native48 views, paired 96x3072 source and compact menu exported');
  return manifest;
}

export function mirrorHorizontal(sprite) {
  const pixels = new Uint8Array(sprite.pixels.length);
  for (let y = 0; y < sprite.height; y++) for (let x = 0; x < sprite.width; x++) {
    const source = (y * sprite.width + sprite.width - 1 - x) * 4;
    pixels.set(sprite.pixels.subarray(source, source + 4), (y * sprite.width + x) * 4);
  }
  return { ...sprite, pixels };
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) exportBattle48();
