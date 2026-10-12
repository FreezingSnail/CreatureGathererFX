#!/usr/bin/env node
// Binary exports of imagegen redraws; only writes the review catalogue.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import {decodePng,encodePng} from './creature-sprite-export.mjs';
const root='docs/assets/world-environment-32-spike/simple';
for(const [folder,width,height,cell] of [['legacy16',256,768,16],['urban',256,256,32]]){
 const path=root+'/'+folder,source=decodePng(fs.readFileSync(path+'/concept.png'));
 assert.ok(Math.abs(source.width/source.height-width/height)<.02,'source aspect ratio');
 const native={width,height,pixels:new Uint8Array(width*height*4).fill(255)};
 for(let y=0;y<height;y++)for(let x=0;x<width;x++){
  const l=x*source.width/width,r=(x+1)*source.width/width,t=y*source.height/height,b=(y+1)*source.height/height;
  let sum=0,area=0;
  for(let yy=Math.floor(t);yy<Math.ceil(b);yy++)for(let xx=Math.floor(l);xx<Math.ceil(r);xx++){
   const w=(Math.min(r,xx+1)-Math.max(l,xx))*(Math.min(b,yy+1)-Math.max(t,yy));
   sum+=source.pixels[(yy*source.width+xx)*4]*w;area+=w;
  }
  const v=sum/area<128?0:255;native.pixels.set([v,v,v,255],(y*width+x)*4);
 }
 fs.writeFileSync(path+`/atlas${cell}.png`,encodePng(native));
 const preview={width:width*4,height:height*4,pixels:new Uint8Array(width*height*64)};
 for(let y=0;y<preview.height;y++)for(let x=0;x<preview.width;x++){
  const i=(Math.floor(y/4)*width+Math.floor(x/4))*4;
  preview.pixels.set(native.pixels.subarray(i,i+4),(y*preview.width+x)*4);
 }
 fs.writeFileSync(path+'/preview4x.png',encodePng(preview));
 fs.writeFileSync(path+'/manifest.json',JSON.stringify({reviewOnly:true,source:'concept.png',sheet:[width,height],cell:[cell,cell],columns:width/cell,tiles:width*height/(cell*cell),palette:['#000000','#ffffff'],export:'Area-weighted fixed128 threshold, no dithering',layoutCompatibility:folder==='legacy16'?'Redrawn review positions; not a gameplay ABI replacement':'Eight columns; buildings use 2x2 cells'},null,2)+'\n');
 console.log(`PASS: ${folder} ${width}x${height}, ${cell}px cells`);
}
