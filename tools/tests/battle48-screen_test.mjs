// Frozen first-beta label/width/position table for the native 128x64 UI.
//
// Run: node --test tools/tests/battle48-screen_test.mjs   (after `make gen`)
//
// The table below is the accepted contract; source constants and authored
// strings.txt lengths are read back so a drifted width/label fails here rather
// than clipping on the device. The framebuffer checks blit the real packed text
// and glyph bitmaps from dist/fxdata-data.bin at the pinned positions and prove
// the selected/unselected polarities, empty states and terminal messages with
// no screenshots or device run.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const read = (rel) => fs.readFileSync(path.join(repo, rel), 'utf8');

const WIDTH = 128;
const HEIGHT = 64;
const GLYPH = 5; // packed text/glyph cell width
const textWidth = (string) => (string.length + 1) * GLYPH; // retained leading blank

// --- authored strings ------------------------------------------------------
const strings = (() => {
  const map = new Map();
  let namespace = '';
  for (const line of read('data/text/strings.txt').split('\n')) {
    const ns = line.match(/^\s*namespace\s+(\w+)/);
    if (ns) { namespace = ns[1]; continue; }
    const entry = line.match(/^\s*(\w+)\s*=\s*"([^"]*)"/);
    if (entry) map.set(`${namespace}.${entry[1]}`, entry[2]);
  }
  return map;
})();
const authored = (key) => {
  assert.ok(strings.has(key), `missing authored string ${key}`);
  return strings.get(key);
};
const symbolOf = (key) => key.split('.').pop();
const entryOf = (symbol) => {
  const key = [...strings.keys()].find((candidate) => symbolOf(candidate) === symbol);
  assert.ok(key, `authored string for ${symbol}`);
  return [key, strings.get(key)];
};

// --- generated symbols and packed pixel data -------------------------------
const symbols = (() => {
  const map = new Map();
  let depth = 0;
  for (const line of read('src/fxdata.h').split('\n')) {
    const entry = line.match(/constexpr uint24_t (\w+) = (0x[0-9A-Fa-f]+);/);
    if (entry && depth === 0) map.set(entry[1], Number.parseInt(entry[2], 16));
    for (const ch of line) {
      if (ch === '{') depth++;
      else if (ch === '}') depth--;
    }
  }
  return map;
})();
const addressOf = (key) => {
  const symbol = symbolOf(key);
  assert.ok(symbols.has(symbol), `missing generated symbol ${symbol}; run make gen`);
  return symbols.get(symbol);
};
const fx = (() => {
  const bin = path.join(repo, 'dist', 'fxdata-data.bin');
  assert.ok(fs.existsSync(bin), 'dist/fxdata-data.bin missing; run make gen');
  return fs.readFileSync(bin);
})();

// --- framebuffer primitives mirroring src/lib/Blit.cpp ---------------------
const frame0 = () => new Uint8Array(WIDTH * HEIGHT);

function blit(frame, x, y, w, h, address, frameIndex = 0, plusMask = false) {
  const pages = (h + 7) >> 3;
  const bpp = plusMask ? 2 : 1;
  const stride = w * pages * bpp;
  const base = address + frameIndex * stride;
  assert.ok(Number.isInteger(address) && address >= 0 && address < fx.length,
    `sprite address 0x${address.toString(16)} outside the packed image`);
  assert.ok(base + stride <= fx.length, 'sprite frame runs past the packed image');
  assert.ok(x >= 0 && y >= 0 && x + w <= WIDTH && y + h <= HEIGHT,
    `${w}x${h} label at ${x},${y} leaves the 128x64 screen`);
  const lastMask = (h & 7) ? (1 << (h & 7)) - 1 : 0xFF;
  for (let c = 0; c < w; c++) {
    for (let page = 0; page < pages; page++) {
      const mask = page === pages - 1 ? lastMask : 0xFF;
      const offset = base + (page * w + c) * bpp;
      const bits = fx[offset] & mask;
      const masks = plusMask ? fx[offset + 1] & mask : mask;
      for (let row = 0; row < 8; row++) {
        const py = y + page * 8 + row;
        if (py >= y + h) continue;
        if (((masks >> row) & 1) === 0) continue;
        frame[py * WIDTH + x + c] = (bits >> row) & 1;
      }
    }
  }
}

