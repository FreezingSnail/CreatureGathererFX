#!/usr/bin/env node
// Review-only habitat composition sources, tied to the authored creature roster.
import fs from 'node:fs';
const base='docs/assets/world-environment-32-spike';
const roster=JSON.parse(fs.readFileSync('data/json/creatures.json'));
const sprites=JSON.parse(fs.readFileSync('docs/assets/battle-sprites48/manifest.json')).sprites;
const groups=[
 {slug:'huskfield',name:'Huskfield Meadow',ids:[32,33,34,53,54,2],description:'Seed weevils share ribbed pods and grain stores with wind tufts and ScrambleSnail. Tall stalks shelter a worn dirt harvest lane; loose husks and seed fluff gather at the field edges.',motifs:'Ribbed husks, grain heads, windseed fluff and a small granary.',views:['Granary approach','Seedpod clearing','Windseed edge'],
 tiles:[[8,32,32],[7,64,64],[10,96,32],[0,144,32],[1,176,32],[2,208,32],[0,240,32],[4,304,32],[5,336,32],[9,32,96],[9,64,96],[9,96,96],[3,144,96],[6,176,96],[4,240,96],[5,272,96],[3,320,96],[0,32,176],[1,64,176],[2,96,176],[0,128,176],[4,176,176],[5,208,176],[4,240,176],[6,304,176],[11,336,176],[13,32,208],[13,64,208],[13,96,208],[13,128,208],[12,176,208],[12,208,208],[4,240,208],[5,272,208],[3,320,208]]},
 {slug:'dewcap',name:'Dewcap Grove',ids:[35,36,37,38,39,18],description:'DewBud and NectarBloom favor the wet edge, while Sporeling, SporeKeeper and MossMole occupy the shaded logs. Skimray connects the plant theme to the pools. A leaf-litter path threads between mushroom groves.',motifs:'Dew leaves, nectar blooms, spotted caps, mossy logs and shallow pools.',views:['Mushroom trail','Dew pool','Moss den'],
 tiles:[[0,32,32],[1,64,32],[2,96,32],[6,144,32],[7,176,32],[5,224,32],[0,288,32],[1,320,32],[2,352,32],[1,32,96],[3,64,96],[4,96,96],[8,160,96],[9,192,96],[3,224,96],[12,256,96],[9,288,96],[3,320,96],[0,32,176],[5,64,176],[13,96,176],[6,144,176],[7,176,176],[1,224,176],[0,256,176],[4,304,176],[2,336,176],[2,0,224],[0,32,224],[1,96,224],[6,144,224],[3,208,224],[4,240,224],[14,304,224],[0,352,224]]},
 {slug:'shellstone',name:'Shellstone Tide',ids:[3,40,42,44,46,41],description:'SkitterCrab, Conelet and Horseshoe inhabit the shell banks; SiltSkorp and Platefin suit the silt pools. SpireSquid gives the old spiral fossils an elder connection. Textured shell sand separates the exposed fossil beds from the water.',motifs:'Spiral fossils, armored shells, tidal pools, layered reef stone and a short jetty.',views:['Shell bank','Fossil bed','Tide landing'],
 tiles:[[0,32,32],[1,64,32],[2,96,32],[3,160,32],[14,192,32],[15,224,32],[4,288,32],[5,320,32],[1,32,96],[0,64,96],[3,96,96],[12,160,96],[2,192,96],[1,224,96],[6,288,96],[4,320,96],[8,0,176],[8,32,176],[8,64,176],[8,96,176],[8,128,176],[9,160,176],[13,224,176],[6,288,176],[7,320,176],[10,0,208],[10,32,208],[10,64,208],[10,96,208],[10,128,208],[10,160,208],[10,192,208],[13,224,208],[6,288,208],[3,336,208],[11,320,224]]},
 {slug:'cinderflint',name:'Cinderflint Quarry',ids:[10,16,48,49,50,51,52],description:'CoalGrub, CinderCocoon and FurnaceBeetle form the hot-soil family. Ember and WaggleWorm fit the vents, while Flintling and GeodeGuard inhabit the cooler mineral face. Packed grit lanes connect the coal working to the crystal pocket.',motifs:'Coal, clinker, vents, faceted geodes and abandoned mine timbers.',views:['Mine threshold','Coal working','Geode pocket'],
 tiles:[[0,0,32],[7,32,32],[8,64,32],[9,96,32],[0,144,32],[1,176,32],[2,208,32],[3,256,32],[4,288,32],[5,320,32],[7,352,32],[9,32,96],[15,64,96],[10,96,96],[1,144,96],[2,176,96],[6,208,96],[3,256,96],[5,288,96],[4,320,96],[0,0,176],[7,32,176],[14,64,176],[14,96,176],[1,144,176],[12,176,176],[2,208,176],[3,256,176],[4,288,176],[5,320,176],[7,352,176],[0,32,224],[7,64,224],[6,96,224],[11,144,224],[12,176,224],[13,208,224],[3,256,224],[5,288,224],[4,320,224]]}
];
for(const [g,group] of groups.entries()) {
 // Stagger vegetation/mineral clusters; retain clear paths at the three player positions.
 for (const tile of group.tiles) {
   const [id,x,y]=tile;
   if(y===32 && id!==8 && id!==9) tile[2]=[0,16,32][Math.floor(x/32)%3];
   if(y===96 && !(g===2 && id===12)) tile[2]=[80,96,80][Math.floor(x/32)%3];
   if(y===176 && !(g===2 && [8,9,13].includes(id))) tile[2]=[176,160,176][Math.floor(x/32)%3];
 }
 const terrainMap=Array.from({length:16},(_,y)=>Array.from({length:24},(_,x)=>{
   let m=0;
   if(g===0 && ((y<3 && x>9 && x<19) || (y>11 && x<9)))m=2;
   if(g===0 && y>11 && x>16)m=3;
   if(g===1 && ((x-14)**2/36+(y-5)**2/9<1))m=2;
   if(g===1 && ((x<2 && y>8) || (x>21 && y<7) || (y>13 && x>15)))m=3;
   if(g===2 && y>=11+Math.round(Math.sin(x/4)))m=2;
   if(g===2 && y<3 && x>12 && x<20)m=3;
   if(g===3 && ((y<2 && x<12) || (x<6 && y>12)))m=2;
   if(g===3 && x>18 && (y<4 || y>12))m=3;
   const centers=[[7,8,9,8,7],[6,7,8,9,10],[9,8,8,8,7],[6,7,8,8,9]][g];
   const center=centers[x<6?0:x<11?1:x<16?2:x<21?3:4];
   if(Math.abs(y-center)<=1 || (x>=6&&x<=8&&y>=3&&y<=8))m=1;
   return String(m);
 }).join(''));
 const excluded=[[13,14,15],[8,10,11,12,15],[6,7,8,9,10,11],[11,12,13,14]][g];
 group.tiles=group.tiles.filter(([id])=>!excluded.includes(id));
 const start=81+g*16;
 const creatures=group.ids.map(id=>{const c=roster.find(c=>c.id===id);const s=sprites.find(s=>s.id===id&&s.view==='front');if(!c||!s)throw Error(`Missing creature ${id}`);return {id,name:c.name,types:[c.type1,c.type2].filter(t=>t!=='none'),sprite:'assets/battle-sprites48/'+s.file};});
 const scene={name:group.name,slug:group.slug,reviewOnly:true,size:[384,256],artTileSize:32,movementScale:16,viewport:[128,64],playerSize:[16,16],tileIds:'81–144 masked habitat props; layered connected terrain',terrain:{biome:g,cellSize:16,sheet:'habitats/terrain/atlas32.png',materials:['ground','path','patch','border'],map:terrainMap},description:group.description,motifs:group.motifs,creatures,placements:group.tiles.map(([id,x,y])=>[start+id,x,y]),views:group.views.map((name,i)=>({name,player:i===0?[112,64]:[56+i*128,136],facing:['up','right','down'][i],camera:i===0?[64,32]:[i*128,112]}))};
 const root=`${base}/scenes/${group.slug}`;fs.mkdirSync(root,{recursive:true});fs.writeFileSync(root+'/scene.json',JSON.stringify(scene,null,2)+'\n');
}
fs.mkdirSync(base+'/habitats',{recursive:true});
fs.writeFileSync(base+'/habitats/groups.json',JSON.stringify(groups.map(({tiles,...g})=>g),null,2)+'\n');
console.log('Authored four habitats from current creature IDs and front sprites.');
