#!/usr/bin/env node
// Review art only: raster seeds -> reusable tile pieces -> layered world -> camera views.
import fs from 'node:fs';
import path from 'node:path';
import { decodePng, encodePng } from './creature-sprite-export.mjs';

const root = 'docs/assets/overworld-fresh';
const directions = [{ id: 'rounded', label: 'Soft blocks' }, { id: 'noir', label: 'Ink silhouettes' }, { id: 'etched', label: 'Stepped etching' }];
const treatments = ['clean', 'light', 'heavy'];
const assets = [
  ['grass',16,16], ['dirt',16,16], ['floor',16,16], ['paving',16,16],
  ['water',16,16], ['wall',16,16], ['flowers',16,16], ['crops',16,16],
  ['tree',32,32], ['pine',16,32], ['bush',16,16], ['boulder',16,16],
  ['cottage',48,32], ['shop',48,48], ['lamp',16,32], ['crystal',16,16],
];
const extensionAssets = [
  ['tower',48,96,false], ['apartment',48,80,false], ['storefront',48,32,false], ['asphalt',16,16,true],
  ['sea',16,16,true], ['beach',16,16,true], ['pier',32,16,false], ['boat',32,16,false],
  ['palm',32,48,false], ['reeds',16,16,false], ['cactus',16,32,false], ['sandstone',32,32,false],
  ['sand',16,16,true], ['mesa',32,32,false], ['adobe',48,32,false], ['dry-shrub',16,16,false],
];
const biomeAssets = [
  ['scree',16,16,true], ['mud',16,16,true], ['snow',16,16,true], ['basalt',16,16,true],
  ['lava',16,16,true], ['ice',16,16,true], ['ruin-paving',16,16,true], ['marsh-water',16,16,true],
  ['cliff',32,32,false], ['stairs',16,32,false], ['ruin-arch',48,48,false], ['broken-column',16,32,false],
  ['snow-pine',32,48,false], ['dead-tree',32,32,false], ['vent',32,32,false], ['ruin-wall',32,16,false],
];
const cameras = [
  { id: 'mountain-pass', label: 'Mountain pass', x: 72, y: 8 },
  { id: 'marsh', label: 'Marsh boardwalk', x: 7, y: 46 },
  { id: 'ancient-ruins', label: 'Ancient ruins', x: 40, y: 47 },
  { id: 'snowy-highlands', label: 'Snowy highlands', x: 77, y: 27 },
  { id: 'volcanic-badlands', label: 'Volcanic badlands', x: 74, y: 47 },
  { id: 'modern-city', label: 'Modern city · street', x: 42, y: 8 },
  { id: 'high-rises', label: 'Modern city · tower tops', x: 54, y: 0 },
  { id: 'waterfront', label: 'Waterfront', x: 6, y: 24 },
  { id: 'islands', label: 'Islands', x: 21, y: 27 },
  { id: 'desert', label: 'Desert', x: 36, y: 25 },
  { id: 'oasis', label: 'Desert oasis', x: 50, y: 27 },
  { id: 'cave', label: 'Cave', x: 4, y: 1 },
  { id: 'forest', label: 'Forest', x: 20, y: 3 },
  { id: 'rural-town', label: 'Rural town', x: 4, y: 11 },
  { id: 'city', label: 'Classic city', x: 20, y: 11 },
];
const W = 96, H = 60, columns = 16;
const player = decodePng(fs.readFileSync('images/Playerchibi_16x16.png'));
const writeJson = (file, value) => fs.writeFileSync(file, JSON.stringify(value, null, 2) + '\n');
const image = (width, height, opaque = false) => {
  const pixels = new Uint8Array(width * height * 4);
  if (opaque) for (let i = 0; i < pixels.length; i += 4) pixels.set([255,255,255,255], i);
  return { width, height, pixels };
};

