import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import {decodePng} from '../creature-sprite-export.mjs';
const root='docs/assets/world-environment-32-spike/combined16',read=file=>decodePng(fs.readFileSync(file));
const ROOT=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..','..');
const showHead=rel=>execFileSync('git',['show',`HEAD:${rel}`],{cwd:ROOT,maxBuffer:64*1024*1024});

// The shipping install is the only writer of these sheets; these constants
// mirror tools/join-simple-environments16.mjs exactly.
const SHIPPING_SHEETS=['images/tiles_16x16.png','images/tiles.png'];
// b2a.8: the Tiled source sheet is normalized to the native binary authoring
// convention (0/255 RGB with 0/255 alpha); the packed sheet is already binary.
// The refreshed expectation for an untouched Tiled-source cell is HEAD collapsed
// the same way the joiner collapses its legacy grays.
const BINARY_SHEETS=new Set(['images/tiles.png']);
const binaryPixel=(r,g,b,a)=>{const opaque=a>=128,value=opaque&&r>=128?255:0;return new Uint8Array([value,value,value,opaque?255:0]);};
const WINDOW_DX=[-4,5],WINDOW_DY=[-3,2];       // render window reach around one player cell
const BORDER_EXTRAS=[38];                       // authored border cell west of the area
const TILE_ID_LIMIT=1023;                       // 10-bit packed tile id
function areaBounds(){
  const text=fs.readFileSync('data/beta-area.toml','utf8'),area={};
  for(const key of ['min_x','max_x','min_y','max_y','start_x','start_y']){
    const m=text.match(new RegExp('^\\s*'+key+'\\s*=\\s*(-?\\d+)\\s*$','m'));
    assert.ok(m,`data/beta-area.toml is missing ${key}`);area[key]=Number(m[1]);
  }
  return area;
}
function derivedVisibility(){
  const collision=fs.readFileSync('fxdata/generated/raw_map.bin');
  assert.equal(collision.length,256*256*2,'raw_map.bin must hold256x256 property words; run `make gen`');
  const wordAt=(x,y)=>collision.readUInt16LE((y*256+x)*2);
  const gidAt=(x,y)=>(x<0||y<0||x>=256||y>=256)?0:(wordAt(x,y)&0x03ff);
  const walkable=(x,y)=>gidAt(x,y)!==0&&((wordAt(x,y)>>10&0x3f)&1)!==0;
  const area=areaBounds(),startX=area.start_x,startY=area.start_y;
  assert.ok(walkable(startX,startY),'beta start cell must be walkable');
  const reachable=new Set([startX+','+startY]),queue=[[startX,startY]];
  while(queue.length){
    const [x,y]=queue.shift();
    for(const [dx,dy] of [[1,0],[-1,0],[0,1],[0,-1]]){
      const nx=x+dx,ny=y+dy,key=nx+','+ny;
      if(reachable.has(key)||!walkable(nx,ny))continue;
      reachable.add(key);queue.push([nx,ny]);
    }
  }
  const beta=new Set(),visible=new Set();
  for(let y=area.min_y;y<=area.max_y;y++)for(let x=area.min_x;x<=area.max_x;x++){
    const g=gidAt(x,y);if(g)beta.add(g);
  }
  for(const key of reachable){
    const [x,y]=key.split(',').map(Number);
    for(let dx=WINDOW_DX[0];dx<=WINDOW_DX[1];dx++)for(let dy=WINDOW_DY[0];dy<=WINDOW_DY[1];dy++){
      const g=gidAt(x+dx,y+dy);if(g)visible.add(g);
    }
  }
  return {area,beta,visible};
}
function tsxProperties(text){
  const map=new Map();
  for(const match of text.matchAll(/<tile id="(\d+)">([\s\S]*?)<\/tile>/g)){
    const props=[...match[2].matchAll(/<property name="([^"]+)"[^>]*value="([^"]*)"/g)]
      .map(p=>`${p[1]}=${p[2]}`).sort();
    map.set(Number(match[1]),props.join(';'));
  }
  return map;
}
function cellPixels(image,gid){
  const index=gid-1,col=index%16,row=Math.floor(index/16);
  return {col,row,at:(x,y)=>((row*16+y)*image.width+col*16+x)*4};
}
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
// ---------------------------------------------------------------------------
// Shipping install (CreatureGathererFX-cf7). The approved simple redraw of the
// selected base cells is written at the same GIDs into the ordinary game
// sheets; the mapping, the byte-exact copies and the packed cart bytes are
// validated here.
// ---------------------------------------------------------------------------
const shippingMap=JSON.parse(fs.readFileSync('docs/assets/world-environment-32-spike/simple/shipping-map.json'));
const installedByGid=new Map(shippingMap.installed.map(e=>[e.shippingGid,e]));

