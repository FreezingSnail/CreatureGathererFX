#!/usr/bin/env node
// Headless frame harness for mock/index.html.
// Stubs window/document/canvas, loads mock/data.js + mock/battle-mock.js (browser
// branch), and renders 128x64 frames so the mock can be verified without a
// desktop browser. Test: node --test tools/tests/battle-design-spike-frame_test.mjs
// Previews: node tools/battle-design-spike-preview.mjs
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import { fileURLToPath } from 'node:url';
import { decodePng, encodePng } from './creature-sprite-export.mjs';

const repo = path.dirname(fileURLToPath(import.meta.url)) + '/..';
const W = 128, H = 64;

class StubEvent {
  constructor(type) { this.type = type; this.target = null; }
}

function elementStub(properties = {}) {
  const listeners = new Map();
  return {
    listeners,
    style: {},
    dataset: {},
    textContent: '',
    value: '',
    checked: false,
    addEventListener(type, handler) {
      if (!listeners.has(type)) listeners.set(type, []);
      listeners.get(type).push(handler);
    },
    dispatchEvent(event) {
      event.target = event.target || this;
      for (const handler of listeners.get(event.type) ?? []) handler(event);
      return true;
    },
    ...properties,
  };
}

export function createHarness(options = {}) {
  const imageCache = new Map();
  let now = 0;
  let rafCallback = null;
  const frame = new Uint8Array(W * H);
  const windowListeners = new Map();

  const ctx = {
    fillStyle: '#000',
    fillRect(x, y, w, h) {
      const value = ctx.fillStyle === '#fff' ? 1 : 0;
      for (let py = Math.max(0, Math.floor(y)); py < Math.min(H, y + h); py++)
        for (let px = Math.max(0, Math.floor(x)); px < Math.min(W, x + w); px++)
          frame[py * W + px] = value;
    },
    drawImage(image, x, y) {
      if (!image.ready) return;
      for (let sy = 0; sy < image.height; sy++) for (let sx = 0; sx < image.width; sx++) {
        const offset = (sy * image.width + sx) * 4;
        if (image.pixels[offset + 3] === 0) continue;
        const tx = x + sx, ty = y + sy;
        if (tx < 0 || tx >= W || ty < 0 || ty >= H) continue;
        frame[ty * W + tx] = image.pixels[offset] >= 128 ? 1 : 0;
      }
    },
  };

  class StubImage {
    constructor() { this.ready = false; this.onload = null; }
    set src(value) {
      let entry = imageCache.get(value);
      if (!entry) {
        entry = decodePng(fs.readFileSync(path.resolve(repo, 'mock', value)));
        imageCache.set(value, entry);
      }
      this.width = entry.width;
      this.height = entry.height;
      this.pixels = entry.pixels;
      this.ready = true;
      if (this.onload) this.onload();
    }
  }

  const elements = new Map();
  const getElement = (id) => {
    if (!elements.has(id)) {
      const stub = elementStub();
      if (id === 'screen') {
        stub.width = W;
        stub.height = H;
        stub.getContext = () => ctx;
      }
      if (id === 'trim-type') stub.value = '4single';
      if (id === 'duel-stances') stub.value = '3';
      if (id === 'duel-read') stub.value = '45';
      if (id === 'scale') stub.value = '5';
      if (['trim-arrows', 'trim-press', 'trim-foe', 'duel-explain'].includes(id)) stub.checked = true;
      elements.set(id, stub);
    }
    return elements.get(id);
  };

  const modeRadios = ['current', 'trim', 'duel'].map((value) => elementStub({ value, checked: value === 'current' }));
  const padButtons = ['left', 'right', 'up', 'down', 'a', 'b'].map((key) => {
    const button = elementStub();
    button.dataset.key = key;
    return button;
  });

  const documentStub = {
    body: elementStub(),
    getElementById: getElement,
    querySelectorAll(selector) {
      if (selector === 'input[name="mode"]') return modeRadios;
      if (selector === '[data-key]') return padButtons;
      return [];
    },
  };
  documentStub.body.dataset = {};
  if (options.mode) documentStub.body.dataset.mode = options.mode;
  if (options.duel) documentStub.body.dataset.duel = options.duel;
  if (options.scoring) documentStub.body.dataset.scoring = options.scoring;
  const windowStub = {
    addEventListener(type, handler) {
      if (!windowListeners.has(type)) windowListeners.set(type, []);
      windowListeners.get(type).push(handler);
    },
  };

  const context = vm.createContext({
    window: windowStub,
    document: documentStub,
    location: { search: options.search || '' },
    performance: { now: () => now },
    requestAnimationFrame(callback) { rafCallback = callback; },
    Image: StubImage,
    Event: StubEvent,
    console,
  });
  const dataSource = fs.readFileSync(path.join(repo, 'mock/data.js'), 'utf8');
  const iconsSource = fs.readFileSync(path.join(repo, 'mock/icons.js'), 'utf8');
  const matricesSource = fs.readFileSync(path.join(repo, 'mock/matrices.js'), 'utf8');
  const mockSource = fs.readFileSync(path.join(repo, 'mock/battle-mock.js'), 'utf8');
  vm.runInContext(`${dataSource}\n${iconsSource}\n${matricesSource}\n${mockSource}`, context, { filename: 'battle-mock.js' });

  const KEY_ALIASES = {
    left: 'ArrowLeft', right: 'ArrowRight', up: 'ArrowUp', down: 'ArrowDown',
    a: 'z', b: 'x',
  };
  const harness = {
    frame,
    window: windowStub,
    state: () => windowStub.__battleMock,
    key(key, type) {
      const event = new StubEvent(type);
      event.key = KEY_ALIASES[key] || key;
      event.preventDefault = () => {};
      for (const handler of windowListeners.get(type) ?? []) handler(event);
    },
    press(key) { harness.key(key, 'keydown'); harness.key(key, 'keyup'); },
    advance(milliseconds = 20) {
      now += milliseconds;
      if (rafCallback) {
        const callback = rafCallback;
        rafCallback = null;
        callback(now);
      }
    },
    advances(count, milliseconds = 20) { for (let i = 0; i < count; i++) harness.advance(milliseconds); },
    switchMode(mode) {
      for (const radio of modeRadios) radio.checked = radio.value === mode;
      const radio = modeRadios.find((entry) => entry.value === mode);
      radio.dispatchEvent(new StubEvent('change'));
      harness.advance(0);
    },
    setControl(id, value) {
      const element = getElement(id);
      if (typeof value === 'boolean') element.checked = value;
      else element.value = String(value);
      element.dispatchEvent(new StubEvent(typeof value === 'boolean' ? 'change' : 'input'));
    },
    whitePixels() {
      let count = 0;
      for (const value of frame) if (value) count++;
      return count;
    },
    spritePixels() {
      let count = 0;
      for (let y = 0; y < 48; y++) for (let x = 0; x < 48; x++) {
        count += frame[y * W + x];
        count += frame[y * W + 80 + x];
      }
      return count;
    },
    toPng(scale = 1) {
      const pixels = new Uint8Array(W * scale * H * scale * 4);
      for (let y = 0; y < H * scale; y++) for (let x = 0; x < W * scale; x++) {
        const value = frame[Math.floor(y / scale) * W + Math.floor(x / scale)] ? 255 : 0;
        pixels.set([value, value, value, 255], (y * W * scale + x) * 4);
      }
      return encodePng({ width: W * scale, height: H * scale, pixels });
    },
  };
  return harness;
}

export function writePreview(harness, file, scale = 3) {
  const output = path.join(repo, 'docs/assets/battle-design-spike', file);
  fs.mkdirSync(path.dirname(output), { recursive: true });
  fs.writeFileSync(output, harness.toPng(scale));
  return output;
}