// Generated sheets need not have uniform gutters. Find the 16 actual isolated
// alpha components, then assign rows by their common baseline, left to right.
function sourceBoxes(source) {
  const visited = new Uint8Array(source.width * source.height), components = [];
  for (let p = 0; p < visited.length; p++) {
    if (visited[p] || source.pixels[p * 4 + 3] < 192) continue;
    const queue = [p]; visited[p] = 1;
    let x0 = source.width, y0 = source.height, x1 = 0, y1 = 0;
    for (let n = 0; n < queue.length; n++) {
      const q = queue[n], x = q % source.width, y = Math.floor(q / source.width);
      x0 = Math.min(x0, x); x1 = Math.max(x1, x); y0 = Math.min(y0, y); y1 = Math.max(y1, y);
      for (const [dx,dy] of [[-1,0],[1,0],[0,-1],[0,1]]) {
        const nx = x + dx, ny = y + dy;
        if (nx < 0 || nx >= source.width || ny < 0 || ny >= source.height) continue;
        const next = ny * source.width + nx;
        if (visited[next] || source.pixels[next * 4 + 3] < 192) continue;
        visited[next] = 1; queue.push(next);
      }
    }
    if (queue.length > 50) components.push({ x: x0, y: y0, width: x1-x0+1, height: y1-y0+1, area: queue.length });
  }
  const main = components.sort((a,b) => b.area-a.area).slice(0,16).sort((a,b) => a.y+a.height-b.y-b.height);
  if (main.length !== 16) throw Error('Source does not contain 16 isolated sprites');
  const boxes = [];
  for (let row = 0; row < 4; row++) boxes.push(...main.slice(row*4,row*4+4).sort((a,b) => a.x-b.x));
  return boxes;
}

function reduce(source, box, width, height, terrain) {
  const out = image(width,height), inset = terrain ? Math.min(box.width,box.height)*0.055 : 0;
  const bx = box.x+inset, by = box.y+inset, bw = box.width-2*inset, bh = box.height-2*inset;
  for (let y = 0; y < height; y++) for (let x = 0; x < width; x++) {
    const x0=bx+x*bw/width, x1=bx+(x+1)*bw/width, y0=by+y*bh/height, y1=by+(y+1)*bh/height;
    let sum=0, coverage=0, area=0;
    for (let sy=Math.floor(y0);sy<Math.ceil(y1);sy++) for(let sx=Math.floor(x0);sx<Math.ceil(x1);sx++) {
      const weight=(Math.min(x1,sx+1)-Math.max(x0,sx))*(Math.min(y1,sy+1)-Math.max(y0,sy));
      const i=(sy*source.width+sx)*4, alpha=source.pixels[i+3]/255;
      sum+=(source.pixels[i]*0.2126+source.pixels[i+1]*0.7152+source.pixels[i+2]*0.0722)*weight*alpha;
      coverage+=weight*alpha; area+=weight;
    }
    if (!terrain && coverage/area < 0.4) continue;
    const luma=coverage ? sum/coverage : 255;
    const value=luma<80?0:luma<205?153:255;
    out.pixels.set([value,value,value,255],(y*width+x)*4);
  }
  return out;
}

function convert(source, treatment) {
  const out=image(source.width,source.height), bayer=[0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5];
  for(let y=0;y<source.height;y++) for(let x=0;x<source.width;x++) {
    const i=(y*source.width+x)*4;
    if(!source.pixels[i+3]) continue;
    const gray=source.pixels[i], mark=treatment==='light'?(x%2===0&&y%2===0):treatment==='heavy'?bayer[(y%4)*4+x%4]<8:false;
    const value=gray===0||gray===153&&mark?0:255;
    out.pixels.set([value,value,value,255],i);
  }
  return out;
}

function paste(target,source,ox,oy) {
  for(let y=0;y<source.height;y++) for(let x=0;x<source.width;x++) {
    if(ox+x<0||oy+y<0||ox+x>=target.width||oy+y>=target.height) continue;
    const si=(y*source.width+x)*4;
    if(!source.pixels[si+3]) continue;
    target.pixels.set(source.pixels.subarray(si,si+4),((oy+y)*target.width+ox+x)*4);
  }
}
function crop(source,x,y,width,height) {
  const out=image(width,height);
  for(let yy=0;yy<height;yy++) for(let xx=0;xx<width;xx++) {
    const si=((y+yy)*source.width+x+xx)*4;
    out.pixels.set(source.pixels.subarray(si,si+4),(yy*width+xx)*4);
  }
  return out;
}
function enlarge(source,scale) {
  const out=image(source.width*scale,source.height*scale);
  for(let y=0;y<out.height;y++) for(let x=0;x<out.width;x++) {
    const si=(Math.floor(y/scale)*source.width+Math.floor(x/scale))*4;
    out.pixels.set(source.pixels.subarray(si,si+4),(y*out.width+x)*4);
  }
  return out;
}