// A full-width packed text raster (drawText): pinned width must equal the
// authored bitmap so a short constant can never clip the glyph run.
function drawText(frame, key, x, y, frameIndex = 0) {
  const width = textWidth(authored(key));
  blit(frame, x, y, width, 8, addressOf(key), frameIndex);
  return width;
}

// drawDuelLabel skips the retained leading blank cell; `visible` is the glyph
// run without it, and dark selects the inverted frame.
function drawDuelLabel(frame, key, x, y, visible, dark = false) {
  const address = addressOf(key) + (dark ? visible + 10 : GLYPH);
  blit(frame, x, y, visible, 8, address);
  return visible;
}

function drawGlyph(frame, x, y, character) {
  if (character === ' ') return;
  assert.ok(character >= '0' && character <= 'z', `glyph ${character} out of range`);
  const base = symbols.get('fontTrimmed') + 4;
  blit(frame, x, y, GLYPH, 6, base, character.charCodeAt(0) - '0'.charCodeAt(0));
}

function drawLabel(frame, x, y, text) {
  for (const character of text) {
    drawGlyph(frame, x, y, character);
    x += GLYPH;
  }
}

function fillRect(frame, x, y, w, h, value) {
  assert.ok(x >= 0 && y >= 0 && x + w <= WIDTH && y + h <= HEIGHT,
    `rect ${w}x${h} at ${x},${y} leaves the 128x64 screen`);
  for (let row = 0; row < h; row++)
    for (let column = 0; column < w; column++)
      frame[(y + row) * WIDTH + x + column] = value;
}

const lit = (frame, x, y, w, h) => {
  let count = 0;
  for (let row = 0; row < h; row++)
    for (let column = 0; column < w; column++)
      count += frame[(y + row) * WIDTH + x + column];
  return count;
};
const litBounds = (frame) => {
  let x0 = WIDTH, y0 = HEIGHT, x1 = -1, y1 = -1, count = 0;
  for (let y = 0; y < HEIGHT; y++) {
    for (let x = 0; x < WIDTH; x++) {
      if (!frame[y * WIDTH + x]) continue;
      count++;
      x0 = Math.min(x0, x); y0 = Math.min(y0, y);
      x1 = Math.max(x1, x); y1 = Math.max(y1, y);
    }
  }
  return { x0, y0, x1, y1, count };
};
const longestCreature = () => {
  const entries = [...strings.entries()].filter(([key]) => key.startsWith('CreatureNames.'));
  entries.sort((a, b) => b[1].length - a[1].length);
  return entries[0];
};
const assertRectEquals = (actual, expected, x, y, w, h, label) => {
  for (let row = 0; row < h; row++) {
    for (let column = 0; column < w; column++) {
      const pixel = (y + row) * WIDTH + x + column;
      assert.equal(actual[pixel], expected[pixel],
        `${label}: pixel ${x + column},${y + row} differs`);
    }
  }
};

// --- pinned table ----------------------------------------------------------
// Every first-beta label: authored key, x, y, region right edge (inclusive).
const LABELS = [
  // Battle action page (3-row wild strip x0..79; 1-row trainer strip x0..127).
  { key: 'MenuStrings.gather', x: 4, y: 40, right: 80 },
  { key: 'BattleActionText.items', x: 4, y: 48, right: 80 },
  { key: 'MenuStrings.escape', x: 4, y: 56, right: 80 },
  // Battle feedback rows (drawLine x2, rows y48/y56, 124 px usable).
  { key: 'BattleFeedbackStrings.battleWin', x: 2, y: 48, right: 126 },
  { key: 'BattleFeedbackStrings.battleDefeat', x: 2, y: 48, right: 126 },
  { key: 'BattleFeedbackStrings.battleEscaped', x: 2, y: 48, right: 126 },
  { key: 'BattleFeedbackStrings.battleGathered', x: 2, y: 48, right: 126 },
  { key: 'BattleFeedbackStrings.battleFled', x: 2, y: 48, right: 126 },
  { key: 'BattleFeedbackStrings.battleEnters', x: 2, y: 56, right: 126 },
  { key: 'BattleFeedbackStrings.battleCannotEscape', x: 2, y: 56, right: 126 },
  { key: 'BattleFeedbackStrings.battleCannotAct', x: 2, y: 56, right: 126 },
  { key: 'BattleFeedbackStrings.battleHurtItself', x: 2, y: 56, right: 126 },
  { key: 'BattleFeedbackStrings.battlePass', x: 2, y: 56, right: 126 },
  // Bag rows (name 0 at x17, name 1 at x66) and the world lure picker.
  { key: 'LureTierNames.lureTier0', x: 17, y: 8, right: 66 },
  { key: 'LureTypeNames.lureType5', x: 66, y: 8, right: 128 },
  { key: 'ConsumableNames.consumable6', x: 17, y: 47, right: 128 },
  { key: 'LureText.lureTextPrompt', x: 0, y: 0, right: 128 },
  { key: 'LureText.lureTextEmpty', x: 0, y: 24, right: 128 },
  { key: 'LureText.lureTextFooter', x: 0, y: 56, right: 128 },
  { key: 'LureText.lureTextExpired', x: 2, y: 56, right: 128 },
  { key: 'LureText.lureTextBetaComplete', x: 2, y: 56, right: 128 },
];

