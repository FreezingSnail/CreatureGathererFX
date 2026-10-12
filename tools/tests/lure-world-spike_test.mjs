import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {decodePng} from '../creature-sprite-export.mjs';
const root='docs/assets/world-environment-32-spike/lure-world';
const atlas=decodePng(fs.readFileSync('images/LureWorld_16x16.png'));
test('playable lure artwork packs every16px render cell exactly from the128px scene',()=>{
 assert.deepEqual([atlas.width,atlas.height],[128,128]);
 const header=fs.readFileSync('src/fxdata.h','utf8');
 const address=parseInt(header.match(/constexpr uint24_t lureWorldTiles = 0x([0-9A-F]+);/)[1],16);
 const packed=fs.readFileSync('dist/fxdata-data.bin');
 assert.deepEqual([...packed.subarray(address,address+4)],[0,16,0,16]);
 for(let frame=0;frame<64;frame++)for(let page=0;page<2;page++)for(let x=0;x<16;x++){
   let expected=0;for(let bit=0;bit<8;bit++){
    const i=(((Math.floor(frame/8)*16+page*8+bit)*128+frame%8*16+x)*4);
    assert.ok(atlas.pixels[i]===0||atlas.pixels[i]===255);assert.equal(atlas.pixels[i+3],255);
    expected|=(atlas.pixels[i]===255?1:0)<<bit;
   }
   assert.equal(packed[address+4+frame*32+page*16+x],expected,`frame${frame},page${page},column${x}`);
 }
});
test('pocket mechanics and docs presets retain box, plant, exit and native player position',()=>{
 const map=JSON.parse(fs.readFileSync('maps/world_map.json'));
 assert.equal(map.tiles[2][9],545);assert.equal(map.tiles[2][14],551);assert.equal(map.tiles[7][12],275);
 const manifest=JSON.parse(fs.readFileSync(`${root}/manifest.json`));
 assert.deepEqual(manifest.worldOrigin,[8,0]);assert.equal(manifest.frames,64);
 for(const v of manifest.views){const image=decodePng(fs.readFileSync(`${root}/${v.name}.png`));assert.deepEqual([image.width,image.height],[128,64]);assert.deepEqual(v.playerScreen,[56,24]);}
 const scene=decodePng(fs.readFileSync(`${root}/terrain.png`));assert.deepEqual(scene.pixels,atlas.pixels);
});
test('lure interaction fixtures retain quiet highlights and native16px silhouettes',()=>{
 const props=decodePng(fs.readFileSync('docs/assets/world-environment-32-spike/habitats/lure-clarity/atlas32.png'));
 for(const id of [8,9]){
  let black=0,insideWhite=0;
  for(let y=0;y<32;y++)for(let x=0;x<32;x++){
   const value=props.pixels[((Math.floor(id/4)*32+y)*128+id%4*32+x)*4];
   if(value===0){black++;assert.ok(x>=8&&x<24&&y>=8&&y<24,'fixture ink fits one16px interaction cell');}
   else if(x>=8&&x<24&&y>=8&&y<24)insideWhite++;
  }
  assert.ok(black>=20&&black<150,'fixture retains its shape without becoming a solid dark blob');
  assert.ok(insideWhite>100,'fixture has quiet white highlights');
 }
});
