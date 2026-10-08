// Browser-only movement review at the device's actual camera dimensions.
(() => {
  const scenes = window.WORLD_TRAVERSAL_SCENES, panel = document.querySelector('#traversal'), canvas = document.querySelector('#walk-preview'), ctx = canvas.getContext('2d');
  const status = document.querySelector('#walk-status'), select = document.querySelector('#walk-scene');
  const atlas = new Image(), playerImage = new Image(), npcImage = new Image(), walkImage = new Image();
  const stepMilliseconds = 16 * 1000 / 52;
  let scene = scenes[0], position = scene.player.slice(), facing = scene.facing, motion = null, frame = null;
  const held = new Set(), directions = { UP: [0,-1], DOWN: [0,1], LEFT: [-1,0], RIGHT: [1,0] };
  const keys = { ArrowUp:'UP', ArrowDown:'DOWN', ArrowLeft:'LEFT', ArrowRight:'RIGHT' };
  function render(now = performance.now()) {
    if (!atlas.complete || !atlas.naturalWidth || !playerImage.complete || !playerImage.naturalWidth) return;
    const travel = motion ? Math.min(16, Math.floor(Math.max(0, now-motion.start) * 16 / stepMilliseconds)) : 0;
    const [dx,dy] = motion ? directions[motion.direction] : [0,0];
    const cameraX = position[0] * 16 + dx * travel - 56, cameraY = position[1] * 16 + dy * travel - 24;
    ctx.imageSmoothingEnabled = false; ctx.fillStyle = '#fff'; ctx.fillRect(0,0,128,64);
    scene.data.forEach((gid,index) => {
      const x = index % scene.width * 16 - cameraX, y = Math.floor(index / scene.width) * 16 - cameraY;
      if (x <= -16 || y <= -16 || x >= 128 || y >= 64) return;
      ctx.drawImage(atlas,(gid-1)%16*16,Math.floor((gid-1)/16)*16,16,16,x,y,16,16);
    });
    if (npcImage.complete && npcImage.naturalWidth) for (const npc of scene.npcs || []) {
      ctx.drawImage(npcImage,npc.sprite%4*16,Math.floor(npc.sprite/4)*16,16,16,npc.x*16-cameraX,npc.y*16-cameraY,16,16);
    }
    const playerFrame = { DOWN: 0, UP: 1, LEFT: 2, RIGHT: 3 }[facing];
    if (motion && walkImage.complete && walkImage.naturalWidth) {
      const walkFrame = playerFrame * 2 + (travel < 8 ? 0 : 1);
      ctx.drawImage(walkImage,walkFrame%4*16,Math.floor(walkFrame/4)*16,16,16,56,24,16,16);
    } else ctx.drawImage(playerImage,playerFrame%2*16,Math.floor(playerFrame/2)*16,16,16,56,24,16,16);
  }
  function requestFrame() { if (frame === null && motion) frame = requestAnimationFrame(tick); }
  function tick(now) {
    frame = null;
    if (!motion) return;
    if (now-motion.start >= stepMilliseconds) {
      position = motion.to; motion = null; render(now);
      const next = Array.from(held).at(-1);
      if (next) step(next, now);
    } else render(now);
    requestFrame();
  }
  function step(direction, now = performance.now()) {
    if (motion) return;
    facing = direction; const [dx,dy] = directions[direction], x = position[0]+dx, y = position[1]+dy;
    if (x >= 0 && y >= 0 && x < scene.width && y < scene.height && scene.walkable[y*scene.width+x]) {
      motion = { to: [x,y], direction, start: now }; status.textContent = 'Walking · '+scene.name;
      render(now); requestFrame();
    } else { status.textContent = 'Blocked · '+scene.name; render(now); }
  }
  function enter() {
    if (motion) return;
    const [dx,dy] = directions[facing], door = scene.doors.find(d => d.x === position[0]+dx && d.y === position[1]+dy);
    const npc = (scene.npcs || []).find(n => n.x === position[0]+dx && n.y === position[1]+dy);
    status.textContent = npc ? npc.name+' · '+npc.message : door ? door.name+' entrance reached' : 'Face a doorway or person and press A';
  }
  for (const direction of Object.keys(directions)) document.querySelector('#walk-'+direction.toLowerCase()).addEventListener('click',()=>{panel.focus();step(direction);});
  document.querySelector('#walk-enter').addEventListener('click',()=>{panel.focus();enter();});
  panel.addEventListener('keydown',event=>{
    if (event.target === select) return;
    if (keys[event.key]) {
      event.preventDefault(); const direction = keys[event.key];
      if (!held.has(direction)) { held.add(direction); step(direction); }
    } else if (event.key.toLowerCase() === 'a') { event.preventDefault(); enter(); }
  });
  window.addEventListener('keyup',event=>{if (keys[event.key]) held.delete(keys[event.key]);});
  window.addEventListener('blur',()=>held.clear());
  document.addEventListener('visibilitychange',()=>{if (document.hidden) held.clear();});
  select.addEventListener('change',()=>{
    if (frame !== null) cancelAnimationFrame(frame);
    frame = null; motion = null; held.clear();
    scene=scenes[Number(select.value)];position=scene.player.slice();facing=scene.facing;status.textContent=scene.name;render();
  });
  atlas.onload = () => render(); playerImage.onload = () => render(); npcImage.onload = () => render(); walkImage.onload = () => render();
  atlas.src = 'assets/world-environment-spike/atlas-native.png'; playerImage.src = 'assets/world-player/sprites16.png'; npcImage.src = 'assets/world-npcs/sprites16.png';
  walkImage.src = 'assets/world-player/walk/sprites16.png';
})();