test('pinned label widths match authored text and source constants', () => {
  for (const label of LABELS) {
    const width = textWidth(authored(label.key));
    assert.ok(label.x + width <= label.right,
      `${label.key} (${width}px) at x${label.x} overruns its ${label.right}px region`);
  }
  assert.equal(symbols.has('items'), true, 'generated items symbol exists after make gen');

  // BattleMenu: action-row constants are authored widths, and the bag row uses
  // its own plural label rather than a creature-name sentinel.
  const menu = read('src/engine/battle/BattleMenu.cpp');
  assert.match(menu, /constexpr uint8_t gatherWidth = \(6 \+ 1\) \* 5;/);
  assert.match(menu, /constexpr uint8_t escapeWidth = \(6 \+ 1\) \* 5;/);
  assert.match(menu, /constexpr uint8_t itemWidth = \(5 \+ 1\) \* 5;/);
  assert.match(menu, /constexpr uint24_t itemLabel = items;/);
  assert.match(menu, /const uint8_t width = rows == 3 \? 80 : 128;/);
  assert.match(menu, /const uint8_t firstY = rows == 3 \? 40 : 48;/);
  assert.match(menu, /Blit::fillRect\(0, y, width, 8, WHITE\);/);
  assert.match(menu, /drawText\(4, y, address, labelWidth, selected \? 1 : 0\);/);
  assert.equal(textWidth(authored('BattleActionText.items')), 30);

  // LureZoneView and InventoryMenu decode the same lure/consumable charset.
  const lureView = read('src/engine/world/LureZoneView.cpp');
  const lureTiers = lureView.match(/static const uint8_t tierChars\[\] = \{([^}]*)\};/);
  const lureTypes = lureView.match(/static const uint8_t typeChars\[\] = \{([^}]*)\};/);
  assert.ok(lureTiers && lureTypes, 'lure picker charset tables present');
  assert.deepEqual(lureTiers[1].split(',').map(Number),
    [0, 1, 2].map((tier) => authored(`LureTierNames.lureTier${tier}`).length));
  assert.deepEqual(lureTypes[1].split(',').map(Number),
    [0, 1, 2, 3, 4, 5, 6, 7].map((type) => authored(`LureTypeNames.lureType${type}`).length));

  const inventory = read('src/engine/menu/InventoryMenu.cpp');
  const bagTypes = inventory.match(/const uint8_t typeChars\[\] PROGMEM = \{([^}]*)\};/);
  const consumables = inventory.match(/const uint8_t consumableChars\[\] PROGMEM = \{([^}]*)\};/);
  assert.ok(bagTypes && consumables, 'bag charset tables present');
  assert.deepEqual(bagTypes[1].split(',').map(Number), lureTypes[1].split(',').map(Number));
  assert.deepEqual(consumables[1].split(',').map(Number),
    [0, 1, 2, 3, 4, 5, 6, 7].map((id) => authored(`ConsumableNames.consumable${id}`).length));
  assert.match(inventory, /constexpr uint8_t kRowY\[kVisibleRows\] = \{8, 21, 34, 47\};/);
  assert.match(inventory, /drawText\(17, kRowY\[row\], m\.rowName\[row\]\[0\]/);
  assert.match(inventory, /drawText\(66, kRowY\[row\], m\.rowName\[row\]\[1\]/);
  // Selected row = white name cell (x12..127) carrying the inverted frame.
  assert.match(inventory, /fillRect\(12, kRowY\[row\], 116, 8, WHITE\)/);
  assert.match(inventory, /const uint8_t frame = selected \? 1 : 0;/);

  // Collection view: rows, 80-px name cell, level/marker gutter, feedback line.
  const collection = read('src/engine/menu/CollectionMenu.cpp');
  assert.match(collection, /constexpr uint8_t kRowY\[kVisibleRows\] = \{12, 24, 36, 48\};/);
  assert.match(collection, /constexpr uint8_t kNameMaxWidth = 80;/);
  assert.match(collection, /fillRect\(8, y, kNameMaxWidth, 8, WHITE\)/);
  assert.match(collection, /FRAME\(selected \? 1 : 0\)/);
  assert.match(collection, /PpGlyph::number\(92, static_cast<uint8_t>\(y \+ 1\), m\.rowLevel\[row\]\);/);
  assert.match(collection, /drawGlyph\(122, y, 'P', Blit::OVERWRITE\);/);
  const pstr = new Map([...collection.matchAll(/const char (\w+)\[\] PROGMEM = "([^"]*)";/g)]
    .map((m) => [m[1], m[2]]));
  assert.equal(pstr.get('collectionLabel'), 'COLLECTION');
  assert.equal(pstr.get('alreadyInPartyLabel'), 'ALREADY IN PARTY');
  assert.equal(pstr.get('saveNeededLabel'), 'SAVE NEEDED');
  for (const name of ['collectionLabel', 'alreadyInPartyLabel', 'saveNeededLabel']) {
    assert.ok(pstr.get(name).length * GLYPH <= WIDTH, `${name} fits 128px`);
  }
  assert.match(collection, /drawLabel\(0, 56, alreadyInPartyLabel\)/);

  // Feedback captions: every hard-coded character count equals the authored
  // string, so a rename cannot silently strip the tail of a message.
  const feedback = read('src/engine/battle/BattleFeedback.cpp');
  assert.match(feedback, /return static_cast<uint8_t>\(\(characters \+ 1u\) \* 5u\);/);
  const captions = [...feedback.matchAll(
    /cacheCaption\(lines_\[\d\], widths_\[\d\], (\w+), (\d+)\);/g)];
  assert.ok(captions.length >= 11, 'feedback caption table present');
  for (const [, symbol, characters] of captions) {
    const [key, value] = entryOf(symbol);
    assert.equal(Number(characters), value.length, `${key} caption length`);
  }
  assert.match(feedback, /Blit::fillRect\(0, 40, 128, 24, BLACK\);/);
  assert.match(feedback, /Blit::draw\(2, y, width, 8, address, 0, Blit::OVERWRITE\);/);
  assert.match(feedback, /drawLine\(lines_\[0\], widths_\[0\], 48\);/);
  assert.match(feedback, /drawLine\(lines_\[1\], widths_\[1\], 56\);/);

  // Ending + lure message widths are authored widths, and the longest label
  // ends exactly on the last screen column.
  assert.match(read('src/engine/world/World.cpp'),
    /LureZone::showMessage\(lureTextBetaComplete, 125, 60\);/);
  assert.match(read('src/engine/world/World.cpp'),
    /LureZone::showMessage\(lureTextGathered, 45, 40\);/);
  assert.match(read('src/engine/world/World.cpp'),
    /LureZone::showMessage\(lureTextFull, 45, 40\);/);
  assert.equal(textWidth(authored('LureText.lureTextBetaComplete')), 125);
  assert.match(read('src/engine/world/LureZone.cpp'),
    /showMessage\(lureTextExpired, 50, 40\);/);
  assert.equal(textWidth(authored('LureText.lureTextExpired')), 50);

  // Lure picker/intro literals: every hard-coded width is its authored width.
  const pickerPins = [
    /drawText\(0, 0, LureZone::pickerFeedback\(\) \? lureTextSet : lureTextPrompt, 45, 0\);/,
    /drawText\(0, 24, lureTextEmpty, 45, 0\);/,
    /drawText\(0, 56, lureTextFooter, 65, 0\);/,
    /drawText\(4, 4, lureTextIntroTitle, 50, 0\);/,
    /drawText\(4, 14, lureTextIntroSet, 95, 0\);/,
    /drawText\(4, 24, lureTextIntroBias, 90, 0\);/,
    /drawText\(4, 34, lureTextIntroSteps, 60, 0\);/,
    /drawText\(4, 44, lureTextIntroGather, 95, 0\);/,
    /drawText\(4, 54, lureTextIntroNoSave, 65, 0\);/,
  ];
  for (const pin of pickerPins) assert.match(lureView, pin);
  assert.equal(textWidth(authored('LureText.lureTextSet')), 45);
  assert.equal(textWidth(authored('LureText.lureTextPrompt')), 45);
  assert.equal(textWidth(authored('LureText.lureTextEmpty')), 45);
  assert.equal(textWidth(authored('LureText.lureTextFooter')), 65);
  assert.equal(textWidth(authored('LureText.lureTextIntroTitle')), 50);
  assert.equal(textWidth(authored('LureText.lureTextIntroSet')), 95);
  assert.equal(textWidth(authored('LureText.lureTextIntroBias')), 90);
  assert.equal(textWidth(authored('LureText.lureTextIntroSteps')), 60);
  assert.equal(textWidth(authored('LureText.lureTextIntroGather')), 95);
  assert.equal(textWidth(authored('LureText.lureTextIntroNoSave')), 65);

  // Duel captions and centre/pick labels.
  const scene = read('src/engine/battle/BattleScene.cpp');
  const duelCaptions = [...scene.matchAll(
    /case duel::Feedback::\w+: address = (\w+); width = (\d+); break;/g)];
  assert.ok(duelCaptions.length >= 17, 'duel caption table present');
  for (const [, symbol, width] of duelCaptions) {
    if (symbol === 'gather') {
      assert.equal(Number(width), textWidth(authored('MenuStrings.gather')));
      continue;
    }
    const [key, value] = entryOf(symbol);
    assert.equal(Number(width), textWidth(value), `${key} caption width`);
    assert.ok(42 + Number(width) <= WIDTH, `${key} caption fits beside the sprites`);
  }
  assert.match(scene, /drawText\(42, 48, caption, width, 0\);/);
  assert.match(scene, /drawDuelLabel\(labelX, 53, duelSwap, 20, selected\);/);
  assert.match(scene, /drawDuelLabel\(48, 41, gather, 30, false\);/);
  assert.match(scene, /drawDuelLabel\(18, 51, duelVs, 10, false\);/);
  assert.match(scene, /drawDuelLabel\(48, 41, duelRound, 5, false\);/);
  assert.equal(authored('DuelText.duelSwap').length * GLYPH, 20, 'SWAP label is 20 px');
  assert.equal(authored('DuelText.duelWait').length * GLYPH, 20, 'WAIT label is 20 px');
});