function catalogFor(seed) {
  const tiles=[], catalog=[], ids={}, sprites={};
  const add=(name,pixels,meta={})=>{ const id=tiles.length; tiles.push(pixels);catalog.push({id,name,...meta});ids[name]=id;return id; };
  const aggregate=new Set(['5,3','10,6','20,2','28,5','2,11','13,13','24,12','30,15','6,19','18,21','27,18','3,27','13,30','23,28']);
  const pavingInk=(x,y)=>y%8===0||(x+(Math.floor(y/8)%2)*8)%16===0||aggregate.has(`${x%32},${y%32}`);
  for(const name of ['grass','dirt','floor','paving','water','wall']) add(name,seed[name],{family:'ground'});
  const inShape=(mask,x,y)=>mask===15||(x>=4&&x<12&&y>=4&&y<12)||Boolean(mask&1&&x>=4&&x<12&&y<8)||Boolean(mask&2&&y>=4&&y<12&&x>=8)||Boolean(mask&4&&x>=4&&x<12&&y>=8)||Boolean(mask&8&&y>=4&&y<12&&x<8);
  for(const family of ['path','bank']) for(let mask=0;mask<16;mask++) {
    const tile=crop(seed.grass,0,0,16,16), fill=family==='path'?seed.dirt:seed.water;
    for(let y=0;y<16;y++) for(let x=0;x<16;x++) {
      const i=(y*16+x)*4;
      if(inShape(mask,x,y)) tile.pixels.set(fill.pixels.subarray(i,i+4),i);
      else if(family==='path'&&(x+y)%3===0&&[[1,0],[-1,0],[0,1],[0,-1]].some(([dx,dy])=>inShape(mask,x+dx,y+dy))) tile.pixels.set([0,0,0,255],i);
    }
    add(`${family}-${mask}`,tile,{family,connections:mask});
  }
  for(const side of ['n','e','s','w','ne','se','sw','nw']) {
    const tile=crop(seed.floor,0,0,16,16);
    for(let y=0;y<16;y++)for(let x=0;x<16;x++)if(side.includes('n')&&y<5||side.includes('s')&&y>=11||side.includes('w')&&x<5||side.includes('e')&&x>=11){const i=(y*16+x)*4;tile.pixels.set(seed.wall.pixels.subarray(i,i+4),i);}
    add(`cave-edge-${side}`,tile,{family:'cave edge'});
  }
  const bridge=crop(seed.paving,0,0,16,16);
  for(let x=0;x<16;x++)for(const y of [0,15])bridge.pixels.set([0,0,0,255],(y*16+x)*4);
  add('bridge',bridge,{family:'crossing'});
  for(const [name,width,height] of assets.slice(6)) {
    const parts=[];
    for(let y=0;y<height;y+=16)for(let x=0;x<width;x+=16)parts.push(add(`${name}-${x/16}-${y/16}`,crop(seed[name],x,y,16,16),{family:'object',asset:name,part:[x/16,y/16]}));
    sprites[name]={width:width/16,height:height/16,parts};
  }
  // Append the new families so the first pass's 75 IDs stay stable.
  for(const name of ['asphalt','sea','beach','sand'])add(name,seed[name],{family:'extension ground'});
  for(const family of ['coast','oasis'])for(let mask=0;mask<16;mask++){
    const tile=crop(family==='coast'?seed.sea:seed.sand,0,0,16,16), fill=family==='coast'?seed.beach:seed.sea;
    for(let y=0;y<16;y++)for(let x=0;x<16;x++)if(inShape(mask,x,y)){const i=(y*16+x)*4;tile.pixels.set(fill.pixels.subarray(i,i+4),i);}
    add(`${family}-${mask}`,tile,{family,connections:mask});
  }
  const rotate=source=>{const out=image(16,16);for(let y=0;y<16;y++)for(let x=0;x<16;x++){const i=(y*16+x)*4;out.pixels.set(source.pixels.subarray(i,i+4),((x*16)+(15-y))*4);}return out;};
  add('asphalt-horizontal',rotate(seed.asphalt),{family:'street'});
  const intersection=crop(seed.asphalt,0,0,16,16);
  for(let i=0;i<intersection.pixels.length;i+=4)intersection.pixels.set([0,0,0,255],i);
  add('asphalt-intersection',intersection,{family:'street'});
  const crossing=crop(intersection,0,0,16,16);
  for(let y=0;y<16;y++)for(let x=0;x<16;x++)if(y%4<2)crossing.pixels.set([255,255,255,255],(y*16+x)*4);
  add('crosswalk-vertical',crossing,{family:'street'});add('crosswalk-horizontal',rotate(crossing),{family:'street'});
  for(const side of ['n','e','s','w']){
    const curb=image(16,16,true);
    for(let y=0;y<16;y++)for(let x=0;x<16;x++){
      const edging=side==='n'?y<4:side==='s'?y>=12:side==='w'?x<4:x>=12;
      if(!edging&&pavingInk(x,y))curb.pixels.set([0,0,0,255],(y*16+x)*4);
    }
    for(let y=0;y<16;y++)for(let x=0;x<16;x++)if(side==='n'&&y===0||side==='s'&&y===15||side==='w'&&x===0||side==='e'&&x===15)curb.pixels.set([0,0,0,255],(y*16+x)*4);
    add(`curb-${side}`,curb,{family:'street edge'});
  }
  for(const [name,width,height,terrain]of extensionAssets)if(!terrain){
    const parts=[];
    for(let y=0;y<height;y+=16)for(let x=0;x<width;x+=16)parts.push(add(`${name}-${x/16}-${y/16}`,crop(seed[name],x,y,16,16),{family:'extension object',asset:name,part:[x/16,y/16]}));
    sprites[name]={width:width/16,height:height/16,parts};
  }
  const floors=(name,asset,rows)=>{const source=sprites[asset];sprites[name]={width:source.width,height:rows.length,parts:rows.flatMap(row=>source.parts.slice(row*source.width,(row+1)*source.width)),source:asset,sourceRows:rows};};
  floors('tower-tall','tower',[0,1,2,3,2,3,4,5]);
  floors('tower-short','tower',[0,1,2,5]);
  floors('apartment-tall','apartment',[0,1,2,3,1,2,4]);
  floors('apartment-short','apartment',[0,1,2,4]);
  for(let qy=0;qy<2;qy++)for(let qx=0;qx<2;qx++){
    const slab=image(16,16,true);
    for(let y=0;y<16;y++)for(let x=0;x<16;x++)if(pavingInk(qx*16+x,qy*16+y))slab.pixels.set([0,0,0,255],(y*16+x)*4);
    add(`concrete-${qx}-${qy}`,slab,{family:'modern paving',quadrant:[qx,qy],pattern:'staggered 16×8 pavers with sparse aggregate'});
  }
  // Preserve IDs 0–189, then append the terrain and props for five more biomes.
  for(const [name,,,terrain]of biomeAssets)if(terrain)add(name,seed[name],{family:'biome ground'});
  for(const [family,base,fill]of[
    ['mountain-trail','scree','dirt'],['marsh-bank','marsh-water','mud'],
    ['ice-bank','snow','ice'],['lava-bank','basalt','lava'],
  ])for(let mask=0;mask<16;mask++){
    const tile=crop(seed[base],0,0,16,16);
    for(let y=0;y<16;y++)for(let x=0;x<16;x++)if(inShape(mask,x,y)){
      const i=(y*16+x)*4;tile.pixels.set(seed[fill].pixels.subarray(i,i+4),i);
    }
    add(`${family}-${mask}`,tile,{family,connections:mask});
  }
  for(const [name,width,height,terrain]of biomeAssets)if(!terrain){
    const parts=[];
    for(let y=0;y<height;y+=16)for(let x=0;x<width;x+=16)parts.push(add(`${name}-${x/16}-${y/16}`,crop(seed[name],x,y,16,16),{family:'biome object',asset:name,part:[x/16,y/16]}));
    sprites[name]={width:width/16,height:height/16,parts};
  }
  return {tiles,catalog,ids,sprites};
}

