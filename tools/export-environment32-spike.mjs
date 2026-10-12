#!/usr/bin/env node
// Review artwork only: does not generate or replace installed game assets.
import fs from 'node:fs';
import { decodePng, encodePng } from './creature-sprite-export.mjs';

const root = process.argv[2] || 'docs/assets/world-environment-32-spike';
const clarity = root.endsWith('/lure-clarity');
const columns = (clarity || root.endsWith('/expansion') || root.endsWith('/terrain')) ? 4 : 8;
const size = columns * 32, enlarged = size * 4;
const source = decodePng(fs.readFileSync(`${root}/concept.png`));
if (!clarity && source.width !== source.height) throw Error('Expected square concept');
const native = { width: size, height: size, pixels: new Uint8Array(size * size * 4) };
let black = 0;
const bayer = [0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5];
if (!clarity) for (let y = 0; y < size; y++) for (let x = 0; x < size; x++) {
  let luminance = 0, area = 0;
  const left = x * source.width / size, right = (x + 1) * source.width / size;
  const top = y * source.height / size, bottom = (y + 1) * source.height / size;
  for (let sy = Math.floor(top); sy < Math.ceil(bottom); sy++)
    for (let sx = Math.floor(left); sx < Math.ceil(right); sx++) {
      const weight = (Math.min(right, sx + 1) - Math.max(left, sx)) * (Math.min(bottom, sy + 1) - Math.max(top, sy));
      const i = (sy * source.width + sx) * 4, alpha = source.pixels[i + 3] / 255;
      luminance += weight * (alpha * (source.pixels[i] * 299 + source.pixels[i + 1] * 587 + source.pixels[i + 2] * 114) / 1000 + 255 * (1 - alpha));
      area += weight;
    }
  const value = luminance / area < (clarity ? 128 : (bayer[(y % 4) * 4 + x % 4] + .5) * 16) ? 0 : 255;
  black += value === 0;
  native.pixels.set([value, value, value, 255], (y * size + x) * 4);
}

if(clarity) {
 native.pixels.fill(255);
 const bands=JSON.parse(fs.readFileSync(`${root}/source-bands.json`));
 for(let row=0;row<4;row++)for(let col=0;col<4;col++){
  let left=source.width,top=source.height,right=0,bottom=0;
  for(let y=bands[row];y<bands[row+1];y++)for(let x=Math.floor(col*source.width/4);x<Math.floor((col+1)*source.width/4);x++){
   const i=(y*source.width+x)*4;
   if(source.pixels[i]<128){left=Math.min(left,x);right=Math.max(right,x+1);top=Math.min(top,y);bottom=Math.max(bottom,y+1);}
  }
  const limit=row===2?16:30,ratio=Math.min(limit/(right-left),limit/(bottom-top));
  const w=Math.max(1,Math.round((right-left)*ratio)),h=Math.max(1,Math.round((bottom-top)*ratio));
  const dx=col*32+Math.floor((32-w)/2),dy=row*32+Math.floor((32-h)/2);
  for(let y=0;y<h;y++)for(let x=0;x<w;x++){
   const l=left+x*(right-left)/w,r=left+(x+1)*(right-left)/w,t=top+y*(bottom-top)/h,b=top+(y+1)*(bottom-top)/h;
   let sum=0,area=0;
   for(let yy=Math.floor(t);yy<Math.ceil(b);yy++)for(let xx=Math.floor(l);xx<Math.ceil(r);xx++){
    const weight=(Math.min(r,xx+1)-Math.max(l,xx))*(Math.min(b,yy+1)-Math.max(t,yy));
    sum+=source.pixels[(yy*source.width+xx)*4]*weight;area+=weight;
   }
   const v=sum/area<128?0:255;native.pixels.set([v,v,v,255],((dy+y)*size+dx+x)*4);
  }
 }
 black=0;for(let i=0;i<native.pixels.length;i+=4)black+=native.pixels[i]===0;
}

if (root.endsWith('/terrain')) {
  // Pair the texture's opposite edge pixels so repetitions have no border discontinuity.
  for(let row=0;row<4;row++)for(let col=0;col<4;col++) {
    for(let y=0;y<32;y++) {
      const a=((row*32+y)*size+col*32)*4,b=a+31*4;
      native.pixels.set(native.pixels.subarray(a,a+4),b);
    }
    for(let x=0;x<32;x++) {
      const a=(row*32*size+col*32+x)*4,b=a+31*size*4;
      native.pixels.set(native.pixels.subarray(a,a+4),b);
    }
  }
  black=0;for(let i=0;i<native.pixels.length;i+=4)black+=native.pixels[i]===0;
}
fs.writeFileSync(`${root}/atlas32.png`, encodePng(native));
const preview = { width: enlarged, height: enlarged, pixels: new Uint8Array(enlarged * enlarged * 4) };
for (let y = 0; y < enlarged; y++) for (let x = 0; x < enlarged; x++) {
  const i = (Math.floor(y / 4) * size + Math.floor(x / 4)) * 4;
  preview.pixels.set(native.pixels.subarray(i, i + 4), (y * enlarged + x) * 4);
}
fs.writeFileSync(`${root}/preview4x.png`, encodePng(preview));
fs.writeFileSync(`${root}/manifest.json`, JSON.stringify({
  reviewOnly: true, source: 'concept.png', sourceSize: [source.width, source.height],
  sheet: [size, size], cell: [32, 32], columns, rows: columns, tiles: columns * columns,
  palette: ['#000000', '#ffffff'], opaque: true,
  export: clarity ? 'Authored source row bands and ink bounds fitted to30px scenery or16px fixtures; area-weighted fixed128 binary threshold; no added Bayer noise.' : 'Area-weighted sampling; alpha composited over white; 4x4 Bayer ordered dithering at native pixel scale. Preview enlarged nearest-neighbor.',
  blackPixels: black, whitePixels: size * size - black,
  raw1bppBytes: size * size / 8, alignmentAndSeamsCertified: false,
  rowThemes: clarity ? ['Canopy silhouettes', 'Shrubs and roots', 'Small interaction fixtures', 'Boundary props'] : root.endsWith('/terrain') ? ['Meadow: grass / dirt / stubble / soil', 'Grove: moss / leaf litter / pond / wet undergrowth', 'Shore: shell sand / packed sand / sea / reef', 'Quarry: ash / grit / coal / flint'] : root.endsWith('/habitats') ? ['Huskfield vegetation', 'Huskfield props', 'Dewcap vegetation', 'Dewcap pools and logs', 'Shellstone fossils', 'Shellstone shore', 'Cinderflint minerals', 'Cinderflint mine'] : columns === 4 ? ['16px paths', 'Water and footbridge', 'Compact cottage and garden', 'Foliage and props'] : ['Ground', 'Foliage', 'Stone', 'Water', 'Village props', 'Roofs and bridges', 'Facades and docks', 'Interiors'],
}, null, 2) + '\n');
console.log(`PASS: ${size}x${size} opaque binary sheet; ${columns * columns} cells of 32x32; ${black} black / ${size * size - black} white pixels; ${size * size / 8} B raw 1bpp.`);
