# Fresh overworld pass

[Open the fresh pass in the combined viewer](overworld-explorations.html#fresh-round).

Three art directions explore soft blocks, ink silhouettes and stepped
etching. Each now has a base sprite sheet, a modern-city/water/desert
extension sheet and a mountain/marsh/ruins/snow/volcano sheet. All were generated with built-in `image_gen`, with
no previous spike images supplied as references. Source PNGs are saved in
`assets/overworld-fresh/sources/`; the [base prompt set](assets/overworld-fresh/sources/prompts.json)
and [extension prompt set](assets/overworld-fresh/sources/extension-prompts.json), plus the
[five-biome prompt set](assets/overworld-fresh/sources/biomes-prompts.json),
are preserved alongside them.

Each direction becomes the same 295-tile kit. The previous 190 tile IDs stay
stable. Ground tiles are 16×16; object pieces retain binary transparency.
Shared sources produce connected paths, ocean coastlines, oasis banks,
cave edges, roads, crosswalks, curbs, concrete pavers, mountain trails,
marsh banks, frozen ponds and lava channels.

The world expands to 96×60 tiles (1536×960 pixels), with separate ground and
object layers. Fifteen 128×64 camera crops cover the five new biomes, modern city streets, tower tops,
waterfront, islands, desert, oasis and the original four locations. Every
crop places the game's down-facing 16×16 player at `(56,24)`. The viewer
shows all nine combinations for every area in a 3×3 comparison: Soft
blocks, Ink silhouettes and Stepped etching in columns; clean, light and
heavy textures in rows. It offers native 1× and enlarged views plus a
tile-grid overlay. Separate style and texture selectors control the full
world, atlas and source previews below the comparison.

The modern district uses glass towers, apartments and flat storefronts.
Its pavement uses staggered 16×8 pavers with a little aggregate texture;
curbs keep a clear border along the road.
Tower prefabs reuse the same roof, floor and entrance tile rows to build
short and tall versions; the tallest is eight tiles (128 pixels). Their
row selections are recorded under `prefabs` in the manifest. Street-level
and tower-top camera crops show complementary parts of the taller buildings.

The coastal region adds piers, boats, palms, reeds and islands. Desert
pieces add cactus, sandstone rocks, mesas, adobe houses and dry shrubs;
palms and water-bank pieces also form the oasis.

The new regions extend the same modular kit:

| Region | Reusable pieces and visual test |
| --- | --- |
| Mountain pass | Repeated 2×2 cliff blocks form long terraces; paired stair strips and a bridge connect the trails. Tests height and ledges at player scale. |
| Marsh | Mud-to-water connection masks, bare trees, reeds and reused pier sections form boardwalks. Tests separating paths from shallow water. |
| Ancient ruins | 3×3 arches, narrow broken columns, repeating low walls and worn courtyard paving. Tests large structures and clear openings. |
| Snowy highlands | White wind-marked ground, dark snow-capped pines, exposed rocks and connected cracked ice. Tests sparse marks on a bright biome. |
| Volcanic badlands | Dark basalt plates, connected bright lava, steam vents, pale scree paths and stone crossings. Tests reversed terrain contrast. |

The original 64×40 map occupies the northwest. Mountains and snow fill the
new eastern strip; marsh, ruins and volcano fill the southern row. All
camera crops read the same world layers, and every variant shares tile IDs
and placement. These are visual studies; terrain collision and elevation
rules have not been added to the game.

| Treatment | Gray shadow conversion |
| --- | --- |
| Clean | White; black contours stay fixed |
| Light | 25% black ordered texture |
| Heavy | 50% black ordered texture |

Conversion happens on reusable sprite assets before they are split into
tiles and assembled into maps. It does not dither a completed scene.

Every direction/treatment folder contains its native atlas, Tiled tileset,
layered world TMJ, world PNG and fifteen native camera PNG/TMJ pairs: 135 viewport
images and 144 Tiled maps across the nine variants. The layout and tile IDs
are shared across all versions. These are review exports; adopting them
into firmware is a separate change.

Rebuild the exports with:

```sh
node tools/generate-overworld-fresh.mjs
```
