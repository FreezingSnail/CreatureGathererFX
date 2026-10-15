// Authored duel UI icon sheet for CreatureGathererFX-d73i.2.
//
// Source of truth: mock/icons.js (the five folded-type 8x8 icons in
// mock/matrices.js id "5" order) plus the mock/battle-mock.js SYMBOLS up/down
// bonus markers. The five types are emitted first so the frame index equals the
// duel::DType value (Spirit, Water, Earth, Fire, Plant); frames 5 and 6 are the
// up (bonus) and down (weak) markers. The sheet is one vertical strip because
// cgfx-tools packs vertically stacked frames with spacing = 0.
//
// Run: node tools/export-duel-icons.mjs   (writes images/DuelIcons_8x8.png)

import { deflateSync } from 'node:zlib';
import { writeFileSync } from 'node:fs';

const ICONS = {
  spirit: [
    '..####..', '.#....#.', '#.#..#.#', '#......#',
    '#......#', '#.#..#.#', '#.#..#.#', '.#.##.#.',
  ],
  water: [
    '...#....', '..###...', '.#####..', '.#####..',
    '#######.', '#######.', '.#####..', '..###...',
  ],
  earth: [
    '........', '...#....', '..###...', '.#####..',
    '#######.', '#######.', '........', '#######.',
  ],
  fire: [
    '...#....', '..##....', '..###...', '.#####..',
    '.#####..', '.#####..', '..###...', '...#....',
  ],
  plant: [
    '....##..', '...###..', '..####..', '.#####..',
    '#####...', '####....', '###.....', '##......',
  ],
};

// 5-wide markers centered in the 8-wide frame (offset 1).
const UP = ['..#..', '.###.', '#####', '.....', '.....', '.....', '.....', '.....'];
const DOWN = ['.....', '.....', '.....', '#####', '.###.', '..#..', '.....', '.....'];

// Two inks per frame, selected second: Blit::draw with PLUSMASK lays the pixel
// plane down where the alpha mask is set, so a white-ink frame draws white and a
// black-ink frame clears to black. Frame order:
//   0..4  white spirit,water,earth,fire,plant   (cells, badges, centre)
//   5,6   white up / down bonus markers
//   7..11 black spirit,water,earth,fire,plant   (selected white cell)
//   12,13 black up / down markers
const TYPES = ['spirit', 'water', 'earth', 'fire', 'plant'];
const FRAMES = TYPES.map((name) => ({ rows: ICONS[name], ink: 255 }));
FRAMES.push({ rows: UP, ink: 255 }, { rows: DOWN, ink: 255 });
FRAMES.push(...TYPES.map((name) => ({ rows: ICONS[name], ink: 0 })));
FRAMES.push({ rows: UP, ink: 0 }, { rows: DOWN, ink: 0 });

const WIDTH = 8;
const HEIGHT = 8 * FRAMES.length;

// RGBA with an alpha mask: ink is opaque white, background fully transparent.
// cgfx-tools emits a two-plane (pixels + mask) field for a masked image, which
// Blit::draw reads with PLUSMASK (white ink) or NEGATIVE (black ink), matching
// the mock's two-tone `color` argument.
const raw = Buffer.alloc((WIDTH * 4 + 1) * HEIGHT, 0);
for (let frame = 0; frame < FRAMES.length; ++frame) {
  const { rows, ink } = FRAMES[frame];
  for (let y = 0; y < 8; ++y) {
    for (let x = 0; x < WIDTH; ++x) {
      const row = rows[y] || '';
      const offset = frame * 8 + y;
      const at = offset * (WIDTH * 4 + 1) + 1 + x * 4;
      if (row[x] === '#') {
        // Ink is opaque; the pixel plane carries the ink polarity.
        raw[at] = ink;
        raw[at + 1] = ink;
        raw[at + 2] = ink;
        raw[at + 3] = 255;
      }
    }
  }
}

function chunk(type, data) {
  const length = Buffer.alloc(4);
  length.writeUInt32BE(data.length, 0);
  const body = Buffer.concat([Buffer.from(type, 'ascii'), data]);
  const crc = Buffer.alloc(4);
  crc.writeUInt32BE(crc32(body) >>> 0, 0);
  return Buffer.concat([length, body, crc]);
}

const CRC_TABLE = (() => {
  const table = new Uint32Array(256);
  for (let n = 0; n < 256; ++n) {
    let c = n;
    for (let k = 0; k < 8; ++k) c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
    table[n] = c >>> 0;
  }
  return table;
})();

function crc32(buf) {
  let c = 0xffffffff;
  for (let i = 0; i < buf.length; ++i) c = CRC_TABLE[(c ^ buf[i]) & 0xff] ^ (c >>> 8);
  return (c ^ 0xffffffff) >>> 0;
}

const ihdr = Buffer.alloc(13);
ihdr.writeUInt32BE(WIDTH, 0);
ihdr.writeUInt32BE(HEIGHT, 4);
ihdr[8] = 8; // bit depth
ihdr[9] = 6; // truecolour + alpha (masked field)
ihdr[10] = 0;
ihdr[11] = 0;
ihdr[12] = 0;

const png = Buffer.concat([
  Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]),
  chunk('IHDR', ihdr),
  chunk('IDAT', deflateSync(raw, { level: 9 })),
  chunk('IEND', Buffer.alloc(0)),
]);

writeFileSync(new URL('../images/DuelIcons_8x8.png', import.meta.url), png);
process.stdout.write(`wrote images/DuelIcons_8x8.png ${WIDTH}x${HEIGHT} (${FRAMES.length} frames)\n`);
