import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';

function viewer(query = '') {
  const elements = new Map(), images = [], draws = [];
  function element(id) {
    if (elements.has(id)) return elements.get(id);
    const value = {
      id, style: {}, children: [], complete: true, naturalWidth: id === '#combined16-atlas' ? 256 : 128, naturalHeight: id === '#combined16-atlas' ? 1408 : 128,
      width: id === '#scene-view' ? 384 : 128, height: id === '#scene-view' ? 256 : 64,
      value: id === '#combined16-zoom' ? '2' : id === '#view-zoom' || id === '#scale' ? '4' : '0', checked: false,
      handlers: {}, addEventListener(name, fn) { this.handlers[name] = fn; },
      replaceChildren(...children) { this.children = children; }, append(...children) { this.children.push(...children); },
      getContext() { return this.context ||= { set filter(value) { draws.push(['filter', id, value]); }, drawImage(...args) { draws.push([id, ...args]); }, strokeRect(...args) { draws.push(['boundary', ...args]); } }; },
      getBoundingClientRect() { return { left: 0, top: 0, width: 768, height: 512 }; },
    };
    elements.set(id, value); return value;
  }
  class Image {
    constructor() { images.push(this); this.complete = true; this.naturalWidth = 384; this.naturalHeight = 256; }
  }
  const context = vm.createContext({
    window: { location: { search: query } }, URLSearchParams, Image,
    Option: function(text, value) { this.text = text; this.value = String(value); },
    document: { querySelector: element, createElement: name => element('created-' + elements.size + '-' + name) },
  });
  vm.runInContext(fs.readFileSync('docs/assets/world-environment-32-spike/scene-collection.js', 'utf8'), context);
  vm.runInContext(fs.readFileSync('docs/assets/world-environment-32-spike/simple/scene-collection.js', 'utf8'), context);
  vm.runInContext(fs.readFileSync('docs/assets/environment32-viewer.js', 'utf8'), context);
  function loaded() { images.forEach(image => image.onload?.()); }
  loaded();
  return { elements, images, draws, loaded };
}

