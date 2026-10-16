#!/usr/bin/env node
// Lossless atlas packing: every source pixel remains at its authored size.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import {decodePng,encodePng} from './creature-sprite-export.mjs';
const base='docs/assets/world-environment-32-spike',root=base+'/combined16';
const baseSource=base+'/simple/legacy16/atlas16.png';
const read=file=>decodePng(fs.readFileSync(file));
const original=read(baseSource),columns=original.width/16,baseCount=original.width*original.height/256;
assert.equal(columns,16);assert.equal(original.height%32,0);
const sources=[['world',''],['village','expansion/'],['habitats','habitats/'],['terrain','habitats/terrain/'],['urban','urban/']].map(([name,folder])=>({name,path:base+'/simple/'+folder+'atlas32.png'}));
const groups=sources.map(s=>({...s,image:read(s.path)}));
const count=groups.reduce((n,s)=>n+s.image.width*s.image.height/1024,0);
assert.equal(count,224);
const atlas={width:original.width,height:original.height+Math.ceil(count/8)*32,pixels:new Uint8Array(original.width*(original.height+Math.ceil(count/8)*32)*4).fill(255)};
atlas.pixels.set(original.pixels);
function paste(target,source,dx,dy,sx=0,sy=0,w=source.width,h=source.height){
 for(let y=0;y<h;y++)for(let x=0;x<w;x++){
  const si=((sy+y)*source.width+sx+x)*4,di=((dy+y)*target.width+dx+x)*4;
  if(source.pixels[si+3])target.pixels.set(source.pixels.subarray(si,si+4),di);
 }
}
function enlarge(source,scale){
 const image={width:source.width*scale,height:source.height*scale,pixels:new Uint8Array(source.pixels.length*scale*scale)};
 for(let y=0;y<image.height;y++)for(let x=0;x<image.width;x++){
  const si=(Math.floor(y/scale)*source.width+Math.floor(x/scale))*4;
  image.pixels.set(source.pixels.subarray(si,si+4),(y*image.width+x)*4);
 }
 return image;
}
const tiles=[],sections=[];let packed=0;
for(const group of groups){
 const firstId=baseCount+packed*4+1,firstRow=original.height+Math.floor(packed/8)*32;
 for(let index=0;index<group.image.width*group.image.height/1024;index++){
  const sx=index%(group.image.width/32)*32,sy=Math.floor(index/(group.image.width/32))*32;
  const x=packed%8*32,y=original.height+Math.floor(packed/8)*32;
  paste(atlas,group.image,x,y,sx,sy,32,32);
  const tl=y/16*columns+x/16+1;
  tiles.push({source:group.path,group:group.name,sourceTile:index+1,sceneTileId:packed+1,sourcePixel:[sx,sy],atlasPixel:[x,y],quadrants:[tl,tl+1,tl+columns,tl+columns+1]});packed++;
 }
 sections.push({name:group.name,source:group.path,firstId,lastId:baseCount+packed*4,atlasY:firstRow});
}
fs.mkdirSync(root,{recursive:true});
fs.writeFileSync(root+'/atlas16.png',encodePng(atlas));
fs.writeFileSync(root+'/preview2x.png',encodePng(enlarge(atlas,2)));
fs.writeFileSync(root+'/manifest.json',JSON.stringify({reviewOnly:true,baseSource,baseSize:[original.width,original.height],baseTileCount:baseCount,cell:[16,16],sheet:[atlas.width,atlas.height],columns,tileCount:atlas.width*atlas.height/256,appended32pxTiles:count,appended16pxCells:count*4,ids:'One-based native tile IDs. Quadrant order: top-left,top-right,bottom-left,bottom-right.',artDirection:'Approved simple nondithered environments; legacy v4 redrawn to match.',legacyReference:'docs/assets/world-tiles/tiles-bw-redraw-v4-native.png',packing:'Redrawn16px sheet followed by unscaled32px assets split into four16px cells.',sections,tiles},null,2)+'\n');
fs.writeFileSync(root+'/tileset.tsx',`<?xml version="1.0" encoding="UTF-8"?>\n<tileset version="1.10" tiledversion="1.11.2" name="simple-combined16" tilewidth="16" tileheight="16" tilecount="${atlas.width*atlas.height/256}" columns="${columns}"><image source="atlas16.png" width="${atlas.width}" height="${atlas.height}"/></tileset>\n`);
// Hybrid town: the existing native16px house joins the simple32px village.
const sourceScene=JSON.parse(fs.readFileSync(base+'/simple/scenes/town/scene.json'));
const width=384,height=256,map=Array.from({length:height/16},()=>Array(width/16).fill(0));
function put(id,x,y){assert.ok(id>0&&id<=atlas.width*atlas.height/256);map[y/16][x/16]=id;}
for(const [id,x,y] of sourceScene.placements){
 if((id===73&&x===144&&y===64)||(id===74&&x===176&&y===64)||(id===66&&x===160&&y===96))continue;
 const q=tiles[id-1].quadrants;put(q[0],x,y);put(q[1],x+16,y);put(q[2],x,y+16);put(q[3],x+16,y+16);
}
for(let y=0;y<4;y++)for(let x=0;x<4;x++)put((12+y)*16+x+1,144+x*16,64+y*16);
const terrain={width,height,pixels:new Uint8Array(width*height*4).fill(255)};
for(let y=0;y<map.length;y++)for(let x=0;x<map[y].length;x++)if(map[y][x]){
 const index=map[y][x]-1;paste(terrain,atlas,x*16,y*16,index%columns*16,Math.floor(index/columns)*16,16,16);
}
fs.writeFileSync(root+'/scene-terrain.png',encodePng(terrain));
const views=[{name:'Redrawn16px house',player:[184,128],facing:'up',camera:[128,80]},sourceScene.views[0],sourceScene.views[2]];
for(const [index,view] of views.entries()){
 const full={...terrain,pixels:terrain.pixels.slice()},player=read(`docs/assets/world-player/${view.facing}.png`);
 paste(full,player,...view.player);
 if(!index){fs.writeFileSync(root+'/scene.png',encodePng(full));fs.writeFileSync(root+'/scene-preview2x.png',encodePng(enlarge(full,2)));}
 const crop={width:128,height:64,pixels:new Uint8Array(32768)};paste(crop,full,0,0,...view.camera,128,64);
 fs.writeFileSync(root+`/viewport-${index+1}.png`,encodePng(crop));fs.writeFileSync(root+`/viewport-${index+1}-2x.png`,encodePng(enlarge(crop,2)));
}
fs.writeFileSync(root+'/scene.json',JSON.stringify({name:'Combined16px town',size:[width,height],tileSize:16,map,views,baseSource},null,2)+'\n');
fs.writeFileSync(root+'/town.tmj',JSON.stringify({type:'map',version:'1.10',tiledversion:'1.11.2',orientation:'orthogonal',renderorder:'right-down',width:width/16,height:height/16,tilewidth:16,tileheight:16,backgroundcolor:'#ffffff',infinite:false,layers:[{id:1,name:'Joined scenery',type:'tilelayer',x:0,y:0,width:width/16,height:height/16,visible:true,opacity:1,data:map.flat()}],tilesets:[{firstgid:1,source:'tileset.tsx'}]},null,2)+'\n');
const prefix='assets/world-environment-32-spike/combined16/';
const gallery=`<section id="combined16"><h2>Combined16×16 tile sheet</h2><p>The older environment-clarity artwork has been redrawn to match the approved simple environments. This is the current art direction: clear black contours, quiet white ground and sparse details. The ${baseCount}-cell redraw joins the world, village, habitats, terrain and new urban set. Each32px asset occupies four16px cells, retaining its full size.</p><p><a href="${prefix}atlas16.png">Native atlas ·${atlas.width}×${atlas.height}</a> · <a href="${prefix}tileset.tsx">Tiled tileset</a> · <a href="${prefix}manifest.json">Source/quadrant mapping</a> · <a href="${prefix}town.tmj">Editable hybrid town</a></p><div class="controls"><label>Sheet zoom <select id="combined16-zoom"><option value="1">1×</option><option value="2" selected>2×</option><option value="4">4×</option></select></label><label><input id="combined16-bounds" type="checkbox">16×16 grid</label></div><div class="scroll"><div id="combined16-sheet" style="position:relative;display:inline-block;line-height:0"><img id="combined16-atlas" src="${prefix}atlas16.png" width="${atlas.width*2}" height="${atlas.height*2}" alt="Unified simple16px environment and urban tiles"><div id="combined16-grid" style="position:absolute;inset:0;pointer-events:none;display:none;background-image:linear-gradient(to right,#e4525280 1px,transparent 1px),linear-gradient(to bottom,#e4525280 1px,transparent 1px)"></div></div></div><p>${sections.map(s=>`${s.name}: cells${s.firstId}–${s.lastId}`).join(' · ')}</p><h3>Joined town example</h3><img class="example-map" src="${prefix}scene.png" width="384" height="256" alt="Native16px house joined with simple32px scenery"><div class="example-crops">${views.map((v,i)=>`<figure><figcaption>${v.name}</figcaption><p>1× ·128×64</p><div class="device-frame"><img class="native-crop" src="${prefix}viewport-${i+1}.png" width="128" height="64" alt="${v.name} native view"></div><p>2× inspection</p><img class="large-crop" src="${prefix}viewport-${i+1}.png" width="256" height="128" alt="${v.name} enlarged"></figure>`).join('')}</div></section>`;
const page='docs/world-environment-32-spike.html';let html=fs.readFileSync(page,'utf8');
if(!html.includes('<!-- combined16-start -->'))html=html.replace('<!-- simple-examples-start -->','<!-- combined16-start -->\n<!-- combined16-end -->\n<!-- simple-examples-start -->');
html=html.replace(/<!-- combined16-start -->[\s\S]*?<!-- combined16-end -->/,'<!-- combined16-start -->\n'+gallery+'\n<!-- combined16-end -->');fs.writeFileSync(page,html);

