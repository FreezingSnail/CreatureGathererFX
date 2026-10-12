#!/usr/bin/env node
// Bake the revised32px art into16px render cells without changing map mechanics.
import fs from 'node:fs';
import { decodePng, encodePng } from './creature-sprite-export.mjs';
import { habitatGround, habitatProp } from './habitat-terrain.mjs';
const base='docs/assets/world-environment-32-spike';
const root=`${base}/lure-world`;
const props=decodePng(fs.readFileSync(`${base}/habitats/lure-clarity/atlas32.png`));
const atlas=decodePng(fs.readFileSync(`${base}/habitats/terrain/atlas32.png`));
const legacy=decodePng(fs.readFileSync('images/tiles_16x16.png'));
const world=JSON.parse(fs.readFileSync('maps/world_map.json'));
const scene={size:[192,128],terrain:{biome:1,map:['000000000000','000000000000','001101111110','001100111100','001100111100','001100111100','001100111100','001100001000']}};
const image=habitatGround(scene,atlas);
function blit(source,dx,dy) {
 for(let y=0;y<source.height;y++)for(let x=0;x<source.width;x++){
  const si=(y*source.width+x)*4,di=((dy+y )*192+dx+x)*4;
  if(source.pixels[si+3])image.pixels.set(source.pixels.subarray(si,si+4),di);
 }
}
// Extend the same canopy palette across the older visible western map edge.
blit(habitatProp(props,12),0,0);blit(habitatProp(props,0),32,0);
// Full32px environmental forms fit the two non-walkable northern rows.
for(const [id,x] of [[0,64],[1,96],[2,128],[3,160]])blit(habitatProp(props,id),x,0);
function smallFixture(id) {
 const full=habitatProp(props,id),small={width:16,height:16,pixels:new Uint8Array(1024)};
 // Small fixtures were authored at16px inside32px cells. Crop them instead of
 // averaging them down into noisy dark blobs.
 for(let y=0;y<16;y++)for(let x=0;x<16;x++) {
  const si=((y+8)*32+x+8)*4;
  small.pixels.set(full.pixels.subarray(si,si+4),(y*16+x)*4);
 }
 return small;
}
// A thin connected contour groups the clearing; foliage provides the detail.
for(let y=32;y<128;y++)for(const x of [66,189]) image.pixels.set([0,0,0,255],(y*192+x)*4);
for(let x=66;x<190;x++) if(x<128||x>=144)image.pixels.set([0,0,0,255],(126*192+x)*4);
// Authored interactions occupy one16px collision cell each. These are small props,
// not32px scenery over the neighboring walkable tiles.
blit(smallFixture(8),80,32); // lure container: world(9,2)
blit(smallFixture(9),160,32); // gather plant: world(14,2)
fs.mkdirSync(root,{recursive:true});
function write(file,data){if(!fs.existsSync(file)||!fs.readFileSync(file).equals(data))fs.writeFileSync(file,data);}
write('images/LureWorld_16x16.png',encodePng(image));
write(`${root}/terrain.png`,encodePng(image));
function crop(px,py,name,facing) {
 // Firmware draws the masked player at(56,24); the map is anchored at tile(3,2).
 const camera=[(px-4-3)*16,(py-2)*16];
 const player=decodePng(fs.readFileSync(`docs/assets/world-player/${facing}.png`));
 const crop={width:128,height:64,pixels:new Uint8Array(32768)};
 for(let y=0;y<64;y++)for(let x=0;x<128;x++){
  const wx=x+camera[0],wy=y+camera[1],di=(y*128+x)*4;
  if(wx>=0&&wx<192&&wy>=0&&wy<128){const si=(wy*192+wx)*4;crop.pixels.set(image.pixels.subarray(si,si+4),di);}
  else {
    const worldX=wx+64,worldY=wy,tx=Math.floor(worldX/16),ty=Math.floor(worldY/16);
    const gid=world.tiles[ty]?.[tx]||0;
    if(gid) {
      const lx=(gid-1)%16*16+(worldX%16+16)%16,ly=Math.floor((gid-1)/16)*16+(worldY%16+16)%16;
      const si=(ly*legacy.width+lx)*4;crop.pixels.set(legacy.pixels.subarray(si,si+4),di);
    } else crop.pixels.set([0,0,0,255],di);
  }
  if(x>=56&&x<72&&y>=24&&y<40){const si=((y-24)*16+x-56)*4;if(player.pixels[si+3])crop.pixels.set(player.pixels.subarray(si,si+4),di);}
 }
 write(`${root}/${name}.png`,encodePng(crop));
 return {name,playerLocation:[px,py],facing,camera,playerScreen:[56,24]};
}
const views=[crop(11,5,'boot','down'),crop(10,2,'container','left'),crop(13,2,'plant','right')];
write(`${root}/manifest.json`,Buffer.from(JSON.stringify({reviewOnly:true,optIn:'CGFX_LURE_WEDGE',worldOrigin:[4,0],size:[192,128],renderCell:[16,16],sceneryCell:[32,32],frames:96,source:'images/LureWorld_16x16.png',artSource:'habitats/lure-clarity/atlas32.png',fixtureSource:'habitats/lure-clarity/atlas32.png',sceneryIds:[0,1,2,3],mechanics:'Existing map and properties unchanged',terrain:scene.terrain,container:[9,2],plant:[14,2],views},null,2)+'\n'));
console.log('PASS: lure scenery192x128;96 native16px frames;32px scenery; original box/plant cells and3 device previews.');

const page='docs/world-environment-32-spike.html';
let html=fs.readFileSync(page,'utf8');
if(!html.includes('<!-- lure-world-start -->')) html=html.replace('<section id="asset-comparison">','<!-- lure-world-start -->\n<!-- lure-world-end -->\n<section id="asset-comparison">');
const gallery=`<section id="lure-world"><h2>Playable lure clearing</h2><p>The lure demo now uses the clear silhouettes, sparse ground and32px scenery, baked into16px render cells. Clear silhouettes continue across the visible western approach. Its box, plant, collision and encounter layout are unchanged.</p><img src="assets/world-environment-32-spike/lure-world/terrain.png" width="384" height="256" alt="Full192px playable lure clearing and western approach at2x"><div class="example-crops">${views.map(v=>`<figure><figcaption>${v.name}</figcaption><p>1× ·128×64</p><div class="device-frame"><img class="native-crop" src="assets/world-environment-32-spike/lure-world/${v.name}.png" width="128" height="64" alt="Playable lure clearing ${v.name} native view"></div><p>2× inspection</p><img class="large-crop" src="assets/world-environment-32-spike/lure-world/${v.name}.png" width="256" height="128" alt="Playable lure clearing ${v.name} enlarged"></figure>`).join('')} </div><p>Run <code>make lure-demo ARDENS=/path/to/Ardens</code> to play it.</p></section>`;
fs.writeFileSync(page,html.replace(/<!-- lure-world-start -->[\s\S]*?<!-- lure-world-end -->/,'<!-- lure-world-start -->\n'+gallery+'\n<!-- lure-world-end -->'));
