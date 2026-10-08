const root = 'assets/world-environment-spike/';
const select = document.querySelector('#version'), viewSelect = document.querySelector('#view');
const image = document.querySelector('#preview'), sheet = document.querySelector('.sheet');
const world = new Image(), buildingAtlas = new Image(), playerSheet = new Image(), fixtureAtlas = new Image();
function folder() { return root + (select.value === 'current' ? '' : select.value + '/'); }
function update() {
  const current = select.value === 'current';
  const showcaseOption = viewSelect.querySelector('option[value="fixture-showcase"]');
  showcaseOption.disabled = !current;
  if (!current && viewSelect.value === 'fixture-showcase') viewSelect.value = 'atlas-native';
  const view = viewSelect.value, zoom = Number(document.querySelector('#zoom').value);
  const sizes = { overworld: [512, 512, 'Overworld · 32×32 tiles'], interiors: [512, 128, 'Interiors · 32×8 tiles'], 'fixture-showcase': [512, 512, 'Fixture showcase · 32×32 tiles'], 'atlas-native': [256, (current || select.value === 'v5') ? 512 : 256, (current || select.value === 'v5') ? 'Combined atlas · 16×32 tiles' : 'Atlas · 16×16 tiles'] };
  const [w, h, label] = sizes[view];
  image.src = view === 'fixture-showcase' ? root + 'fixtures/overworld.png' : folder() + view + '.png';
  image.width = w * zoom; image.height = h * zoom; image.alt = label;
  sheet.style.setProperty('--cell', 16 * zoom + 'px');
  document.querySelector('#dimensions').textContent = label + ' · ' + w + '×' + h + ' native pixels';
}
function refresh() {
  update();
  document.querySelector('#environment-areas').hidden = false;
  world.src = folder() + 'overworld.png';
  buildingAtlas.src = folder() + 'atlas-native.png';
}
viewSelect.addEventListener('change', update);
document.querySelector('#zoom').addEventListener('change', update);
select.addEventListener('change', refresh);
document.querySelector('#grid').addEventListener('change', event => document.querySelector('.viewport').classList.toggle('grid-on', event.target.checked));
function card(name, canvas, description, target) {
  const article = document.createElement('article'); article.className = 'card';
  const h = document.createElement('h3'); h.textContent = name;
  const p = document.createElement('p'); p.textContent = description;
  article.append(h, canvas, p); target.append(article);
}
function canvas(width, height, zoom, label) {
  const result = document.createElement('canvas'); result.width = width; result.height = height;
  result.style.width = width * zoom + 'px'; result.style.height = height * zoom + 'px'; result.setAttribute('aria-label', label);
  result.getContext('2d').imageSmoothingEnabled = false; return result;
}
const names = ['Meadow', 'Woodland', 'Coast', 'Marsh', 'Mountain', 'Snow', 'Village', 'Ruins'];
world.onload = () => {
  const target = document.querySelector('#zones'); target.replaceChildren();
  names.forEach((name, index) => {
    const c = canvas(256, 128, 1, name + ' environment');
    c.getContext('2d').drawImage(world, index % 2 * 256, Math.floor(index / 2) * 128, 256, 128, 0, 0, 256, 128);
    card(name, c, 'Environment map area.', target);
  });
};
function drawEntrances() {
  if (!buildingAtlas.complete || !buildingAtlas.naturalWidth || !playerSheet.complete || !playerSheet.naturalWidth) return;
  const target = document.querySelector('#entrances'); target.replaceChildren();
  const classic = select.value === 'current', compact = ['v5', 'v3'].includes(select.value), offset = classic ? 32 : compact ? 16 : 0, height = classic ? 32 : compact ? 48 : 64;
  ['Cottage', 'Woodland cabin', 'Village shop', 'Watchtower'].forEach((name, index) => {
    const c = canvas(64, height + 16, 2, name + ' doorway and actual 16×16 player'), ctx = c.getContext('2d');
    ctx.fillStyle = '#fff'; ctx.fillRect(0, 0, 64, height + 16);
    ctx.drawImage(buildingAtlas, index * 64, 128 + offset, 64, height, 0, 0, 64, height);
    ctx.drawImage(playerSheet, 16, height); ctx.strokeStyle = '#000'; ctx.setLineDash([2, 2]); ctx.strokeRect(16.5, height + .5, 15, 15);
    card(name, c, 'One-tile player on the approach. Face UP + A; review only.', target);
  });
}
buildingAtlas.onload = drawEntrances; playerSheet.onload = drawEntrances;
playerSheet.src = 'assets/world-player/up.png';
const fixtureNames = ['Cottage', 'Woodland cabin', 'Village shop', 'Ruined watchtower', 'Horizontal bridge', 'Vertical bridge', 'Pond banks', 'Waterfall', 'Broadleaf tree', 'Tree grove', 'Boulders', 'Shrine arch', 'Fishing dock', 'Marsh boardwalk', 'Gathering camp', 'Well and garden'];
fixtureAtlas.onload = () => {
  const target = document.querySelector('#fixtures'); target.replaceChildren();
  fixtureNames.forEach((name, index) => {
    const footprints = [[3,2,0,2],[3,2,0,2],[3,2,0,2],[3,2,0,2],[4,1,0,3],[1,4,1,0],[3,2,0,2],[2,2,1,2],[2,2,1,2],[3,2,0,2],[2,1,1,3],[2,2,1,2],[2,2,1,2],[1,4,1,0],[2,2,1,2],[3,2,0,2]];
    const [w,h,ox,oy] = footprints[index], c = canvas(w*16, h*16, 2, name + ' multi-tile assembly');
    c.getContext('2d').drawImage(fixtureAtlas, index%4*64+ox*16, 256+Math.floor(index/4)*64+oy*16, w*16, h*16, 0, 0, w*16, h*16);
    card(name, c, w+'×'+h+' tiles · player footprint is 1×1.', target);
  });
};
fixtureAtlas.src = root + 'atlas-native.png';
refresh();
