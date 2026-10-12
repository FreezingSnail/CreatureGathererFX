import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {decodePng} from '../creature-sprite-export.mjs';
const root='docs/assets/world-environment-32-spike/combined16',read=file=>decodePng(fs.readFileSync(file));
const manifest=JSON.parse(fs.readFileSync(root+'/manifest.json')),atlas=read(root+'/atlas16.png');
test('joined atlas preserves the refreshed legacy cells and every unscaled32px source quadrant',()=>{
 assert.match(manifest.baseSource,/simple\/legacy16\/atlas16.png$/);
 const old=read(manifest.legacyReference),original=read(manifest.baseSource);assert.notDeepEqual(old.pixels,original.pixels);assert.deepEqual(atlas.pixels.subarray(0,original.pixels.length),original.pixels);
 assert.deepEqual([atlas.width,atlas.height],[256,1664]);assert.deepEqual(manifest.cell,[16,16]);
 assert.equal(manifest.baseTileCount,768);assert.equal(manifest.tileCount,1664);assert.equal(manifest.tiles.length,224);
 const sources=new Map(),ids=new Set();
 for(const t of manifest.tiles){
  if(!sources.has(t.source))sources.set(t.source,read(t.source));const source=sources.get(t.source);
  for(const [q,id] of t.quadrants.entries()){
   assert.ok(id>manifest.baseTileCount&&id<=manifest.tileCount);assert.ok(!ids.has(id));ids.add(id);
   const dx=(id-1)%16*16,dy=Math.floor((id-1)/16)*16,sx=t.sourcePixel[0]+q%2*16,sy=t.sourcePixel[1]+Math.floor(q/2)*16;
   for(let y=0;y<16;y++)for(let x=0;x<16;x++){
    const ai=((dy+y)*atlas.width+dx+x)*4,si=((sy+y)*source.width+sx+x)*4;
    assert.deepEqual(atlas.pixels.subarray(ai,ai+4),source.pixels.subarray(si,si+4));
   }
  }
 }
 assert.equal(ids.size,896);
});
test('hybrid town renders solely from the combined16px atlas and preserves the native player',()=>{
 const scene=JSON.parse(fs.readFileSync(root+'/scene.json')),terrain=read(root+'/scene-terrain.png');
 assert.ok(scene.map.flat().some(id=>id>0&&id<=768));assert.ok(scene.map.flat().some(id=>id>768));
 for(let y=0;y<terrain.height;y++)for(let x=0;x<terrain.width;x++){
  const id=scene.map[y>>4][x>>4],ti=(y*terrain.width+x)*4;
  const ai=(id?((Math.floor((id-1)/16)*16+y%16)*atlas.width+(id-1)%16*16+x%16)*4:0);
  const expected=id?atlas.pixels.subarray(ai,ai+4):new Uint8Array([255,255,255,255]);
  assert.deepEqual(terrain.pixels.subarray(ti,ti+4),expected);
 }
 for(const [index,v] of scene.views.entries()){
  const crop=read(root+`/viewport-${index+1}.png`),player=read(`docs/assets/world-player/${v.facing}.png`);assert.deepEqual([crop.width,crop.height],[128,64]);
  for(let y=0;y<64;y++)for(let x=0;x<128;x++){
   const wx=x+v.camera[0],wy=y+v.camera[1],inside=wx>=v.player[0]&&wx<v.player[0]+16&&wy>=v.player[1]&&wy<v.player[1]+16;
   const pi=((wy-v.player[1])*16+wx-v.player[0])*4,ti=(wy*terrain.width+wx)*4,ci=(y*128+x)*4;
   assert.deepEqual(crop.pixels.subarray(ci,ci+4),inside&&player.pixels[pi+3]?player.pixels.subarray(pi,pi+4):terrain.pixels.subarray(ti,ti+4));
  }
 }
});
test('Tiled map uses the exact combined IDs and source-relative tileset',()=>{
 const scene=JSON.parse(fs.readFileSync(root+'/scene.json')),map=JSON.parse(fs.readFileSync(root+'/town.tmj'));
 assert.equal(map.tilewidth,16);assert.equal(map.tileheight,16);assert.deepEqual(map.layers[0].data,scene.map.flat());
 assert.deepEqual(map.tilesets,[{firstgid:1,source:'tileset.tsx'}]);
 const tsx=fs.readFileSync(root+'/tileset.tsx','utf8');assert.match(tsx,/tilecount="1664" columns="16"/);assert.match(tsx,/source="atlas16.png" width="256" height="1664"/);
});
