// Review-only terrain and sprite compositing; native pixels remain binary.
export function habitatGround(scene, atlas) {
  const [width,height]=scene.size;
  const pixels=new Uint8Array(width*height*4);
  const {map,biome}=scene.terrain;
  const material=(x,y)=>Number(map[Math.max(0,Math.min(map.length-1,y))][Math.max(0,Math.min(map[0].length-1,x))]);
  const hash=(x,y)=>{let n=Math.imul(x+17,374761393)^Math.imul(y+biome*37+29,668265263);n=Math.imul(n^(n>>>13),1274126177);return (n^(n>>>16))>>>0;};
  const bayer=[0,8,2,10,12,4,14,6,3,11,1,9,15,7,13,5];
  for(let y=0;y<height;y++) for(let x=0;x<width;x++) {
    const cx=x>>4,cy=y>>4,lx=x%16,ly=y%16;
    const authored=material(cx,cy);
    let m=authored;
    // Four-pixel dithered transition apron around connected material regions.
    const edges=[[lx,cx-1,cy],[15-lx,cx+1,cy],[ly,cx,cy-1],[15-ly,cx,cy+1]].sort((a,b)=>a[0]-b[0]);
    const [distance,nx,ny]=edges[0];
    if(distance<4 && bayer[(y%4)*4+x%4] < (4-distance)*2) m=material(nx,ny);
    const si=((biome*32+y%32)*atlas.width+m*32+x%32)*4;
    let value=atlas.pixels[si];
    if(m===0 || m===1) {
      // Small local marks sit in otherwise bare white ground. No repeating texture carpet.
      const seed=hash(cx,cy),ax=3+seed%10,ay=3+(seed>>>8)%10,dx=lx-ax,dy=ly-ay;
      const accent=(seed>>>16)%5!==0 && (m===1
        ? ((dx===0&&dy===0)||(dx===1&&dy===0&&seed%3===0))
        : biome===0||biome===1
          ? ((dx===0&&dy===0)||(dy===-1&&Math.abs(dx)===1)||(dy===-2&&Math.abs(dx)===1&&seed%3===0))
          : ((dy===0&&(dx===0||dx===1))||(dy===-1&&dx===0&&seed%3===0)));
      value=accent?0:255;
    }
    const adjacent=material(nx,ny);
    if(distance===0 && authored!==adjacent && (authored===1||adjacent===1) && hash(x,y)%7===0)value=0;
    if(distance===0 && authored!==adjacent && (authored===2||adjacent===2))value=0;
    pixels.set([value,value,value,255],(y*width+x)*4);
  }
  return {width,height,pixels};
}
export function habitatProp(atlas,index) {
  const columns=atlas.width/32,sx=index%columns*32,sy=Math.floor(index/columns)*32;
  const image={width:32,height:32,pixels:new Uint8Array(4096)};
  for(let y=0;y<32;y++)for(let x=0;x<32;x++) {
    const si=((sy+y)*atlas.width+sx+x)*4;
    image.pixels.set(atlas.pixels.subarray(si,si+4),(y*32+x)*4);
  }
  // Remove only edge-connected white; enclosed highlights remain opaque.
  const exterior=new Uint8Array(1024),queue=[];
  const visit=(x,y)=>{if(x<0||y<0||x>=32||y>=32)return;const i=y*32+x;if(!exterior[i]&&image.pixels[i*4]===255){exterior[i]=1;queue.push(i);}};
  for(let i=0;i<32;i++){visit(i,0);visit(i,31);visit(0,i);visit(31,i);}
  for(let q=0;q<queue.length;q++){const i=queue[q],x=i%32,y=i>>5;visit(x-1,y);visit(x+1,y);visit(x,y-1);visit(x,y+1);}
  for(let i=0;i<1024;i++) if(exterior[i]) {
    const x=i%32,y=i>>5;
    // A one-pixel light rim separates dark object silhouettes from dark ground.
    const near=[[x-1,y],[x+1,y],[x,y-1],[x,y+1]].some(([nx,ny])=>nx>=0&&ny>=0&&nx<32&&ny<32&&image.pixels[(ny*32+nx)*4]===0);
    if(!near)image.pixels[i*4+3]=0;
  }
  return image;
}