function worldFor(kit) {
  const {ids,sprites}=kit, ground=new Array(W*H).fill(ids.grass), objects=new Array(W*H).fill(-1), road=new Uint8Array(W*H);
  const rect=(x,y,w,h,id)=>{for(let yy=y;yy<y+h;yy++)for(let xx=x;xx<x+w;xx++)ground[yy*W+xx]=id;};
  const route=(x,y,w,h)=>{for(let yy=y;yy<y+h;yy++)for(let xx=x;xx<x+w;xx++)road[yy*W+xx]=1;};
  const prop=(name,x,y)=>{const s=sprites[name];if(x<0||y<0||x+s.width>W||y+s.height>H)throw Error(`${name}: object leaves the world`);for(let yy=0;yy<s.height;yy++)for(let xx=0;xx<s.width;xx++)objects[(y+yy)*W+x+xx]=s.parts[yy*s.width+xx];};
  rect(1,1,13,8,ids.floor);
  rect(1,1,13,1,ids.wall);rect(1,8,13,1,ids.wall);rect(1,1,1,8,ids.wall);rect(13,1,1,8,ids.wall);
  rect(7,8,2,1,ids.floor);
  rect(17,10,15,10,ids.paving);
  route(7,0,2,20);route(23,0,2,20);route(0,8,32,2);route(0,18,32,2);route(0,13,15,2);
  for(let y=0;y<H;y++)for(let x=0;x<W;x++)if(road[y*W+x]&&ground[y*W+x]===ids.grass){
    let mask=0;for(const [dx,dy,bit]of[[0,-1,1],[1,0,2],[0,1,4],[-1,0,8]])if(x+dx>=0&&x+dx<W&&y+dy>=0&&y+dy<H&&road[(y+dy)*W+x+dx])mask|=bit;
    ground[y*W+x]=ids[`path-${mask}`];
  }
  for(let y=0;y<20;y++){ground[y*W+15]=ids['bank-7'];ground[y*W+16]=ids['bank-13'];}
  for(const y of [8,9,18,19])for(const x of [15,16])ground[y*W+x]=ids.bridge;
  // A modern district: dark avenues, pale pavements, crossings and floor-built towers.
  for(let y=0;y<20;y++)for(let x=32;x<64;x++)ground[y*W+x]=ids[`concrete-${x%2}-${y%2}`];
  for(const y of [12,13,18,19])rect(32,y,32,1,ids['asphalt-horizontal']);
  for(const x of [45,46,57,58])rect(x,0,1,20,ids.asphalt);
  for(const y of [12,13,18,19])for(const x of [45,46,57,58])ground[y*W+x]=ids['asphalt-intersection'];
  for(const y of [11,14])for(const x of [45,46,57,58])ground[y*W+x]=ids['crosswalk-vertical'];
  for(const x of [43,48,55,60])for(const y of [12,13])ground[y*W+x]=ids['crosswalk-horizontal'];
  for(let y=0;y<20;y++)if(![11,12,13,14,18,19].includes(y))for(const [x,side]of[[44,'e'],[47,'w'],[56,'e'],[59,'w']])ground[y*W+x]=ids[`curb-${side}`];
  // The southern coast and islands use shared land/water connection masks.
  rect(0,20,32,20,ids.sea);
  const land=new Uint8Array(W*H);
  for(let y=20;y<40;y++)for(let x=0;x<(y>=24&&y<30?10:12);x++)land[y*W+x]=1;
  for(let y=27;y<=30;y++)for(let x=23;x<=27;x++)land[y*W+x]=1;
  for(const y of [26,31])for(let x=24;x<=26;x++)land[y*W+x]=1;
  for(let y=35;y<=37;y++)for(let x=18;x<=21;x++)land[y*W+x]=1;
  const connects=(maskData,x,y,edge=false)=>{let mask=0;for(const [dx,dy,bit]of[[0,-1,1],[1,0,2],[0,1,4],[-1,0,8]]){const xx=x+dx,yy=y+dy;if(xx>=0&&xx<W&&yy>=0&&yy<H&&maskData[yy*W+xx]||edge&&(xx<0||yy===19&&xx<12))mask|=bit;}return mask;};
  for(let y=20;y<40;y++)for(let x=0;x<32;x++)if(land[y*W+x])ground[y*W+x]=ids[`coast-${connects(land,x,y,true)}`];
  // Open desert and an oasis; sand and shore variants share the same grid.
  rect(32,20,32,20,ids.sand);
  for(let y=20;y<40;y++)ground[y*W+32]=ids['coast-7'];
  const pond=new Uint8Array(W*H);
  for(let y=27;y<=31;y++)for(let x=55;x<=59;x++)pond[y*W+x]=1;
  for(const y of [26,32])for(let x=56;x<=58;x++)pond[y*W+x]=1;
  for(let y=26;y<=32;y++)for(let x=55;x<=59;x++)if(pond[y*W+x])ground[y*W+x]=ids[`oasis-${connects(pond,x,y)}`];
  for(const [name,x,y] of [
    ['crystal',5,3],['crystal',11,4],['boulder',10,3],['crystal',3,6],['boulder',11,6],
    ['tree',20,3],['tree',26,3],['tree',18,6],['tree',27,6],['pine',21,1],['pine',29,1],['pine',22,5],['bush',21,6],['flowers',22,4],['flowers',25,6],['boulder',26,7],
    ['cottage',3,11],['cottage',10,11],['cottage',2,16],['cottage',10,16],['crops',5,14],['crops',5,15],['crops',4,15],['flowers',10,14],['bush',2,14],['bush',12,14],
    ['shop',19,11],['shop',26,11],['shop',19,16],['shop',26,16],['lamp',22,13],['lamp',25,13],['flowers',18,14],['bush',30,14],
  ])prop(name,x,y);
  for(const [name,x,y]of[
    ['tower-tall',41,3],['apartment-tall',48,4],['tower-short',34,4],['apartment-short',34,12],
    ['tower',54,1],['apartment-tall',60,0],['storefront',41,14],['storefront',48,14],['storefront',54,15],['storefront',60,15],
    ['flowers',44,10],['bush',47,10],['bush',38,9],['bush',51,11],['tree',34,17],['tree',38,17],
    ['palm',6,24],['palm',3,28],['palm',22,26],['palm',26,28],['palm',18,34],
    ['boat',12,27],['boat',19,24],['boat',29,34],['reeds',9,28],['reeds',11,32],['reeds',27,31],
    ['cottage',3,21],['cottage',1,30],
    ['mesa',36,25],['mesa',44,34],['mesa',57,36],['sandstone',42,25],['sandstone',34,22],['sandstone',60,23],
    ['cactus',38,27],['cactus',42,27],['cactus',49,23],['cactus',45,30],['cactus',60,34],['cactus',34,37],
    ['dry-shrub',40,25],['dry-shrub',39,29],['dry-shrub',52,33],['dry-shrub',59,37],
    ['adobe',34,32],['adobe',42,32],['adobe',49,35],['adobe',59,21],
    ['palm',51,27],['palm',60,25],['palm',60,30],['reeds',54,30],['reeds',60,29],
  ])prop(name,x,y);
  for(const y of [25,26])for(const x of [9,11,13,15])prop('pier',x,y);
  for(const x of [20,22])prop('pier',x,30);
  // Mountain: repeating cliff faces make terraces, with stairs and a chasm bridge.
  rect(64,0,32,20,ids.scree);
  const trail=new Uint8Array(W*H), mark=(data,x,y,w,h)=>{for(let yy=y;yy<y+h;yy++)for(let xx=x;xx<x+w;xx++)data[yy*W+xx]=1;};
  mark(trail,64,9,32,2);mark(trail,75,0,2,20);mark(trail,84,3,2,15);
  for(let y=0;y<20;y++)for(let x=64;x<W;x++)if(trail[y*W+x])ground[y*W+x]=ids[`mountain-trail-${connects(trail,x,y)}`];
  for(const y of [2,7,11,16])for(let x=64;x<96;x+=2)if(x<80||x>81)prop('cliff',x,y);
  for(const [x,y]of[[75,2],[76,2],[77,8],[78,8],[75,11],[76,11],[84,16],[85,16]])prop('stairs',x,y);
  rect(80,0,2,20,ids.basalt);
  rect(79,9,4,2,ids.bridge);
  for(const [x,y]of[[68,5],[90,5],[69,14],[89,14],[93,18]])prop('boulder',x,y);
  // Marsh: irregular mud islands and a continuous two-tile boardwalk network.
  rect(0,40,32,20,ids['marsh-water']);
  const mud=new Uint8Array(W*H);
  mark(mud,0,40,8,20);mark(mud,5,43,5,5);mark(mud,18,44,8,8);mark(mud,14,54,13,6);
  mark(mud,25,40,7,8);mark(mud,0,56,16,4);mark(mud,13,45,3,3);
  for(let y=40;y<H;y++)for(let x=0;x<32;x++)if(mud[y*W+x])ground[y*W+x]=ids[`marsh-bank-${connects(mud,x,y)}`];
  for(const y of [48,49])for(let x=6;x<26;x+=2)prop('pier',x,y);
  for(let y=41;y<58;y++)prop('pier',10,y);
  for(const [name,x,y]of[
    ['dead-tree',7,46],['dead-tree',13,46],['dead-tree',22,44],['dead-tree',3,50],['dead-tree',20,54],['dead-tree',28,41],
    ['reeds',9,46],['reeds',12,47],['reeds',13,50],['reeds',17,47],['reeds',21,50],['reeds',25,48],['reeds',7,53],['reeds',17,55],
    ['boulder',2,43],['boulder',24,57],
  ])prop(name,x,y);
  // Ruins: a repeatable courtyard, modular perimeter walls and broken columns.
  rect(32,40,32,20,ids.grass);rect(36,44,25,13,ids['ruin-paving']);
  rect(32,48,32,2,ids.dirt);rect(43,40,2,20,ids['ruin-paving']);
  for(let x=36;x<62;x+=2)for(const y of [44,56])if(x<42||x>45)prop('ruin-wall',x,y);
  for(const x of [37,41,48,52,58])for(const y of [46,52])prop('broken-column',x,y);
  for(const [x,y]of[[40,47],[51,48],[34,53]])prop('ruin-arch',x,y);
  prop('broken-column',46,47);prop('ruin-wall',46,50);
  for(const [name,x,y]of[['bush',46,46],['bush',49,55],['bush',38,54],['tree',32,41],['tree',59,41],['boulder',57,54],['flowers',46,52]])prop(name,x,y);
  // Snow: open white ground, dark snow-capped pines and a cracked frozen lake.
  rect(64,20,32,20,ids.snow);
  const frozen=new Uint8Array(W*H);mark(frozen,82,26,9,7);mark(frozen,84,24,5,11);
  for(let y=24;y<35;y++)for(let x=82;x<91;x++)if(frozen[y*W+x])ground[y*W+x]=ids[`ice-bank-${connects(frozen,x,y)}`];
  for(const [x,y]of[[65,21],[70,22],[74,24],[86,22],[78,27],[66,28],[72,31],[78,32],[91,27],[91,34],[84,36]])prop('snow-pine',x,y);
  for(const [x,y]of[[68,24],[89,22],[81,35],[94,24],[68,35]])prop('boulder',x,y);
  for(let x=64;x<96;x+=2)if(x<74||x>77)prop('cliff',x,37);
  for(const x of [75,76])prop('stairs',x,37);
  // Volcano: white connected lava against dark plates; pale scree is safe ground.
  rect(64,40,32,20,ids.basalt);
  const molten=new Uint8Array(W*H);
  mark(molten,64,43,32,2);mark(molten,83,40,3,20);mark(molten,69,50,27,2);mark(molten,68,50,2,10);
  mark(molten,88,54,8,6);
  for(let y=40;y<H;y++)for(let x=64;x<W;x++)if(molten[y*W+x])ground[y*W+x]=ids[`lava-bank-${connects(molten,x,y)}`];
  rect(76,46,4,4,ids.scree);rect(75,40,2,20,ids.scree);
  // A stone crossing continues the highlands route over both lava channels.
  for(const y of [43,44,50,51])rect(75,y,2,1,ids.bridge);
  for(const [x,y]of[[74,47],[80,47],[87,46],[69,55],[91,40],[91,55]])prop('vent',x,y);
  for(const [x,y]of[[65,46],[78,54],[88,52]])prop('cliff',x,y);
  for(const [x,y]of[[79,49],[73,53],[81,58]])prop('boulder',x,y);
  return {width:W,height:H,ground,objects};
}