test('shipping map covers every beta and visible-margin GID with distinct10-bit identities',()=>{
 const {beta,visible}=derivedVisibility();
 assert.match(shippingMap.catalogueSource,/simple\/legacy16\/atlas16\.png$/);
 assert.notEqual(shippingMap.reviewOnly,true,'the shipping map is not review-only');
 assert.deepEqual(shippingMap.cell,[16,16]);assert.deepEqual(shippingMap.sheet,[256,768]);assert.equal(shippingMap.columns,16);
 const gids=new Set();const allowedClasses=new Set(['beta-area','visible-margin','adjacent-border']);
 for(const entry of shippingMap.installed){
  assert.equal(entry.cell,entry.shippingGid,'catalogue cell id is id-aligned with the shipping GID');
  assert.ok(Number.isInteger(entry.shippingGid)&&entry.shippingGid>=1&&entry.shippingGid<=TILE_ID_LIMIT,`GID ${entry.shippingGid} fits the10-bit tile id`);
  assert.ok(!gids.has(entry.shippingGid),`GID ${entry.shippingGid} is mapped once`);gids.add(entry.shippingGid);
  assert.ok(allowedClasses.has(entry.visibility),`GID ${entry.shippingGid} has a documented visibility class`);
  assert.ok(typeof entry.reason==='string'&&entry.reason.length>0,`GID ${entry.shippingGid} carries a reason`);
 }
 for(const g of beta)assert.ok(installedByGid.has(g),`beta GID ${g} is installed`);
 for(const g of visible)assert.ok(installedByGid.has(g),`visible-margin GID ${g} is installed`);
 for(const g of BORDER_EXTRAS)assert.equal(installedByGid.get(g)?.visibility,'adjacent-border',`border GID ${g} is an adjacent-border entry`);
 const extras=[...gids].filter(g=>!beta.has(g)&&!visible.has(g));
 assert.deepEqual(extras.sort((a,b)=>a-b),[...BORDER_EXTRAS].sort((a,b)=>a-b),'the only non-visible installed cells are the documented border extras');
});

test('shipping sheets install byte-exact monochrome copies, normalize the Tiled source, and leave every other cell at HEAD',()=>{
 const catalogue=read(shippingMap.catalogueSource);
 assert.deepEqual([catalogue.width,catalogue.height],[256,768]);
 const headSheets=new Map(SHIPPING_SHEETS.map(rel=>[rel,decodePng(showHead(rel))]));
 for(const rel of SHIPPING_SHEETS){
  const sheet=read(rel),head=headSheets.get(rel);
  assert.deepEqual([sheet.width,sheet.height],[catalogue.width,catalogue.height],`${rel} stays256x768`);
  assert.deepEqual([head.width,head.height],[catalogue.width,catalogue.height],`HEAD ${rel} is256x768`);
  for(let index=0;index<256*768/256;index++){
   const gid=index+1,col=index%16,row=Math.floor(index/16);
   for(let y=0;y<16;y++)for(let x=0;x<16;x++){
    const i=((row*16+y)*256+col*16+x)*4;
    if(installedByGid.has(gid)){
     assert.deepEqual(sheet.pixels.subarray(i,i+4),catalogue.pixels.subarray(i,i+4),`${rel} GID ${gid} is a byte-exact catalogue copy`);
     const [r,g,b,a]=sheet.pixels.subarray(i,i+4);
     assert.ok((r===0||r===255)&&r===g&&g===b&&a===255,`${rel} GID ${gid} is monochrome opaque`);
    }else if(BINARY_SHEETS.has(rel)){
     const [r,g,b,a]=head.pixels.subarray(i,i+4);
     assert.deepEqual(sheet.pixels.subarray(i,i+4),binaryPixel(r,g,b,a),`${rel} normalized GID ${gid} matches the binary refresh of HEAD`);
    }else{
     assert.deepEqual(sheet.pixels.subarray(i,i+4),head.pixels.subarray(i,i+4),`${rel} untouched GID ${gid} equals HEAD`);
    }
   }
  }
 }
});

test('used tile properties in maps/tiles.tsx are unchanged from HEAD',()=>{
 const head=tsxProperties(showHead('maps/tiles.tsx').toString('utf8'));
 const current=tsxProperties(fs.readFileSync('maps/tiles.tsx','utf8'));
 for(const gid of installedByGid.keys()){
  assert.equal(current.get(gid-1),head.get(gid-1),`tiles.tsx properties for GID ${gid} are unchanged`);
 }
});

test('packed cart tiles symbol frames match the installed sheet bytes',()=>{
 const header=fs.readFileSync('src/fxdata.h','utf8');
 const match=header.match(/constexpr uint24_t tiles = 0x([0-9A-F]+);/);
 assert.ok(match,'src/fxdata.h declares the tiles symbol');
 const address=parseInt(match[1],16);
 const packed=fs.readFileSync('dist/fxdata-data.bin');
 const sheet=read('images/tiles_16x16.png');
 for(const gid of [...installedByGid.keys()].sort((a,b)=>a-b)){
  const frame=gid-1,cell=cellPixels(sheet,gid);
  const base=address+frame*32;
  assert.ok(base+32<=packed.length,`tiles symbol covers frame ${frame}`);
  for(let page=0;page<2;page++)for(let x=0;x<16;x++){
   let expected=0;
   for(let bit=0;bit<8;bit++){
    const i=cell.at(x,page*8+bit);
    assert.ok(sheet.pixels[i]===0||sheet.pixels[i]===255,`GID ${gid} pixel is monochrome`);
    expected|=(sheet.pixels[i]===255?1:0)<<bit;
   }
   assert.equal(packed[base+page*16+x],expected,`tiles frame${frame} page${page} col${x} (GID ${gid})`);
  }
 }
});