test('battle action page selected and unselected rows', () => {
  // Three-row gatherable wild: GATHER / ITEMS / ESCAPE in the 80-px strip.
  const rows3 = ['MenuStrings.gather', 'BattleActionText.items', 'MenuStrings.escape'];
  for (let cursor = 0; cursor < rows3.length; cursor++) {
    const frame = frame0();
    fillRect(frame, 0, 40, 80, 24, 0);
    for (let row = 0; row < rows3.length; row++) {
      const y = 40 + row * 8;
      const selected = row === cursor;
      if (selected) fillRect(frame, 0, y, 80, 8, 1);
      drawText(frame, rows3[row], 4, y, selected ? 1 : 0);
      assert.ok(4 + textWidth(authored(rows3[row])) <= 80,
        `${rows3[row]} fits the 80-px strip`);
      // Selected: exactly the inverted frame inside a white cell. Unselected:
      // exactly the normal frame on the black strip.
      const expected = frame0();
      if (selected) fillRect(expected, 0, y, 80, 8, 1);
      drawText(expected, rows3[row], 4, y, selected ? 1 : 0);
      assertRectEquals(frame, expected, 0, y, 80, 8, rows3[row]);
      assert.ok(lit(frame, 4, y, 80 - 4, 8) > 0, `${rows3[row]} paints its row`);
    }
    assert.equal(lit(frame, 80, 40, 48, 24), 0, 'three-row strip keeps x80..127 clear');
    assert.ok(lit(frame, 0, 40, 80, 8) > 0, 'GATHER row paints');
    assert.ok(lit(frame, 0, 56, 80, 8) > 0, 'ESCAPE row paints');
  }

  // One-row trainer page: ITEMS only, full 128-px width at y48.
  const frame = frame0();
  fillRect(frame, 0, 48, 128, 8, 0);
  fillRect(frame, 0, 48, 128, 8, 1);
  drawText(frame, 'BattleActionText.items', 4, 48, 1);
  const expected = frame0();
  fillRect(expected, 0, 48, 128, 8, 1);
  drawText(expected, 'BattleActionText.items', 4, 48, 1);
  assertRectEquals(frame, expected, 0, 48, 128, 8, 'trainer ITEMS row');
  assert.equal(lit(frame, 0, 56, 128, 8), 0, 'no ESCAPE row without escape');
  assert.equal(lit(frame, 0, 40, 128, 8), 0, 'no GATHER row without gather');
});