function render(kit,world) {
  const out=image(world.width*16,world.height*16,true);
  for(const layer of ['ground','objects'])world[layer].forEach((id,i)=>{if(id>=0)paste(out,kit.tiles[id],i%world.width*16,Math.floor(i/world.width)*16);});
  return out;
}
function playerOn(out) {
  paste(out,crop(player,0,16,16,16),56,24);
}
function tiled(world) {
  return {type:'map',version:'1.10',orientation:'orthogonal',renderorder:'right-down',width:world.width,height:world.height,tilewidth:16,tileheight:16,infinite:false,
    tilesets:[{firstgid:1,source:'world.tsx'}],
    layers:['ground','objects'].map((name,i)=>({id:i+1,name,type:'tilelayer',width:world.width,height:world.height,x:0,y:0,opacity:1,visible:true,data:world[name].map(id=>id+1)})),nextlayerid:3,nextobjectid:1};
}
function tileset(kit) {
  const rows=Math.ceil(kit.tiles.length/columns);
  return `<?xml version="1.0" encoding="UTF-8"?>\n<tileset version="1.10" name="Fresh overworld" tilewidth="16" tileheight="16" tilecount="${kit.tiles.length}" columns="${columns}">\n<image source="atlas.png" width="${columns*16}" height="${rows*16}"/>\n${kit.catalog.map(t=>`<tile id="${t.id}"><properties><property name="name" value="${t.name}"/></properties></tile>`).join('\n')}\n</tileset>\n`;
}

