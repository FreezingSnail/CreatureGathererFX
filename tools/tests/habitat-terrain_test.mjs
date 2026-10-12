import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { decodePng } from '../creature-sprite-export.mjs';
import { habitatGround, habitatProp } from '../habitat-terrain.mjs';
const base='docs/assets/world-environment-32-spike';
const read=p=>decodePng(fs.readFileSync(p));
const atlas=read(`${base}/habitats/terrain/atlas32.png`);
test('all sixteen ground tiles repeat with matching edges and contain full-surface texture',()=>{
 assert.deepEqual([atlas.width,atlas.height],[128,128]);
 const sample=(c,r,x,y)=>atlas.pixels[((r*32+y)*128+c*32+x)*4];
 for(let r=0;r<4;r++)for(let c=0;c<4;c++){
   let black=0;for(let y=0;y<32;y++)for(let x=0;x<32;x++)black+=sample(c,r,x,y)===0;
   assert.ok(black>60&&black<970,`material ${r},${c} must have visible texture`);
   for(let i=0;i<32;i++){assert.equal(sample(c,r,0,i),sample(c,r,31,i));assert.equal(sample(c,r,i,0),sample(c,r,i,31));}
 }
});
for(const slug of ['huskfield','dewcap','shellstone','cinderflint'])test(slug+': connected path, sparse open ground and no opaque prop boxes',()=>{
 const scene=JSON.parse(fs.readFileSync(`${base}/scenes/${slug}/scene.json`));
 const map=scene.terrain.map;assert.equal(map.length,16);assert.ok(map.every(row=>row.length===24));
 const paths=new Set();map.forEach((row,y)=>[...row].forEach((m,x)=>{if(m==='1')paths.add(`${x},${y}`);}));
 const reached=new Set(),queue=[paths.values().next().value];
 for(let i=0;i<queue.length;i++){const key=queue[i];if(reached.has(key))continue;reached.add(key);const [x,y]=key.split(',').map(Number);for(const [nx,ny]of[[x-1,y],[x+1,y],[x,y-1],[x,y+1]]){const n=`${nx},${ny}`;if(paths.has(n)&&!reached.has(n))queue.push(n);}}
 assert.equal(reached.size,paths.size,'all walking ground connects');
 for(const view of scene.views){const [x,y]=view.player;assert.equal(map[(y+8)>>4][(x+8)>>4],'1','player stands on light path');}
 const ground=habitatGround(scene,atlas);
 for(let cy=0;cy<16;cy++)for(let cx=0;cx<24;cx++){
   if(!['0','1'].includes(map[cy][cx]))continue;
   let black=0;for(let y=4;y<12;y++)for(let x=4;x<12;x++)black+=ground.pixels[(((cy*16+y)*384+cx*16+x)*4)]===0;
   assert.ok(black<=6,'open ground has small details rather than blanket dithering');
 }
 const props=read(`${base}/habitats/atlas32.png`);
 const prop=habitatProp(props,scene.terrain.biome*16);
 assert.ok(prop.pixels.some((v,i)=>i%4===3&&v===0),'white exterior is removed');
 assert.ok(prop.pixels.some((v,i)=>i%4===3&&v===255),'object silhouette retained');
 // Inspect a far exterior corner where a white tile box used to erase the floor.
 const placement=scene.placements.find(([id])=>id===81+scene.terrain.biome*16);
 const terrain=read(`${base}/scenes/${slug}/scene-terrain.png`);
 const [,dx,dy]=placement;const pi=prop.pixels.findIndex((v,i)=>i%4===3&&v===0);
 const pixel=(pi-3)/4,x=pixel%32,y=pixel>>5,offset=((dy+y)*384+dx+x)*4;
 assert.deepEqual(terrain.pixels.subarray(offset,offset+4),ground.pixels.subarray(offset,offset+4));
});