// ---------------------------------------------------------------------------
// Shipping install: write the approved simple redraw of the selected base
// catalogue cells into the ordinary game sheets at their existing GIDs. This is
// the only writer of images/tiles_16x16.png and images/tiles.png (or the Tiled
// source); every other cell is copied through byte-for-byte. The review
// catalogue above stays review-only; no appended32px cell is installed.
// ---------------------------------------------------------------------------
const shippingMapPath=base+'/simple/shipping-map.json';
const windowDX=[-4,5],windowDY=[-3,2]; // readStartX(-3 or-4)..+8, readStartY(-2 or-3)..+4
// The single authored border cell at(4,4), west of the area. It is not inside the
// exact render-window union but sits in the same authored wall family one step
// beyond it; installing it removes any future legacy/simple seam there.
const borderExtras=[38];
const readPixels=file=>decodePng(fs.readFileSync(file));
function write(file,buffer){if(!fs.existsSync(file)||!fs.readFileSync(file).equals(buffer))fs.writeFileSync(file,buffer);}
function areaBounds(){
  const text=fs.readFileSync('data/beta-area.toml','utf8');
  return Object.fromEntries(['min_x','max_x','min_y','max_y','start_x','start_y'].map(key=>{
    const m=text.match(new RegExp('^\\s*'+key+'\\s*=\\s*(-?\\d+)\\s*$','m'));
    assert.ok(m,`data/beta-area.toml is missing ${key}`);
    return [key,Number(m[1])];
  }));
}
const area=areaBounds();
const collisionPath='fxdata/generated/raw_map.bin';
assert.ok(fs.existsSync(collisionPath),'run `make gen` before the shipping install');
const collision=fs.readFileSync(collisionPath);
assert.equal(collision.length,256*256*2,'raw_map.bin must hold256x256 property words');
const wordAt=(x,y)=>collision.readUInt16LE((y*256+x)*2);
const gidAt=(x,y)=>wordAt(x,y)&0x03ff;
const walkable=(x,y)=>x>=0&&y>=0&&x<256&&y<256&&gidAt(x,y)!==0&&((wordAt(x,y)>>10&0x3f)&1)!==0;
const startX=area.start_x,startY=area.start_y;
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
const betaGids=new Set(),visibleGids=new Set();
for(let y=area.min_y;y<=area.max_y;y++)for(let x=area.min_x;x<=area.max_x;x++){
  const g=gidAt(x,y);if(g)betaGids.add(g);
}
for(const key of reachable){
  const [x,y]=key.split(',').map(Number);
  for(let dx=windowDX[0];dx<=windowDX[1];dx++)for(let dy=windowDY[0];dy<=windowDY[1];dy++){
    const cx=x+dx,cy=y+dy;
    if(cx<0||cy<0||cx>=256||cy>=256)continue;
    const g=gidAt(cx,cy);if(g)visibleGids.add(g);
  }
}
const entries=[];
for(const g of [...betaGids].sort((a,b)=>a-b)){
  entries.push({cell:g,shippingGid:g,visibility:'beta-area',reason:`non-empty GID inside the first-beta area x${area.min_x}..${area.max_x},y${area.min_y}..${area.max_y}`});
}
for(const g of [...visibleGids].filter(g=>!betaGids.has(g)).sort((a,b)=>a-b)){
  entries.push({cell:g,shippingGid:g,visibility:'visible-margin',reason:'non-empty GID inside the render window of a reachable beta cell'});
}
for(const g of borderExtras){
  assert.ok(!betaGids.has(g)&&!visibleGids.has(g),`border extra ${g} is already covered`);
  entries.push({cell:g,shippingGid:g,visibility:'adjacent-border',reason:'authored border cell in the visible-neighbourhood wall family west of the area'});
}
const shippingGids=new Set();
for(const entry of entries){
  assert.ok(entry.cell>=1&&entry.cell<=baseCount,`mapping cell ${entry.cell} is inside the base catalogue`);
  assert.ok(entry.shippingGid>=1&&entry.shippingGid<=1023,`shipping GID ${entry.shippingGid} is a10-bit tile id`);
  assert.ok(!shippingGids.has(entry.shippingGid),`shipping GID ${entry.shippingGid} is mapped twice`);
  shippingGids.add(entry.shippingGid);
}
const shippingMapping={reviewOnly:false,catalogueSource:baseSource,cell:[16,16],sheet:[original.width,original.height],columns,installed:entries};
write(shippingMapPath,Buffer.from(JSON.stringify(shippingMapping,null,2)+'\n'));
// Native1bpp source convention (CreatureGathererFX-b2a.8). The packed shipping
// sheet is already authored monochrome; the Tiled source sheet still carries the
// legacy four-gray palette in its untouched cells. Collapse every pixel to the
// project convention, binary 0/255 RGB with 0/255 alpha (opaque => luminance
// threshold at128; transparent stays transparent black). Installed cells are
// already 0/255 opaque so they remain byte-exact. This writes no new pixel value
// that the packer would decode differently; it only removes the unused grays.
const BINARY_SHEETS=new Set(['images/tiles.png']);
function normalizeBinary(image){
  for(let i=0;i<image.pixels.length;i+=4){
    const opaque=image.pixels[i+3]>=128,value=opaque&&image.pixels[i]>=128?255:0;
    image.pixels[i]=value;image.pixels[i+1]=value;image.pixels[i+2]=value;image.pixels[i+3]=opaque?255:0;
  }
}
for(const path of ['images/tiles_16x16.png','images/tiles.png']){
  const sheet=readPixels(path);
  assert.deepEqual([sheet.width,sheet.height],[original.width,original.height],`${path} must stay ${original.width}x${original.height}`);
  for(const {cell} of entries){
    const index=cell-1,col=index%columns,row=Math.floor(index/columns);
    for(let y=0;y<16;y++)for(let x=0;x<16;x++){
      const si=((row*16+y)*original.width+col*16+x)*4;
      sheet.pixels.set(original.pixels.subarray(si,si+4),((row*16+y)*original.width+col*16+x)*4);
    }
  }
  if(BINARY_SHEETS.has(path))normalizeBinary(sheet);
  write(path,encodePng(sheet));
}
const tiledSheet=readPixels('images/tiles.png');
for(let i=0;i<tiledSheet.pixels.length;i+=4){
  const [r,g,b,a]=tiledSheet.pixels.subarray(i,i+4);
  assert.ok((a===0||a===255)&&(r===0||r===255)&&((a===0&&r===0&&g===0&&b===0)||(a===255&&r===g&&g===b)),'images/tiles.png uses binary0/255 RGB and0/255 alpha');
}
console.log(`PASS: ${baseCount} original cells +${count*4} unscaled scenery cells =${atlas.width*atlas.height/256} native16px cells; hybrid town and3 device crops`);
console.log(`PASS: installed${entries.length} simple cells (${betaGids.size} beta +${visibleGids.size-betaGids.size} visible margin +${borderExtras.length} adjacent border) into the shipping tile sheets; mapping${shippingMapPath}`);