const manifest={reviewOnly:true,directions,treatments,viewport:[128,64],tileSize:[16,16],world:[W,H],player:{source:'images/Playerchibi_16x16.png',position:[56,24],size:[16,16]},cameras,
  dither:{clean:'Gray shadow areas become white; black contours are retained.',light:'Only gray shadow pixels receive 25% black ordered texture.',heavy:'Only gray shadow pixels receive 50% black ordered texture.'},
  sourcePolicy:'Three base, three city/coast/desert and three biome sprite sheets generated with built-in image_gen. No existing spike imagery supplied as reference.',sourceAssets:[...assets.map(([name,width,height])=>({name,width,height,source:'base'})),...extensionAssets.map(([name,width,height])=>({name,width,height,source:'extension'})),...biomeAssets.map(([name,width,height])=>({name,width,height,source:'biomes'}))]};
for(const direction of directions){
  const source=decodePng(fs.readFileSync(`${root}/sources/${direction.id}.png`)), boxes=sourceBoxes(source), native={};
  assets.forEach(([name,width,height],i)=>{native[name]=reduce(source,boxes[i],width,height,i<6);});
  direction.extraction=assets.map(([name],i)=>({name,...boxes[i]}));
  const extension=decodePng(fs.readFileSync(`${root}/sources/extension-${direction.id}.png`)), extensionBoxes=sourceBoxes(extension);
  extensionAssets.forEach(([name,width,height,terrain],i)=>{native[name]=reduce(extension,extensionBoxes[i],width,height,terrain);});
  direction.extensionExtraction=extensionAssets.map(([name],i)=>({name,...extensionBoxes[i]}));
  const biomes=decodePng(fs.readFileSync(`${root}/sources/biomes-${direction.id}.png`)), biomeBoxes=sourceBoxes(biomes);
  biomeAssets.forEach(([name,width,height,terrain],i)=>{native[name]=reduce(biomes,biomeBoxes[i],width,height,terrain);});
  direction.biomeExtraction=biomeAssets.map(([name],i)=>({name,...biomeBoxes[i]}));
  const contact=image(800,cameras.length*136+8,true);
  for(const [column,treatment] of treatments.entries()){
    const outDir=`${root}/${direction.id}/${treatment}`;fs.mkdirSync(outDir,{recursive:true});
    const seed=Object.fromEntries(Object.entries(native).map(([name,pixels])=>[name,convert(pixels,treatment)]));
    const kit=catalogFor(seed),world=worldFor(kit),full=render(kit,world),atlas=image(columns*16,Math.ceil(kit.tiles.length/columns)*16);
    if(kit.tiles.length!==295)throw Error(`Fresh catalog must contain 295 stable tile IDs; found ${kit.tiles.length}`);
    for(const tile of kit.tiles)for(let i=0;i<tile.pixels.length;i+=4){
      const [r,g,b,a]=tile.pixels.subarray(i,i+4);
      if(![0,255].includes(a)||r!==g||r!==b||![0,255].includes(r))throw Error('Atlas contains a nonbinary pixel');
    }
    for(const layer of ['ground','objects'])if(world[layer].length!==W*H||world[layer].some(id=>id < (layer==='objects'?-1:0)||id>=kit.tiles.length))throw Error(`Invalid ${layer} map data`);
    kit.tiles.forEach((tile,i)=>paste(atlas,tile,i%columns*16,Math.floor(i/columns)*16));
    fs.writeFileSync(`${outDir}/atlas.png`,encodePng(atlas));
    fs.writeFileSync(`${outDir}/atlas-4x.png`,encodePng(enlarge(atlas,4)));
    fs.writeFileSync(`${outDir}/world.png`,encodePng(full));
    fs.writeFileSync(`${outDir}/world.tsx`,tileset(kit));
    writeJson(`${outDir}/world.tmj`,tiled(world));
    for(const [row,camera] of cameras.entries()){
      if(camera.x<0||camera.y<0||camera.x+8>W||camera.y+4>H)throw Error(`${camera.id}: camera leaves the world`);
      const view=crop(full,camera.x*16,camera.y*16,128,64);playerOn(view);
      fs.writeFileSync(`${outDir}/${camera.id}.png`,encodePng(view));
      const local={width:8,height:4,ground:[],objects:[]};
      for(let y=0;y<4;y++)for(let x=0;x<8;x++)for(const layer of ['ground','objects'])local[layer].push(world[layer][(camera.y+y)*W+camera.x+x]);
      writeJson(`${outDir}/${camera.id}.tmj`,tiled(local));
      paste(contact,enlarge(view,2),8+column*264,8+row*136);
    }
    if(!manifest.catalog){manifest.catalog=kit.catalog;manifest.prefabs=kit.sprites;manifest.worldLayers={ground:world.ground,objects:world.objects};}
  }
  fs.writeFileSync(`${root}/${direction.id}/comparison.png`,encodePng(contact));
}
writeJson(`${root}/manifest.json`,manifest);
console.log(`Exported ${manifest.catalog.length} reusable tiles, 3 art directions, 3 selective-dither variants, one ${W}×${H} layered world and ${cameras.length*directions.length*treatments.length} native camera views.`);
