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
console.log(`PASS: ${baseCount} original cells +${count*4} unscaled scenery cells =${atlas.width*atlas.height/256} native16px cells; hybrid town and3 device crops`);
