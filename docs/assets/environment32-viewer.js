const root = 'assets/world-environment-32-spike/';
const scenes = window.environment32Scenes;
const names = ['river', 'town', 'city', 'cave', 'cave-dark', 'huskfield', 'dewcap', 'shellstone', 'cinderflint'];
const requested = names.indexOf(new URLSearchParams(window.location.search).get('scene'));
let scene = scenes[Math.max(0, requested)];
const camera = [...scene.views[0].camera];
let selected = 0;
const terrain = new Image();
const players = Object.fromEntries(['up', 'right', 'down', 'left'].map(facing => {
  const image = new Image(); image.src = `assets/world-player/${facing}.png`; image.onload = draw; return [facing, image];
}));
const game = document.querySelector('#game-view'), overview = document.querySelector('#scene-view'), native = document.querySelector('#native-view');
function draw() {
  const player = players[scene.views[selected].facing];
  if (!terrain.complete || !terrain.naturalWidth || !player.complete || !player.naturalWidth) return;
  camera[0] = Math.max(0, Math.min(scene.size[0] - 128, camera[0]));
  camera[1] = Math.max(0, Math.min(scene.size[1] - 64, camera[1]));
  const view = scene.views[selected];
  const full = overview.getContext('2d'); full.imageSmoothingEnabled = false;
  full.drawImage(terrain, 0, 0); full.drawImage(player, ...view.player);
  const context = game.getContext('2d'); context.imageSmoothingEnabled = false;
  context.drawImage(overview, ...camera, 128, 64, 0, 0, 128, 64);
  native.getContext('2d').drawImage(game, 0, 0);
  // Draw the review boundary only after copying the monochrome game viewport.
  full.strokeStyle = '#e04040'; full.lineWidth = 1; full.strokeRect(camera[0] + .5, camera[1] + .5, 127, 63);
  document.querySelector('#camera-status').textContent = `${view.name} · camera (${camera.join(', ')}) · player 16×16 · viewport 128×64`;
}
terrain.onload = draw;
const position = document.querySelector('#position'), example = document.querySelector('#example');
function loadScene() {
  selected = 0; camera.splice(0, 2, ...scene.views[0].camera);
  overview.width = scene.size[0]; overview.height = scene.size[1];
  overview.style.width = scene.size[0] * 2 + 'px'; overview.style.height = scene.size[1] * 2 + 'px';
  position.replaceChildren(...scene.views.map((view, index) => new Option(view.name, index)));
  document.querySelector('#scene-title').textContent = scene.name + ' · ' + scene.size.join('×');
  document.querySelector('#scene-description').textContent = scene.description || 'Compact cottage and garden to the west, river crossing in the middle, and a grove to the east.';
  const folder = root + (scene.folder ? scene.folder + '/' : '');
  document.querySelector('#scene-file').href = folder + 'scene.png';
  document.querySelector('#placements-file').href = folder + 'scene.json';
  const saved = document.querySelector('#saved-views'); saved.replaceChildren();
  scene.views.forEach((view, index) => {
    const figure = document.createElement('figure'), link = document.createElement('a'), image = document.createElement('img'), caption = document.createElement('figcaption');
    link.href = image.src = folder + 'viewport-' + (index + 1) + '.png'; image.alt = scene.name + ' · ' + view.name;
    caption.textContent = view.name; link.append(image); figure.append(link, caption); saved.append(figure);
  });
  terrain.src = folder + 'scene-terrain.png';
}
example.value = String(scenes.indexOf(scene));
example.addEventListener('change', event => { scene = scenes[Number(event.target.value)]; loadScene(); });
const viewZoom = document.querySelector('#view-zoom');
function resizeViewport() { const n = Number(viewZoom.value); game.style.width = 128 * n + 'px'; game.style.height = 64 * n + 'px'; }
viewZoom.addEventListener('change', resizeViewport); resizeViewport(); loadScene();
document.querySelector('#position').addEventListener('change', event => {
  selected = Number(event.target.value); camera.splice(0, 2, ...scene.views[selected].camera); draw();
});
for (const [name, dx, dy] of [['left', -16, 0], ['right', 16, 0], ['up', 0, -16], ['down', 0, 16]])
  document.querySelector(`#camera-${name}`).addEventListener('click', () => { camera[0] += dx; camera[1] += dy; draw(); });
document.querySelector('#camera-reset').addEventListener('click', () => { camera.splice(0, 2, ...scene.views[selected].camera); draw(); });
overview.addEventListener('click', event => {
  const bounds = overview.getBoundingClientRect();
  camera[0] = Math.round((event.clientX - bounds.left) * overview.width / bounds.width - 64);
  camera[1] = Math.round((event.clientY - bounds.top) * overview.height / bounds.height - 32); draw();
});
const atlas = document.querySelector('#atlas'), scale = document.querySelector('#scale'), grid = document.querySelector('#grid');
function updateSheet() {
  const n = Number(scale.value); atlas.width = atlas.naturalWidth * n; atlas.height = atlas.naturalHeight * n;
  grid.style.backgroundSize = `${32 * n}px ${32 * n}px`;
  grid.style.display = document.querySelector('#bounds').checked ? 'block' : 'none';
}
atlas.onload = updateSheet; if (atlas.complete) updateSheet();
scale.addEventListener('change', updateSheet); document.querySelector('#bounds').addEventListener('change', updateSheet);
document.querySelector('#set').addEventListener('change', event => { atlas.src = root + (event.target.value === 'clarity' ? 'habitats/lure-clarity/' : event.target.value === 'terrain' ? 'habitats/terrain/' : ['expansion', 'habitats'].includes(event.target.value) ? event.target.value + '/' : '') + 'atlas32.png'; });