test('battle feedback rows and terminal messages', () => {
  // Terminal outcomes own row 0; action captions own row 1. No row may cross.
  const terminal = ['battleWin', 'battleDefeat', 'battleEscaped', 'battleGathered', 'battleFled'];
  for (const symbol of terminal) {
    const key = `BattleFeedbackStrings.${symbol}`;
    const frame = frame0();
    drawText(frame, key, 2, 48);
    const bounds = litBounds(frame);
    assert.ok(bounds.count > 0, `${symbol} paints its message`);
    assert.ok(bounds.y0 >= 48, `${symbol} stays on row 0`);
    assert.ok(bounds.y1 <= 55, `${symbol} never enters row 1`);
    assert.ok(bounds.x1 <= 125, `${symbol} stays inside the 124-px row`);
  }

  // Longest second-row caption plus the longest creature name on row 0.
  const [longestKey, longestName] = longestCreature();
  const frame = frame0();
  drawText(frame, longestKey, 2, 48);
  drawText(frame, 'BattleFeedbackStrings.battleEnters', 2, 56);
  const nameEnd = 2 + textWidth(longestName);
  assert.ok(lit(frame, 2, 48, nameEnd - 2, 8) > 0, 'longest actor name paints');
  assert.equal(lit(frame, nameEnd, 48, 128 - nameEnd, 8), 0,
    'longest actor name ends on its last glyph column');
  assert.ok(2 + textWidth(authored('BattleFeedbackStrings.battleEnters')) <= 126,
    'enters-the-field caption fits');
  assert.ok(lit(frame, 2, 48, 124, 8) > 0, 'actor row paints');
  assert.ok(lit(frame, 2, 56, 124, 8) > 0, 'caption row paints');
  assert.equal(lit(frame, 0, 56, 2, 8), 0, 'caption starts at the pinned x2');
});

