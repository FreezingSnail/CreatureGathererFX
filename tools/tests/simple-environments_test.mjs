import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import {decodePng} from '../creature-sprite-export.mjs';
const base='docs/assets/world-environment-32-spike',simple=base+'/simple';
const slugs=['river','town','city','cave','cave-dark','huskfield','dewcap','shellstone','cinderflint'];
const read=file=>decodePng(fs.readFileSync(file));
for(const slug of slugs){
 const folder=slug==='river'?'':'/scenes/'+slug,root=simple+folder;
 const original=JSON.parse(fs.readFileSync(base+folder+'/scene.json')),scene=JSON.parse(fs.readFileSync(root+'/scene.json'));
 test(slug+': simple variant preserves layouts, native cameras and original player pixels',()=>{
  assert.deepEqual(scene.placements,original.placements);assert.deepEqual(scene.views,original.views);assert.deepEqual(scene.size,original.size);
  assert.deepEqual(scene.creatures,original.creatures);
  const terrain=read(root+'/scene-terrain.png');assert.deepEqual([terrain.width,terrain.height],scene.size);
  for(const [index,v] of scene.views.entries()){
   const crop=read(root+`/viewport-${index+1}.png`),player=read(`docs/assets/world-player/${v.facing}.png`);
   assert.deepEqual([crop.width,crop.height],[128,64]);
   for(let y=0;y<64;y++)for(let x=0;x<128;x++){
    const wx=x+v.camera[0],wy=y+v.camera[1],inside=wx>=v.player[0]&&wx<v.player[0]+16&&wy>=v.player[1]&&wy<v.player[1]+16;
    const pi=((wy-v.player[1])*16+wx-v.player[0])*4,ti=(wy*terrain.width+wx)*4;
    const source=inside&&player.pixels[pi+3]?player.pixels.subarray(pi,pi+4):terrain.pixels.subarray(ti,ti+4);
    const ci=(y*128+x)*4;assert.deepEqual(crop.pixels.subarray(ci,ci+4),source);
    assert.ok(crop.pixels[ci]===0||crop.pixels[ci]===255);assert.equal(crop.pixels[ci+3],255);
   }
  }
  const html=fs.readFileSync('docs/world-environment-32-spike.html','utf8');
  assert.ok(html.includes(`id="simple-${slug}"`));
 });
}
test('simple atlases use fewer black pixels than dithered sources and retain the same grid',()=>{
 for(const folder of ['','/expansion','/habitats','/habitats/terrain']){
  const old=JSON.parse(fs.readFileSync(base+folder+'/manifest.json')),manifest=JSON.parse(fs.readFileSync(simple+folder+'/manifest.json'));
  assert.deepEqual(manifest.sheet,old.sheet);assert.deepEqual(manifest.cell,[32,32]);assert.equal(manifest.tiles,old.tiles);
  assert.ok(manifest.blackPixels<old.blackPixels);assert.match(manifest.export,/no added dithering/);
  const atlas=read(simple+folder+'/atlas32.png');for(let i=0;i<atlas.pixels.length;i+=4){assert.ok(atlas.pixels[i]===0||atlas.pixels[i]===255);assert.equal(atlas.pixels[i+3],255);}
  assert.ok(fs.existsSync(simple+folder+'/concept.png'));assert.ok(fs.existsSync(simple+folder+'/prompt.json'));
 }
});
test('dark cave flips simple terrain only and style comparison preserves every native crop',()=>{
 const light=read(simple+'/scenes/cave/scene-terrain.png'),dark=read(simple+'/scenes/cave-dark/scene-terrain.png');
 for(let i=0;i<light.pixels.length;i+=4)assert.equal(dark.pixels[i],255-light.pixels[i]);
 const manifest=JSON.parse(fs.readFileSync(simple+'/comparison.json')),sheet=read(simple+'/comparison-native.png');
 assert.equal(manifest.cells.length,18);
 for(const c of manifest.cells){const crop=read(c.source);for(let y=0;y<64;y++)for(let x=0;x<128;x++){
  const si=((c.y+y)*sheet.width+c.x+x)*4,ci=(y*128+x)*4;assert.deepEqual(sheet.pixels.subarray(si,si+4),crop.pixels.subarray(ci,ci+4));
 }}
});
