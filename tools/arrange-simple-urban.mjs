#!/usr/bin/env node
import fs from 'node:fs';
import {decodePng,encodePng} from './creature-sprite-export.mjs';
const base='docs/assets/world-environment-32-spike',root=base+'/simple/urban';
const atlas=decodePng(fs.readFileSync(base+'/combined16/atlas16.png'));
const manifest=JSON.parse(fs.readFileSync(base+'/combined16/manifest.json'));
const tiles=manifest.tiles.filter(t=>t.group==='urban');
const blank=(w,h)=>({width:w,height:h,pixels:new Uint8Array(w*h*4).fill(255)});
function paste(dst,src,dx,dy,sx=0,sy=0,w=src.width,h=src.height){
 for(let y=0;y<h;y++)for(let x=0;x<w;x++){
  const i=((sy+y)*src.width+sx+x)*4;
  if(src.pixels[i+3])dst.pixels.set(src.pixels.subarray(i,i+4),((dy+y)*dst.width+dx+x)*4);
 }
}
const definitions=[
 {slug:'market-street',name:'Market Street',buildings:[[0,32,16],[1,128,16],[2,224,16],[4,32,176],[5,128,176],[6,224,176]],props:[[48,16,80],[49,80,80],[50,144,80],[52,240,80],[56,320,16],[57,320,48],[56,320,176]],views:[{name:'Grocery walk',player:[72,88],camera:[16,48],facing:'up'},{name:'Clinic crossing',player:[232,120],camera:[176,88],facing:'right'},{name:'Cafe lane',player:[168,152],camera:[112,128],facing:'down'}]},
 {slug:'residential-block',name:'Residential Block',buildings:[[7,32,16],[1,128,16],[7,224,16],[3,32,176],[7,128,176],[4,224,176]],props:[[56,16,80],[58,80,80],[56,112,80],[59,176,80],[48,224,80],[51,304,176],[62,320,16],[57,320,48],[57,320,176]],views:[{name:'Townhouse stoop',player:[56,88],camera:[0,48],facing:'up'},{name:'Apartment lane',player:[152,88],camera:[96,48],facing:'up'},{name:'Civic square',player:[56,152],camera:[0,128],facing:'down'}]}
];
for(const def of definitions){
 const map=Array.from({length:16},()=>Array(24).fill(0));
 function tile(index,x,y){
  const q=tiles[index].quadrants;
  for(let yy=0;yy<2;yy++)for(let xx=0;xx<2;xx++)map[y/16+yy][x/16+xx]=q[yy*2+xx];
 }
 // White paving with sparse slabs; a continuous street separates two blocks.
 for(let y=0;y<256;y+=32)for(let x=0;x<384;x+=32)tile(0,x,y);
 for(const y of [80,144])for(let x=0;x<384;x+=32)tile(2,x,y);
 for(let x=0;x<384;x+=32)tile(x===192?12:8,x,112);
 for(const [b,x,y] of def.buildings){
  const index=(b<4?16:32)+(b%4)*2;
  tile(index,x,y);tile(index+1,x+32,y);tile(index+8,x,y+32);tile(index+9,x+32,y+32);
 }
 for(const [index,x,y] of def.props)tile(index,x,y);
 const terrain=blank(384,256);
 for(let y=0;y<16;y++)for(let x=0;x<24;x++){
  const i=map[y][x]-1;paste(terrain,atlas,x*16,y*16,i%16*16,Math.floor(i/16)*16,16,16);
 }
 const folder=root+'/'+def.slug;fs.mkdirSync(folder,{recursive:true});
 fs.writeFileSync(folder+'/scene-terrain.png',encodePng(terrain));
 for(const [n,v] of def.views.entries()){
  const full={...terrain,pixels:terrain.pixels.slice()},player=decodePng(fs.readFileSync(`docs/assets/world-player/${v.facing}.png`));paste(full,player,...v.player);
  if(!n)fs.writeFileSync(folder+'/scene.png',encodePng(full));
  const crop=blank(128,64);paste(crop,full,0,0,...v.camera,128,64);fs.writeFileSync(folder+`/viewport-${n+1}.png`,encodePng(crop));
 }
 fs.writeFileSync(folder+'/scene.json',JSON.stringify({name:def.name,size:[384,256],tileSize:16,map,views:def.views,atlas:'../../../combined16/atlas16.png'},null,2)+'\n');
 fs.writeFileSync(folder+'/map.tmj',JSON.stringify({type:'map',version:'1.10',orientation:'orthogonal',renderorder:'right-down',width:24,height:16,tilewidth:16,tileheight:16,infinite:false,layers:[{id:1,name:'Urban scenery',type:'tilelayer',x:0,y:0,width:24,height:16,visible:true,opacity:1,data:map.flat()}],tilesets:[{firstgid:1,source:'../../../combined16/tileset.tsx'}]},null,2)+'\n');
}
const prefix='assets/world-environment-32-spike/simple/urban/';
const gallery=`<section id="urban"><h2>Simple urban environments</h2><p>64 new32px tiles: paving, connected streets, eight64px buildings, shopfronts, apartments, clinic, civic hall, warehouse, cafe, station, townhouses and city fixtures. The original16px player appears in each128×64 device view.</p><p><a href="${prefix}atlas32.png">Urban sheet · native</a> · <a href="${prefix}manifest.json">Sheet manifest</a></p><img src="${prefix}atlas32.png" width="512" height="512" alt="Simple nondithered urban tile sheet"><div class="example-grid">${definitions.map(d=>`<article class="example-card"><h3>${d.name}</h3><p><a href="${prefix+d.slug}/map.tmj">Editable Tiled map</a></p><img class="example-map" src="${prefix+d.slug}/scene.png" width="384" height="256" alt="${d.name} arrangement"><div class="example-crops">${d.views.map((v,i)=>`<figure><figcaption>${v.name}</figcaption><p>1× ·128×64</p><div class="device-frame"><img class="native-crop" src="${prefix+d.slug}/viewport-${i+1}.png" width="128" height="64" alt="${v.name} actual size"></div><p>2× inspection</p><img class="large-crop" src="${prefix+d.slug}/viewport-${i+1}.png" width="256" height="128" alt="${v.name} enlarged"></figure>`).join('')}</div></article>`).join('')}</div></section>`;
const page='docs/world-environment-32-spike.html';let html=fs.readFileSync(page,'utf8');
if(!html.includes('<!-- urban-start -->'))html=html.replace('<!-- simple-examples-start -->','<!-- urban-start -->\n<!-- urban-end -->\n<!-- simple-examples-start -->');
fs.writeFileSync(page,html.replace(/<!-- urban-start -->[\s\S]*?<!-- urban-end -->/,'<!-- urban-start -->\n'+gallery+'\n<!-- urban-end -->'));
console.log('PASS: two urban blocks and six native128x64 player views');