test('bag rows, longest names and empty bag', () => {
  const rowY = [8, 21, 34, 47];
  const rows = [
    { first: 'LureTierNames.lureTier0', second: 'LureTypeNames.lureType5' }, // crude lightning
    { first: 'LureTierNames.lureTier2', second: 'LureTypeNames.lureType3' }, // pure earth
    { first: 'LureTierNames.lureTier1', second: 'LureTypeNames.lureType5' }, // fine lightning
    { first: 'ConsumableNames.consumable6' }, // essence
  ];
  for (let cursor = 0; cursor < rows.length; cursor++) {
    const frame = frame0();
    for (let row = 0; row < rows.length; row++) {
      const y = rowY[row];
      const selected = row === cursor;
      // Count glyph stays in the black left gutter, outside the white cell.
      drawGlyph(frame, 1, y + 1, '3');
      if (selected) fillRect(frame, 12, y, 116, 8, 1);
      drawText(frame, rows[row].first, 17, y, selected ? 1 : 0);
      if (rows[row].second) drawText(frame, rows[row].second, 66, y, selected ? 1 : 0);
      assert.ok(17 + textWidth(authored(rows[row].first)) <= 66,
        'tier name ends before the type column');
      assert.ok(66 + textWidth(authored(rows[row].second ?? rows[row].first)) <= 128,
        'bag row fits 128px');
      assert.equal(lit(frame, 9, y, 3, 8), 0, 'count stays left of the name cell');
      if (selected) {
        // Every pixel of the cell is white except the inverted glyph runs.
        for (let column = 12; column < 128; column++) {
          const inName0 = column >= 17 && column < 17 + textWidth(authored(rows[row].first));
          const inName1 = Boolean(rows[row].second) && column >= 66 &&
            column < 66 + textWidth(authored(rows[row].second));
          if (inName0 || inName1) continue;
          for (let line = 0; line < 8; line++)
            assert.equal(frame[(y + line) * 128 + column], 1,
              `selected bag cell ${column},${y + line} is white`);
        }
      } else {
        // Unselected rows stay on the black page: no white bar behind the
        // glyphs (an inverted frame here would paint the whole row white).
        for (let column = 12; column < 128; column++) {
          const inName0 = column >= 17 && column < 17 + textWidth(authored(rows[row].first));
          const inName1 = Boolean(rows[row].second) && column >= 66 &&
            column < 66 + textWidth(authored(rows[row].second));
          if (inName0 || inName1) continue;
          for (let line = 0; line < 8; line++)
            assert.equal(frame[(y + line) * 128 + column], 0,
              `unselected bag cell ${column},${y + line} stays black`);
        }
      }
    }
  }

  // Empty bag: the page is a real empty state with no rows and no stray pixels.
  const empty = frame0();
  assert.equal(litBounds(empty).count, 0, 'empty bag paints no rows');
});

