#!/usr/bin/env node
// Preview-only imagegen atlas export. Never writes canonical game assets.
import fs from 'node:fs';
import path from 'node:path';
import zlib from 'node:zlib';
import { fileURLToPath } from 'node:url';

const signature = Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]);
function crc(bytes) {
  let value = 0xffffffff;
  for (const byte of bytes) {
    value ^= byte;
    for (let i = 0; i < 8; i++) value = (value >>> 1) ^ ((value & 1) ? 0xedb88320 : 0);
  }
  return (value ^ 0xffffffff) >>> 0;
}
function chunk(type, data) {
  const tag = Buffer.from(type), length = Buffer.alloc(4), checksum = Buffer.alloc(4);
  length.writeUInt32BE(data.length);
  checksum.writeUInt32BE(crc(Buffer.concat([tag, data])));
  return Buffer.concat([length, tag, data, checksum]);
}
export function decodePng(bytes) {
  if (!bytes.subarray(0, 8).equals(signature)) throw Error('Not a PNG');
  let width, height, channels, bitDepth, colorType, palette = null, transparency = null;
  const parts = [];
  for (let offset = 8; offset < bytes.length;) {
    const length = bytes.readUInt32BE(offset), end = offset + 8 + length;
    if (end + 4 > bytes.length) throw Error('Truncated PNG');
    const type = bytes.toString('ascii', offset + 4, offset + 8);
    const data = bytes.subarray(offset + 8, end);
    if (crc(bytes.subarray(offset + 4, end)) !== bytes.readUInt32BE(end)) throw Error('PNG CRC mismatch');
    if (type === 'IHDR') {
      width = data.readUInt32BE(0); height = data.readUInt32BE(4);
      bitDepth = data[8]; colorType = data[9];
      if (data[12] !== 0) throw Error('Interlaced PNG is not supported');
      if (colorType === 3) {
        // Indexed PNGs (as authored for images/tiles.png) expand to RGBA8.
        if (![1, 2, 4, 8].includes(bitDepth)) throw Error('Unsupported palette bit depth');
        channels = 1;
      } else {
        if (bitDepth !== 8 || ![2, 6].includes(colorType)) {
          throw Error('Expected non-interlaced RGB8 or RGBA8 PNG');
        }
        channels = colorType === 6 ? 4 : 3;
      }
    }
    if (type === 'PLTE') palette = data;
    if (type === 'tRNS') transparency = data;
    if (type === 'IDAT') parts.push(data);
    offset = end + 4;
  }
  if (!width || !height || !channels) throw Error('Missing PNG header');
  if (colorType === 3 && !palette) throw Error('Indexed PNG is missing its palette');
  const bytesPerPixel = colorType === 3 ? 1 : channels;
  const row = Math.ceil(width * (colorType === 3 ? bitDepth : channels * 8) / 8);
  const raw = zlib.inflateSync(Buffer.concat(parts));
  if (raw.length !== (row + 1) * height) throw Error('Unexpected scanline size');
  const scan = Buffer.alloc(row * height), pixels = new Uint8Array(width * height * 4);
  for (let y = 0; y < height; y++) {
    const filter = raw[y * (row + 1)];
    if (filter > 4) throw Error('Unknown PNG filter');
    for (let x = 0; x < row; x++) {
      const left = x >= bytesPerPixel ? scan[y * row + x - bytesPerPixel] : 0;
      const up = y ? scan[(y - 1) * row + x] : 0;
      const diagonal = y && x >= bytesPerPixel ? scan[(y - 1) * row + x - bytesPerPixel] : 0;
      let prediction = 0;
      if (filter === 1) prediction = left;
      if (filter === 2) prediction = up;
      if (filter === 3) prediction = Math.floor((left + up) / 2);
      if (filter === 4) {
        const p = left + up - diagonal;
        const a = Math.abs(p - left), b = Math.abs(p - up), c = Math.abs(p - diagonal);
        prediction = a <= b && a <= c ? left : b <= c ? up : diagonal;
      }
      scan[y * row + x] = (raw[y * (row + 1) + 1 + x] + prediction) & 255;
    }
  }
  if (colorType === 3) {
    const perByte = 8 / bitDepth, mask = (1 << bitDepth) - 1;
    for (let y = 0; y < height; y++) for (let x = 0; x < width; x++) {
      const byte = scan[y * row + Math.floor(x / perByte)];
      const index = (byte >> (8 - bitDepth * (x % perByte + 1))) & mask;
      const p = index * 3, i = (y * width + x) * 4;
      pixels[i] = palette[p]; pixels[i + 1] = palette[p + 1]; pixels[i + 2] = palette[p + 2];
      pixels[i + 3] = transparency && index < transparency.length ? transparency[index] : 255;
    }
  } else {
    for (let i = 0; i < width * height; i++) {
      pixels[i * 4] = scan[i * channels];
      pixels[i * 4 + 1] = scan[i * channels + 1];
      pixels[i * 4 + 2] = scan[i * channels + 2];
      pixels[i * 4 + 3] = channels === 4 ? scan[i * channels + 3] : 255;
    }
  }
  return { width, height, pixels };
}
export function encodePng({ width, height, pixels }) {
  const header = Buffer.alloc(13), row = width * 4;
  header.writeUInt32BE(width); header.writeUInt32BE(height, 4);
  header[8] = 8; header[9] = 6;
  const raw = Buffer.alloc((row + 1) * height);
  for (let y = 0; y < height; y++) raw.set(pixels.subarray(y * row, (y + 1) * row), y * (row + 1) + 1);
  return Buffer.concat([signature, chunk('IHDR', header), chunk('IDAT', zlib.deflateSync(raw)), chunk('IEND', Buffer.alloc(0))]);
}
export function bounds(image, cell) {
  let left = cell.x + cell.width, right = -1, top = cell.y + cell.height, bottom = -1;
  for (let y = cell.y; y < cell.y + cell.height; y++) for (let x = cell.x; x < cell.x + cell.width; x++) {
    if (image.pixels[(y * image.width + x) * 4 + 3] < 128) continue;
    left = Math.min(left, x); right = Math.max(right, x);
    top = Math.min(top, y); bottom = Math.max(bottom, y);
  }
  if (right < left || bottom < top) throw Error('Empty sprite cell');
  return { x: left, y: top, width: right - left + 1, height: bottom - top + 1 };
}
// Imagegen does not guarantee equally spaced cells. Use transparent gutters,
// and refuse to export when a cut would pass through a creature.
function gutterCuts(projection) {
  const cuts = [0], span = projection.length;
  for (let i = 1; i < 4; i++) {
    const expected = span * i / 4;
    const low = Math.max(cuts.at(-1) + 1, Math.floor(expected - span / 10));
    const high = Math.min(span - 1, Math.ceil(expected + span / 10));
    const gaps = [];
    for (let p = low; p <= high;) {
      if (projection[p]) { p++; continue; }
      const start = p;
      while (p <= high && !projection[p]) p++;
      if (p - start >= 2) gaps.push(Math.floor((start + p - 1) / 2));
    }
    if (!gaps.length) throw Error('No transparent gutter; atlas needs correction');
    gaps.sort((a, b) => Math.abs(a - expected) - Math.abs(b - expected));
    cuts.push(gaps[0]);
  }
  cuts.push(span);
  return cuts;
}
export function atlasCells(image) {
  const rows = new Uint32Array(image.height);
  for (let y = 0; y < image.height; y++) for (let x = 0; x < image.width; x++) {
    if (image.pixels[(y * image.width + x) * 4 + 3] >= 128) rows[y]++;
  }
  const rowCuts = gutterCuts(rows), cells = [];
  for (let row = 0; row < 4; row++) {
    const columns = new Uint32Array(image.width);
    for (let y = rowCuts[row]; y < rowCuts[row + 1]; y++) for (let x = 0; x < image.width; x++) {
      if (image.pixels[(y * image.width + x) * 4 + 3] >= 128) columns[x]++;
    }
    const columnCuts = gutterCuts(columns);
    for (let col = 0; col < 4; col++) cells.push({
      x: columnCuts[col], y: rowCuts[row],
      width: columnCuts[col + 1] - columnCuts[col], height: rowCuts[row + 1] - rowCuts[row]
    });
  }
  return cells;
}
export function exportView(image, box, size, pairExtent) {
  const margin = size === 32 ? 2 : 3, usable = size - margin * 2;
  const scale = usable / pairExtent;
  const width = Math.max(1, Math.round(box.width * scale)), height = Math.max(1, Math.round(box.height * scale));
  const pixels = new Uint8Array(size * size * 4);
  const ox = Math.floor((size - width) / 2), oy = size - margin - height;
  for (let y = 0; y < height; y++) for (let x = 0; x < width; x++) {
    const x0 = box.x + Math.floor(x * box.width / width), x1 = box.x + Math.ceil((x + 1) * box.width / width);
    const y0 = box.y + Math.floor(y * box.height / height), y1 = box.y + Math.ceil((y + 1) * box.height / height);
    let opaque = 0, white = 0;
    for (let sy = y0; sy < y1; sy++) for (let sx = x0; sx < x1; sx++) {
      const index = (sy * image.width + sx) * 4;
      if (image.pixels[index + 3] < 128) continue;
      opaque++;
      if ((77 * image.pixels[index] + 150 * image.pixels[index + 1] + 29 * image.pixels[index + 2]) >= 32768) white++;
    }
    if (!opaque || opaque * 4 < (x1 - x0) * (y1 - y0)) continue;
    const index = ((oy + y) * size + ox + x) * 4;
    pixels[index] = pixels[index + 1] = pixels[index + 2] = white * 2 >= opaque ? 255 : 0;
    pixels[index + 3] = 255;
  }
  return { width: size, height: size, pixels };
}
export function validateSprite(image, size) {
  if (image.width !== size || image.height !== size) throw Error('Sprite dimensions mismatch');
  let opaque = 0, transparent = 0, white = 0;
  for (let y = 0; y < size; y++) for (let x = 0; x < size; x++) {
    const i = (y * size + x) * 4, [r, g, b, a] = image.pixels.subarray(i, i + 4);
    if (![0, 255].includes(a) || r !== g || g !== b || ![0, 255].includes(r)) throw Error('Nonbinary sprite pixel');
    if (a) { opaque++; if (r === 255) white++; }
    else transparent++;
    if ((x === 0 || y === 0 || x === size - 1 || y === size - 1) && a) throw Error('Sprite touches tile edge');
  }
  if (!opaque || !transparent || !white) throw Error('Empty/unmasked sprite');
  return { opaque, transparent, white };
}
// Parse a constexpr value (address or geometry size) from src/fxdata.h.
export function fxdataConstant(header, name) {
  const match = header.match(new RegExp(`constexpr\\s+u?int(?:8|16|24|32)_t\\s+${name}\\s*=\\s*(0x[0-9A-Fa-f]+|\\d+)`));
  if (!match) throw Error(`src/fxdata.h does not declare ${name}`);
  return Number(match[1]);
}
// Compare a monochrome source sheet against the bytes cgfx-tools packs into the
// cart. Frames are row-major (columns = sheet width / frameWidth). Overview
// sheets store one pixel byte per column per 8-row page; sheets with
// transparent pixels interleave a pixel and a mask byte ("plus mask"). A pixel
// bit is set where the source red channel is non-zero, a mask bit where alpha
// is non-zero. The fxlayout image entries carry a 4-byte big-endian width and
// height header before the frames; the legacy Sprites.txt sets do not.
export function packedSheetDiff(sheet, packed, address,
    { frameWidth, frameHeight, hasHeader = false, plusMask = false }) {
  const columns = Math.floor(sheet.width / frameWidth);
  const pages = Math.ceil(frameHeight / 8);
  const frames = columns * Math.floor(sheet.height / frameHeight);
  const stride = frameWidth * pages * (plusMask ? 2 : 1);
  const base = address + (hasHeader ? 4 : 0), diffs = [];
  for (let frame = 0; frame < frames && diffs.length <= 4; frame++) {
    const originX = (frame % columns) * frameWidth, originY = Math.floor(frame / columns) * frameHeight;
    for (let page = 0; page < pages && diffs.length <= 4; page++) for (let x = 0; x < frameWidth && diffs.length <= 4; x++) {
      let pixel = 0, mask = 0;
      for (let bit = 0; bit < 8; bit++) {
        const y = page * 8 + bit;
        if (y >= frameHeight) continue;
        const i = ((originY + y) * sheet.width + originX + x) * 4;
        if (sheet.pixels[i] !== 0) pixel |= 1 << bit;
        if (plusMask && sheet.pixels[i + 3] !== 0) mask |= 1 << bit;
      }
      const at = base + frame * stride + (page * frameWidth + x) * (plusMask ? 2 : 1);
      if (packed[at] !== pixel || (plusMask && packed[at + 1] !== mask)) {
        diffs.push({ frame, page, x, pixel, mask, cartPixel: packed[at], cartMask: packed[at + 1] });
      }
    }
  }
  return { frames, stride, columns, diffs };
}
function paste(target, source, ox, oy, scale = 1) {
  for (let y = 0; y < source.height * scale; y++) for (let x = 0; x < source.width * scale; x++) {
    const from = (Math.floor(y / scale) * source.width + Math.floor(x / scale)) * 4;
    const to = ((oy + y) * target.width + ox + x) * 4;
    target.pixels.set(source.pixels.subarray(from, from + 4), to);
  }
}
export function exportAtlases(baseDirectory) {
  const base = path.resolve(baseDirectory), concepts = JSON.parse(fs.readFileSync(path.join(base, 'concepts.json')));
  const root = path.join(base, 'native'), manifest = { method: 'imagegen atlas cleanup; area coverage sampling; binary RGB/alpha', sizes: [32, 48], sprites: [] };
  const sheets = new Map([32, 48].map(size => [size, { width: size * 2, height: size * 32, pixels: new Uint8Array(size * 2 * size * 32 * 4) }]));
  for (const size of [32, 48]) fs.mkdirSync(path.join(root, `${size}x${size}`), { recursive: true });
  for (let group = 0; group < 4; group++) {
    const atlasName = `atlas-${group + 1}.png`, image = decodePng(fs.readFileSync(path.join(root, atlasName)));
    if (image.width !== image.height) throw Error('Expected square4x4 atlas');
    if (!image.pixels.some((v, i) => i % 4 === 3 && v < 128)) throw Error('Atlas has no transparency');
    const cells = atlasCells(image);
    const contacts = new Map([32, 48].map(size => [size, { width: size * 4, height: size * 4, pixels: new Uint8Array(size * size * 16 * 4) }]));
    for (let species = 0; species < 8; species++) {
      const concept = concepts[group * 8 + species];
      const boxes = [0, 1].map(view => {
        const slot = species * 2 + view;
        const cell = cells[slot];
        const box = bounds(image, cell);
        if (box.x === cell.x || box.y === cell.y || box.x + box.width === cell.x + cell.width || box.y + box.height === cell.y + cell.height) throw Error(`Atlas sprite touches cell edge: ${concept.name}/${view}`);
        return box;
      });
      const pairExtent = Math.max(...boxes.flatMap(box => [box.width, box.height]));
      for (const size of [32, 48]) for (let view = 0; view < 2; view++) {
        const sprite = exportView(image, boxes[view], size, pairExtent), stats = validateSprite(sprite, size);
        const pose = view === 0 ? 'front' : 'back';
        const name = `${concept.conceptId}-${concept.name.toLowerCase()}-${pose}_${size}x${size}.png`;
        const relative = `${size}x${size}/${name}`;
        fs.writeFileSync(path.join(root, relative), encodePng(sprite));
        const slot = species * 2 + view;
        paste(contacts.get(size), sprite, slot % 4 * size, Math.floor(slot / 4) * size);
        paste(sheets.get(size), sprite, view * size, (group * 8 + species) * size);
        manifest.sprites.push({ conceptId: concept.conceptId, name: concept.name, view: pose, size, file: relative, sourceAtlas: atlasName, crop: boxes[view], pairExtent, ...stats });
      }
    }
    for (const size of [32, 48]) {
      const contact = contacts.get(size), scale = size === 32 ? 6 : 4;
      fs.writeFileSync(path.join(root, `contact-${group + 1}-${size}.png`), encodePng(contact));
      const enlarged = { width: contact.width * scale, height: contact.height * scale, pixels: new Uint8Array(contact.width * contact.height * scale * scale * 4) };
      paste(enlarged, contact, 0, 0, scale);
      fs.writeFileSync(path.join(root, `contact-${group + 1}-${size}-preview.png`), encodePng(enlarged));
    }
  }
  for (const size of [32, 48]) fs.writeFileSync(path.join(root, `expansion_${size}x${size}.png`), encodePng(sheets.get(size)));
  fs.writeFileSync(path.join(root, 'manifest.json'), JSON.stringify(manifest, null, 2) + '\n');
  console.log(`PASS: ${manifest.sprites.length} binary transparent sprites,32 concepts x2views x2sizes; paired sheets and contacts exported.`);
  return manifest;
}
if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  exportAtlases(process.argv[2] || 'docs/assets/creature-expansion');
}
