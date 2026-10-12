#!/usr/bin/env node
// Separate review variants: authored layouts and installed game assets stay intact.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import {decodePng,encodePng} from './creature-sprite-export.mjs';
import {habitatGround,habitatProp} from './habitat-terrain.mjs';
const original='docs/assets/world-environment-32-spike',base=original+'/simple';
const slugs=['river','town','city','cave','cave-dark','huskfield','dewcap','shellstone','cinderflint'];
const read=file=>decodePng(fs.readFileSync(file));
const write=(file,data)=>fs.writeFileSync(file,data);
const blank=(width,height)=>({width,height,pixels:new Uint8Array(width*height*4).fill(255)});
function copy(target,source,dx,dy,sx=0,sy=0,width=source.width,height=source.height){
 for(let y=0;y<height;y++)for(let x=0;x<width;x++){
  const si=((sy+y)*source.width+sx+x)*4,di=((dy+y)*target.width+dx+x)*4;
  if(source.pixels[si+3])target.pixels.set(source.pixels.subarray(si,si+4),di);
 }
}
function enlarge(source,scale){
 const result=blank(source.width*scale,source.height*scale);
 for(let y=0;y<result.height;y++)for(let x=0;x<result.width;x++){
  const si=(Math.floor(y/scale)*source.width+Math.floor(x/scale))*4;
  result.pixels.set(source.pixels.subarray(si,si+4),(y*result.width+x)*4);
 }
 return result;
}
function exportSheet(folder,columns){
 const root=base+(folder?'/'+folder:''),source=read(root+'/concept.png'),size=columns*32,native=blank(size,size);
 assert.equal(source.width,source.height);
 for(let y=0;y<size;y++)for(let x=0;x<size;x++){
  const l=x*source.width/size,r=(x+1)*source.width/size,t=y*source.height/size,b=(y+1)*source.height/size;
  let sum=0,area=0;
  for(let yy=Math.floor(t);yy<Math.ceil(b);yy++)for(let xx=Math.floor(l);xx<Math.ceil(r);xx++){
   const weight=(Math.min(r,xx+1)-Math.max(l,xx))*(Math.min(b,yy+1)-Math.max(t,yy));
   sum+=source.pixels[(yy*source.width+xx)*4]*weight;area+=weight;
  }
  const v=sum/area<128?0:255;native.pixels.set([v,v,v,255],(y*size+x)*4);
 }
 if(folder==='habitats/terrain')for(let row=0;row<4;row++)for(let col=0;col<4;col++)for(let p=0;p<32;p++){
  const a=((row*32+p)*size+col*32)*4,b=(row*32*size+col*32+p)*4;
  native.pixels.set(native.pixels.subarray(a,a+4),a+31*4);
  native.pixels.set(native.pixels.subarray(b,b+4),b+31*size*4);
 }
 let black=0;for(let i=0;i<native.pixels.length;i+=4)black+=native.pixels[i]===0;
 write(root+'/atlas32.png',encodePng(native));write(root+'/preview4x.png',encodePng(enlarge(native,4)));
 write(root+'/manifest.json',JSON.stringify({reviewOnly:true,source:'concept.png',sourceSize:[source.width,source.height],sheet:[size,size],cell:[32,32],columns,rows:columns,tiles:columns*columns,palette:['#000000','#ffffff'],opaque:true,export:'Area-weighted sampling with fixed128 binary threshold; no added dithering. Native cell positions preserved.',blackPixels:black,whitePixels:size*size-black,raw1bppBytes:size*size/8},null,2)+'\n');
 return native;
}
const sheets=[exportSheet('',8),exportSheet('expansion',4),exportSheet('habitats',8)];
const materials=exportSheet('habitats/terrain',4);
function ground(scene){
 const open={...scene,terrain:{...scene.terrain,map:scene.terrain.map.map(row=>row.replace(/[23]/g,'0'))}};
 const image=habitatGround(open,materials),map=scene.terrain.map;
 for(let y=0;y<image.height;y++)for(let x=0;x<image.width;x++){
  const m=Number(map[y>>4][x>>4]);if(m<2)continue;
  const si=((scene.terrain.biome*32+y%32)*materials.width+m*32+x%32)*4;
  image.pixels.set(materials.pixels.subarray(si,si+4),(y*image.width+x)*4);
  // A continuous contour distinguishes quiet white pools/patches from white land.
  const cx=x>>4,cy=y>>4;
  if((x%16===0&&cx>0&&Number(map[cy][cx-1])!==m)||(x%16===15&&cx+1<map[cy].length&&Number(map[cy][cx+1])!==m)||
     (y%16===0&&cy>0&&Number(map[cy-1][cx])!==m)||(y%16===15&&cy+1<map.length&&Number(map[cy+1][cx])!==m))
   image.pixels.set([0,0,0,255],(y*image.width+x)*4);
 }
 return image;
}
const collection=[];
for(const slug of slugs){
 const folder=slug==='river'?'':'scenes/'+slug,root=base+(folder?'/'+folder:'');fs.mkdirSync(root,{recursive:true});
 const source=original+(folder?'/'+folder:'');
 const scene=JSON.parse(fs.readFileSync(source+'/scene.json'));
 scene.name+=' · simple';scene.description='Clear outlines, white highlights and sparse ground. Same authored layout and player positions as the dithered version.';
 collection.push({...scene,folder});write(root+'/scene.json',JSON.stringify(scene,null,2)+'\n');
 const [width,height]=scene.size,terrain=scene.terrain?ground(scene):blank(width,height);
 for(const [id,x,y] of scene.placements){
  const sheet=sheets[id>80?2:id>64?1:0],index=id-(id>80?81:id>64?65:1),columns=sheet.width/32;
  if(scene.terrain)copy(terrain,habitatProp(sheet,index),x,y);
  else copy(terrain,sheet,x,y,index%columns*32,Math.floor(index/columns)*32,32,32);
 }
 if(slug==='cave-dark')for(let i=0;i<terrain.pixels.length;i+=4)for(let c=0;c<3;c++)terrain.pixels[i+c]=255-terrain.pixels[i+c];
 write(root+'/scene-terrain.png',encodePng(terrain));
 for(const [index,view] of scene.views.entries()){
  const full={width,height,pixels:terrain.pixels.slice()},player=read(`docs/assets/world-player/${view.facing}.png`);
  copy(full,player,...view.player);
  if(index===0){write(root+'/scene.png',encodePng(full));write(root+'/scene-preview2x.png',encodePng(enlarge(full,2)));}
  const crop=blank(128,64);copy(crop,full,0,0,...view.camera,128,64);
  write(root+`/viewport-${index+1}.png`,encodePng(crop));write(root+`/viewport-${index+1}-4x.png`,encodePng(enlarge(crop,4)));
 }
 write(root+'/scene-data.js','window.environment32Scene = '+JSON.stringify(scene)+';\n');
 console.log(`PASS: simple ${slug},${scene.placements.length} placements,3 native player views`);
}
write(base+'/scene-collection.js','window.environment32SimpleScenes = '+JSON.stringify(collection)+';\n');
const font={A:'010101111101101',B:'110101110101110',C:'011100100100011',D:'110101101101110',E:'111100110100111',F:'111100110100100',G:'011100101101011',H:'101101111101101',I:'111010010010111',J:'001001001101010',K:'101101110101101',L:'100100100100111',M:'101111111101101',N:'101111111111101',O:'010101101101010',P:'110101110100100',Q:'010101101111011',R:'110101110101101',S:'011100010001110',T:'111010010010010',U:'101101101101111',V:'101101101101010',W:'101101111111101',X:'101101010101101',Y:'101101010010010',Z:'111001010100111','-':'000000111000000'};
function label(image,text,x,y){for(const char of text.toUpperCase()){const bits=font[char]||'0'.repeat(15);for(let i=0;i<15;i++)if(bits[i]==='1')image.pixels.set([0,0,0,255],((y+Math.floor(i/3))*image.width+x+i%3)*4);x+=4;}}
const comparison=blank(344,680);label(comparison,'SIMPLE',72,8);label(comparison,'DITHER',208,8);
const cells=[];
for(const [row,slug] of slugs.entries()){
 const folder=slug==='river'?'':'scenes/'+slug+'/';label(comparison,slug,4,40+row*72);
 for(const [col,style] of ['simple','dither'].entries()){
  const path=(style==='simple'?base:original)+'/'+folder+'viewport-1.png',x=72+col*136,y=32+row*72;
  copy(comparison,read(path),x,y);cells.push({slug,style,source:path,x,y,size:[128,64]});
 }
}
write(base+'/comparison-native.png',encodePng(comparison));write(base+'/comparison-2x.png',encodePng(enlarge(comparison,2)));
write(base+'/comparison.json',JSON.stringify({size:[344,680],cells},null,2)+'\n');
const prefix='assets/world-environment-32-spike/';
const cell=(path,name)=>`<p>1× ·128×64</p><div class="device-frame"><img class="native-crop" src="${path}" width="128" height="64" alt="${name} at device size"></div><p>2× inspection</p><img class="large-crop" src="${path}" width="256" height="128" alt="${name} enlarged">`;
const rows=collection.map((scene,i)=>{const folder=scene.folder?scene.folder+'/':'';return `<tr><th>${scene.name.replace(' · simple','')}</th>${['simple/',''].map((style,j)=>`<td>${cell(prefix+style+folder+'viewport-1.png',scene.name)}<a href="?style=${j?'dither':'simple'}&amp;scene=${slugs[i]}#scene">Inspect this style</a></td>`).join('')}</tr>`;}).join('\n');
const cards=collection.map((scene,i)=>{
 const folder=prefix+'simple/'+(scene.folder?scene.folder+'/':'');
 const residents=scene.creatures?'<p>Residents: '+scene.creatures.map(c=>c.name).join(', ')+'.</p>':'';
 return `<article class="example-card" id="simple-${slugs[i]}"><h3>${scene.name}</h3>${residents}<img class="example-map" src="${folder}scene.png" width="384" height="256" alt="Full ${scene.name}"><div class="example-crops">${scene.views.map((v,index)=>`<figure><figcaption>${v.name}</figcaption>${cell(folder+`viewport-${index+1}.png`,scene.name+' '+v.name)}</figure>`).join('')}</div><a href="?style=simple&amp;scene=${slugs[i]}#scene">Inspect and pan</a></article>`;
}).join('\n');
const section=`<section id="simple-examples"><h2>Simple environments</h2><p>All nine arrangements redrawn with clear outlines, quiet white surfaces and sparse ground. Original tile placements,16×16 player and128×64 cameras are preserved. Dark cave flips only terrain.</p><p><a href="${prefix}simple/comparison-native.png">Comparison sheet · native</a> · <a href="${prefix}simple/comparison-2x.png">Comparison sheet ·2×</a></p><div class="scroll"><table class="set-comparison"><thead><tr><th>Scene</th><th>Simple</th><th>Dithered</th></tr></thead><tbody>${rows}</tbody></table></div><h2>All simple arrangements</h2><div class="all-examples">${cards}</div><h2>Simple tile sheets</h2><div class="example-crops">${['','expansion/','habitats/','habitats/terrain/'].map((folder,i)=>`<figure><figcaption>${['World','Compact village','Creature habitats','Terrain'][i]}</figcaption><a href="${prefix}simple/${folder}atlas32.png"><img src="${prefix}simple/${folder}preview4x.png" width="${i===0||i===2?512:256}" alt="Simple ${folder||'world'} native tile sheet"></a></figure>`).join('')}</div></section>`;
const page='docs/world-environment-32-spike.html';let html=fs.readFileSync(page,'utf8');
if(!html.includes('<!-- simple-examples-start -->'))html=html.replace('<!-- lure-world-start -->','<!-- simple-examples-start -->\n<!-- simple-examples-end -->\n<!-- lure-world-start -->');
html=html.replace(/<!-- simple-examples-start -->[\s\S]*?<!-- simple-examples-end -->/,'<!-- simple-examples-start -->\n'+section+'\n<!-- simple-examples-end -->');
write(page,html);