test('lure picker, deposit messages and beta ending', () => {
  // Five-row picker: prompt, per-row tier/type, footer on the last row.
  const frame = frame0();
  drawText(frame, 'LureText.lureTextPrompt', 0, 0);
  const rows = [
    ['LureTierNames.lureTier0', 'LureTypeNames.lureType3'], // crude earth: regression
    ['LureTierNames.lureTier1', 'LureTypeNames.lureType5'],
    ['LureTierNames.lureTier2', 'LureTypeNames.lureType1'],
    ['LureTierNames.lureTier0', 'LureTypeNames.lureType5'],
    ['LureTierNames.lureTier2', 'LureTypeNames.lureType7'],
  ];
  for (let row = 0; row < rows.length; row++) {
    const y = 12 + row * 9;
    drawText(frame, rows[row][0], 17, y, row === 0 ? 0 : 1);
    drawText(frame, rows[row][1], 66, y, row === 0 ? 0 : 1);
    assert.ok(17 + textWidth(authored(rows[row][0])) <= 66, 'tier column ends before type');
    assert.ok(66 + textWidth(authored(rows[row][1])) <= 128, 'type column fits');
  }
  drawText(frame, 'LureText.lureTextFooter', 0, 56);
  assert.ok(lit(frame, 0, 0, 128, 8) > 0, 'picker prompt paints');
  assert.ok(lit(frame, 0, 56, 128, 8) > 0, 'footer paints on the last row');
  assert.ok(lit(frame, 14, 12, 128 - 14, 6) > 0, 'crude earth row paints');
  assert.ok(lit(frame, 66, 12, 30, 6) > 0, 'earth keeps all five glyphs (no clip)');

  // Empty picker: "No Lures" replaces the rows.
  const empty = frame0();
  drawText(empty, 'LureText.lureTextEmpty', 0, 24);
  assert.ok(lit(empty, 0, 24, 45, 8) > 0, 'No Lures message paints');
  assert.equal(lit(empty, 0, 12, 128, 8), 0, 'empty picker draws no row');

  // Terminal ending message: 125 px at x2 ends on column 127.
  const ending = frame0();
  drawText(ending, 'LureText.lureTextBetaComplete', 2, 56);
  const endingBounds = litBounds(ending);
  assert.ok(endingBounds.x1 <= 126, 'ardu yields the clearing stays inside the screen');
  assert.ok(endingBounds.y0 >= 56, 'ending message owns the message row');
  assert.ok(endingBounds.y1 <= 63, 'ending message stays on the last row');
});