test('native128x64 display copies the inspected frame before the camera outline, independent of zoom', () => {
  const { elements, draws } = viewer();
  const native = draws.findLast(call => call[0] === '#native-view');
  assert.equal(native[1], elements.get('#game-view'));
  assert.deepEqual(native.slice(2), [0, 0]);
  assert.ok(draws.lastIndexOf(native) < draws.findLastIndex(call => call[0] === 'boundary'));
  const zoom = elements.get('#view-zoom'); zoom.value = '8'; zoom.handlers.change();
  assert.equal(elements.get('#game-view').style.width, '1024px');
  assert.equal(elements.get('#native-view').width, 128); assert.equal(elements.get('#native-view').height, 64);
  const html = fs.readFileSync('docs/world-environment-32-spike.html', 'utf8');
  assert.match(html, /#native-view\{width:128px;height:64px;/);
});

test('simple style defaults and style switching preserve the selected scene and native viewport',()=>{
 const {elements,images,loaded}=viewer('?scene=dewcap');
 assert.equal(elements.get('#scene-style').value,'simple');
 assert.equal(elements.get('#example').value,'6');
 assert.ok(images.some(image=>image.src.includes('/simple/scenes/dewcap/scene-terrain.png')));
 const style=elements.get('#scene-style');style.value='dither';style.handlers.change({target:style});loaded();
 assert.equal(elements.get('#example').value,'6');
 assert.ok(images.some(image=>image.src==='assets/world-environment-32-spike/scenes/dewcap/scene-terrain.png'));
 assert.equal(elements.get('#native-view').width,128);assert.equal(elements.get('#native-view').height,64);
 assert.equal(viewer('?style=dither&scene=cave').elements.get('#scene-style').value,'dither');
 const set=elements.get('#set');set.value='simple-habitats';set.handlers.change({target:set});
 assert.equal(elements.get('#atlas').src,'assets/world-environment-32-spike/simple/habitats/atlas32.png');
});

test('combined sheet uses16px boundaries and integer native zoom',()=>{
 const {elements}=viewer(),atlas=elements.get('#combined16-atlas'),zoom=elements.get('#combined16-zoom'),bounds=elements.get('#combined16-bounds'),grid=elements.get('#combined16-grid');
 assert.equal(atlas.width,512);assert.equal(atlas.height,2816);assert.equal(grid.style.backgroundSize,'32px 32px');
 zoom.value='1';zoom.handlers.change();bounds.checked=true;bounds.handlers.change();
 assert.equal(atlas.width,256);assert.equal(atlas.height,1408);assert.equal(grid.style.backgroundSize,'16px 16px');assert.equal(grid.style.display,'block');
});

test('all five examples appear together and dark cave keeps the original player', () => {
  const html = fs.readFileSync('docs/world-environment-32-spike.html', 'utf8');
  for (const slug of ['river', 'town', 'city', 'cave', 'cave-dark']) {
    const card = html.match(new RegExp(`<article class="example-card" id="${slug}">([\\s\\S]*?)</article>`));
    assert.ok(card, slug + ' missing from simultaneous scene gallery');
    assert.equal((card[1].match(/class="native-crop/g) || []).length, 3);
    assert.equal((card[1].match(/class="large-crop/g) || []).length, 3);
    assert.ok(!/\bhidden\b|display:none/.test(card[1]));
    if (slug === 'cave-dark') {
      assert.equal((card[1].match(/ inverted"/g) || []).length, 0);
      assert.equal((card[1].match(/scenes\/cave-dark\//g) || []).length, 7);
    }
  }
  const { elements, images, draws } = viewer('?scene=cave-dark');
  assert.equal(elements.get('#example').value, '4');
  assert.ok(images.some(image => image.src.endsWith('/cave-dark/scene-terrain.png')));
  assert.ok(draws.findLast(call => call[0] === '#scene-view')[1].src.endsWith('/up.png'));
  assert.ok(!draws.some(call => call[0] === 'filter' && call[2] === 'invert(1)'));
  for (const figure of elements.get('#saved-views').children) assert.ok(figure.children[0].children[0].src.includes('/cave-dark/'));
});

test('scene selector and deep links update terrain, player facing, presets and saved views', () => {
  const { elements, images, draws, loaded } = viewer('?scene=cave');
  const select = elements.get('#example'); assert.equal(select.value, '3');
  for (const [index, folder] of ['','scenes/town','scenes/city','scenes/cave'].entries()) {
    select.value = String(index); select.handlers.change({ target: select }); loaded();
    assert.ok(images.some(image => image.src.endsWith((folder ? folder + '/' : '') + 'scene-terrain.png')));
    assert.equal(elements.get('#position').children.length, 3);
    assert.equal(elements.get('#saved-views').children.length, 3);
    const position = elements.get('#position'); position.value = '2'; position.handlers.change({ target: position });
    const fullDraw = draws.findLast(call => call[0] === '#scene-view');
    assert.ok(fullDraw[1].src.endsWith(index === 2 ? '/left.png' : '/down.png'));
    const nativeDraw = draws.findLast(call => call[0] === '#native-view');
    assert.equal(nativeDraw[1], elements.get('#game-view'));
  }
});

test('all creature habitats have roster references, native views and working deep links', () => {
  const html = fs.readFileSync('docs/world-environment-32-spike.html', 'utf8');
  const roster = JSON.parse(fs.readFileSync('data/json/creatures.json'));
  for (const [i, slug] of ['huskfield','dewcap','shellstone','cinderflint'].entries()) {
    const scene = JSON.parse(fs.readFileSync(`docs/assets/world-environment-32-spike/scenes/${slug}/scene.json`));
    const card = html.match(new RegExp(`<article class="example-card" id="${slug}">([\\s\\S]*?)</article>`));
    assert.ok(card); assert.equal((card[1].match(/class="native-crop/g) || []).length, 3);
    assert.ok(scene.creatures.length >= 5);
    for (const creature of scene.creatures) {
      const source = roster.find(c => c.id === creature.id);
      assert.equal(creature.name, source.name);
      assert.deepEqual(creature.types, [source.type1,source.type2].filter(t => t !== 'none'));
      assert.ok(fs.existsSync('docs/' + creature.sprite));
      assert.ok(card[1].includes(creature.sprite));
    }
    const {elements, images} = viewer('?scene=' + slug);
    assert.equal(elements.get('#example').value, String(i + 5));
    assert.ok(images.some(image => image.src.endsWith(`/scenes/${slug}/scene-terrain.png`)));
    const set = elements.get('#set'); set.value = 'habitats'; set.handlers.change({target:set});
    assert.ok(elements.get('#atlas').src.endsWith('/habitats/atlas32.png'));
  }
});
