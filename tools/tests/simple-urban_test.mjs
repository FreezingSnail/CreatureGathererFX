import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {decodePng} from '../creature-sprite-export.mjs';
const base='docs/assets/world-environment-32-spike',read=p=>decodePng(fs.readFileSync(p));
test('urban and legacy redraw exports are opaque binary assets of the intended size',()=>{
 for(const [folder,cell,w,h] of [['legacy16',16,256,768],['urban',32,256,256]]){
  const image=read(`${base}/simple/${folder}/atlas${cell}.png`);assert.deepEqual([image.width,image.height],[w,h]);
  for(let i=0;i<image.pixels.length;i+=4){const v=image.pixels[i];assert.ok(v===0||v===255);assert.deepEqual([...image.pixels.subarray(i,i+4)],[v,v,v,255]);}
 }
});
for(const slug of ['market-street','residential-block'])test(`${slug}: atlas-only scenery, Tiled IDs and original player in every native crop`,()=>{
 const root=base+'/simple/urban/'+slug,scene=JSON.parse(fs.readFileSync(root+'/scene.json')),terrain=read(root+'/scene-terrain.png'),atlas=read(path.resolve(root,scene.atlas));
 assert.deepEqual([terrain.width,terrain.height],[384,256]);
 const map=JSON.parse(fs.readFileSync(root+'/map.tmj'));assert.deepEqual(map.layers[0].data,scene.map.flat());assert.equal(map.tilewidth,16);
 assert.ok(fs.existsSync(path.resolve(root,map.tilesets[0].source)));
 for(let y=0;y<256;y++)for(let x=0;x<384;x++){
  const id=scene.map[y>>4][x>>4]-1;assert.ok(id>=1408&&id<1664);
  const a=((Math.floor(id/16)*16+y%16)*256+id%16*16+x%16)*4,t=(y*384+x)*4;
  assert.deepEqual(terrain.pixels.subarray(t,t+4),atlas.pixels.subarray(a,a+4));
 }
 for(const [index,v] of scene.views.entries()){
  const crop=read(root+`/viewport-${index+1}.png`),player=read(`docs/assets/world-player/${v.facing}.png`);assert.deepEqual([crop.width,crop.height],[128,64]);
  for(let y=0;y<64;y++)for(let x=0;x<128;x++){
   const wx=x+v.camera[0],wy=y+v.camera[1],inside=wx>=v.player[0]&&wx<v.player[0]+16&&wy>=v.player[1]&&wy<v.player[1]+16;
   const pi=((wy-v.player[1])*16+wx-v.player[0])*4,ti=(wy*384+wx)*4,ci=(y*128+x)*4;
   assert.deepEqual(crop.pixels.subarray(ci,ci+4),inside&&player.pixels[pi+3]?player.pixels.subarray(pi,pi+4):terrain.pixels.subarray(ti,ti+4));
  }
 }
});