test('collection root, list, party and feedback labels', () => {
  // Root: MENU header, BAG/COLLECTION/SAVE at x12, y12/28/44, marker at x2.
  const frame = frame0();
  drawLabel(frame, 0, 0, 'MENU');
  const rootRows = [
    { y: 12, label: 'BAG' },
    { y: 28, label: 'COLLECTION' },
    { y: 44, label: 'SAVE' },
  ];
  rootRows.forEach((row, index) => {
    if (index === 1) drawGlyph(frame, 2, row.y, '>');
    drawLabel(frame, 12, row.y, row.label);
    assert.ok(12 + row.label.length * GLYPH <= 128, `${row.label} fits the root row`);
    assert.ok(row.y + 6 <= (rootRows[index + 1]?.y ?? 64), `${row.label} row pitch`);
  });
  assert.ok(lit(frame, 0, 0, 20, 6) > 0, 'MENU header paints');

  // List: longest name in the 80-px inverted cell, level and P outside it.
  const [longestKey, longestName] = longestCreature();
  const list = frame0();
  drawLabel(list, 0, 0, 'COLLECTION');
  drawGlyph(list, 0, 12, '>');
  fillRect(list, 8, 12, 80, 8, 1);
  drawText(list, longestKey, 8, 12, 1);
  // White cell beyond the glyph run; the level gutter (x88..91 + digits at 92)
  // is untouched black so the 3x5 numeral stays visible.
  for (let column = 8 + textWidth(longestName); column < 88; column++)
    for (let line = 0; line < 8; line++)
      assert.equal(list[(12 + line) * 128 + column], 1, 'selected name cell is white');
  assert.equal(lit(list, 88, 12, 4, 8), 0, 'name cell ends before the level gutter');
  assert.ok(8 + textWidth(longestName) <= 88, 'longest name fits the 80-px cell');

  // Party: PARTY header and 1..3 slot digits.
  const party = frame0();
  drawLabel(party, 0, 0, 'PARTY');
  for (let slot = 0; slot < 3; slot++) {
    if (slot === 1) drawGlyph(party, 0, 12 + slot * 12, '>');
    drawGlyph(party, 8, 12 + slot * 12, String(slot + 1));
  }
  assert.ok(lit(party, 0, 0, 25, 6) > 0, 'PARTY header paints');
  assert.ok(lit(party, 8, 36, 5, 6) > 0, 'third party row paints');

  // Feedback line under the four rows: fits 128 px and never touches row 4.
  for (const message of ['ALREADY IN PARTY', 'SAVE NEEDED']) {
    const feedback = frame0();
    drawLabel(feedback, 0, 56, message);
    assert.ok(message.length * GLYPH <= 128, `${message} fits 128px`);
    assert.equal(lit(feedback, 0, 48, 128, 8), 0, `${message} stays below the rows`);
    assert.ok(litBounds(feedback).y0 >= 56, `${message} owns the feedback row`);
  }

  // Empty list: header only, no name rows.
  const empty = frame0();
  drawLabel(empty, 0, 0, 'COLLECTION');
  assert.equal(lit(empty, 0, 12, 128, 52), 0, 'empty collection paints no rows');
});

test('save status and duel caption geometry', () => {
  const save = read('src/save/SaveController.cpp');
  const tables = [...save.matchAll(
    /static const uint8_t (saving|failedText)\[6\]\[5\] PROGMEM = \{([\s\S]*?)\n    \};/g)];
  assert.equal(tables.length, 2, 'both save status glyph tables present');
  for (const [, name, body] of tables) {
    const rows = [...body.matchAll(/\{([^}]*)\}/g)]
      .map((m) => m[1].split(',').map((value) => Number(value.trim())));
    assert.equal(rows.length, 6, `${name} has six glyphs`);
    rows.forEach((glyph) => assert.equal(glyph.length, 5, `${name} glyph has five rows`));
    // Geometry: 5 glyph columns of 5 pixels scaled 2x at a 10-px pitch from
    // x34, five rows scaled 2x from y27.
    assert.ok(34 + 5 * 10 + 4 * 2 + 2 <= 128, `${name} columns fit 128px`);
    assert.ok(27 + 4 * 2 + 2 <= 64, `${name} rows fit 64px`);
  }
  assert.match(save, /Blit::fillRect\(34 \+ \(glyph \* 10\) \+ \(column \* 2\),/);
  assert.match(save, /27 \+ \(row \* 2\), 2, 2, BLACK\);/);

  // Save status glyph mask: six 5x5 glyphs at 2x scale on the 10-px pitch.
  const frame = frame0();
  for (let glyph = 0; glyph < 6; glyph++)
    for (let row = 0; row < 5; row++)
      for (let column = 0; column < 5; column++)
        fillRect(frame, 34 + glyph * 10 + column * 2, 27 + row * 2, 2, 2, 1);
  const bounds = litBounds(frame);
  assert.equal(bounds.count, 6 * 5 * 5 * 4, 'six 5x5 glyphs at 2x scale');
  assert.equal(bounds.x0, 34, 'glyph run starts at the pinned x34');
  assert.equal(bounds.x1, 93, 'glyph run ends before the right edge');
  assert.equal(bounds.y0, 27, 'glyph run starts at the pinned y27');
  assert.equal(bounds.y1, 36, 'glyph run ends above the message baseline');
});
