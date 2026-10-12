# 32×32 environmental tile-sheet spike

[Open the artwork viewer](world-environment-32-spike.html).

## All examples and asset-set comparisons on one page

The page shows the river, town, city, light cave and inverted dark cave together;
scene cards contain all three native128×64 views and enlarged inspection views.
The dark cave exchanges black and white in the terrain, then composites the
original player sprite unchanged, including its white outline. The camera
inspector copies this same composition to the game/native frame and draws its
outline afterward. No new art is generated.

At the owner's request, two Luna workers arranged comparable town/city/cave
examples from the existing classic16 combined atlas and native redrawv4/v5
atlases. Their original sheets stay untouched. Sources, placements, facing and
camera data are preserved in each set's `manifest.json` under
`assets/world-arranged-comparison/{classic16,redraw-v4,redraw-v5}/`.

The comparison section aligns four asset sets across town, city, light cave
and dark cave rows. Every primary crop is exactly128×64 and uses the original
masked16px player. Each set has its own authored layout; source cells and
assemblies differ, so these are comparable scene categories rather than
identical maps. Dark rows exchange cave scenery pixels while preserving every
opaque player pixel. Separate native dark PNGs are exported for all four sets.
Full arranged scene sheets appear below the table on the same page.

Download `assets/world-arranged-comparison/comparison-native.png` for the
616×328 contact sheet, or `comparison-2x.png` for its exact2× enlargement.
Four `{set}-scenes.png` files show complete town/city/cave layouts together.

```sh
node tools/arrange-classic16-spike.mjs
node tools/arrange-redraw16-spike.mjs
node tools/compose-environment32-scene.mjs
node tools/compare-world-arrangements.mjs
node --test tools/tests/environment32-scene_test.mjs tools/tests/environment32-viewer_test.mjs tools/tests/world-arranged-comparison_test.mjs
```

Existing generated source seams remain art-review items. This comparison changes
no installed sprites, tile IDs, map grid, firmware or packed cart.

## Player-scale expansion and arranged scene

### Town, city, cave and native-size display

The viewer now selects River village, Garden town, Market city or Crystal cave.
Each is a 384×256 authored arrangement with three player/camera presets.
Town uses compact cottages, gardens and a well. City deliberately places the
original larger shops along streets and a market square to expose how much
building art is cropped by the small screen. Cave reuses cliff, rock, column,
crystal and pebble tiles with white passage space. These are composition
examples, not new generated artwork or playable collision layouts.

The permanent **Actual pixel size · 1×** canvas is 128×64 CSS pixels and copies
the exact same framebuffer as the enlarged view. It never scales with the
inspection zoom selector (1×/2×/4×/8×). Player pixels remain 16×16. Physical
dimensions depend on monitor pixel density and browser zoom; this is native
pixel scale, not a calibrated OLED-size simulation. The red overview boundary
is drawn after copying both framebuffers so it never leaks into either view.

New sources and exports are in `scenes/{town,city,cave}/`. The gallery deep-links
to each via `?scene=town`, `?scene=city` or `?scene=cave`; all earlier scenes and
tile sheets remain accessible. Rebuild and verify all four examples:

```sh
node tools/compose-environment32-scene.mjs
node --test tools/tests/environment32-scene_test.mjs tools/tests/environment32-viewer_test.mjs
```

The 12 composition tests cover binary pixels, every tile placement and all
12 camera/player crops. Two viewer tests cover scene switching, native-size
frame parity, zoom independence and keeping the camera outline outside the
game frame. No firmware or packed artwork changes.

The current page opens on a river village scene with the actual masked 16×16
player and an exact 128×64 camera. It includes three player positions, camera
panning and click-to-center on the overview. These are composition controls;
movement and collision are not simulated.

The original 64-tile sheet is preserved. A new `expansion/atlas32.png` adds
16 cells in a 128×128 sheet: narrow paths, river center/banks, footbridge,
two-cell cottage, vegetable bed, bench, tree, hedge, boulders and sign.
The cottage is 64×32 versus the original 96×64 buildings. Land remains white;
the generated water uses dark dither, providing stronger river contrast.
The exact built-in imagegen prompt and source are preserved in `expansion/`.
New raw 1bpp pixel payload would be 2,048 bytes before headers or masks.

`scene.json` declares 67 tile placements, using IDs 1–64 from the original
sheet and 65–80 from the expansion. Artwork is 32px, but positions follow a
16px layout scale, allowing the cottage approach path to align to its door.
Unoccupied ground is white. `scene-terrain.png` is a 384×256 composition;
`scene.png` adds the player in the cottage lane. Three `viewport-*.png` files
contain exact 128×64 crops with the correct native facing. Enlarged PNGs use
nearest-neighbor pixels. No illustration generator is used to arrange the map.

```sh
node tools/export-environment32-spike.mjs docs/assets/world-environment-32-spike/expansion
node tools/compose-environment32-scene.mjs
node --test tools/tests/environment32-scene_test.mjs
```

Tests verify native binary pixels, tile-ID composition, camera crops and exact
masked player pixels. Generated water repeats have visible seams, and bridge
ends need a shore-specific transition before integration. No shipping assets,
map grid, firmware or packed cart changed. Earlier sections describe the first
sheet; its controls remain in the page's Tile sheets section.

The owner requested white base ground, 32×32 tiles, heavy dithering and a
Pokémon-style top-down JRPG view. This is a new review sheet, separate from the
installed 16px tiles and the earlier environment spike.

Reviewed `images/tiles.png` (256×768, 16px cells) and the current
`assets/world-environment-spike/atlas-native.png` (256×512, 512 16px cells).
The installed atlas has gray texture and large unused areas. The earlier spike
uses white ground, sparse black details and compact fixtures, but many small
props have little room for texture. Its lower half also has substantial blank
space. This pass concentrates the palette into 64 larger review cells.

`assets/world-environment-32-spike/atlas32.png` is 256×256: eight columns and
eight rows of 32×32 cells. Row themes are ground, foliage, stone, water, village
props, roofs/bridges, facades/docks and interiors. Cottage and shop are each
three columns across the roof and facade rows, making 96×64 assemblies.
White ground remains dominant: 45,632 white pixels and 19,904 black pixels.

The built-in imagegen source is preserved as `concept.png`; the exact generation
prompt is in `prompt.json`. The generator supplied fine detail rather than an
exact enlarged native pixel grid. Centered nearest-neighbor reduction initially
broke that texture into irregular marks. The final export uses area-weighted
sampling and 4×4 Bayer dithering at native pixel scale; the 4× preview uses
nearest-neighbor enlargement. This yields real binary pixel-scale dithering,
although dense details can still compete with object outlines. The concept is
the art reference; the native sheet shows what survives at the requested size.

Rebuild the review export with:

```sh
node tools/export-environment32-spike.mjs
```

The viewer offers 1×/2×/4× zoom and optional 32px cell boundaries. Sheet size,
opaque binary palette and preview composition are verified. Generated cell
alignment, seamless repeated terrain and assembly seams are not certified.
The vertical fence also reads more horizontally than requested; it needs a
targeted correction before use. No collision or entrance metadata is assigned.

At 1bpp, 64 tiles require 8,192 bytes of raw FX pixels before headers/masks.
Each 32px tile costs 128 bytes versus 32 bytes for a 16px tile. A 128×64 display
spans four by two 32px cells. The shipping player and movement grid are 16px;
adopting this sheet requires a separate decision on larger map cells versus
32px assemblies over the existing grid. This review adds no firmware, SRAM or
packed cart changes; no AVR build or integrated firmware gate was run.

## Creature habitats · 9uf

The current `data/json/creatures.json` roster has 64 creatures. This continuation groups 25 of them into four visual habitat proposals, using their current types and reviewed front sprites. Habitat membership is an artwork proposal, not an encounter-table or gathering-rule change.

| Area | Creature group | Shared visual language |
| --- | --- | --- |
| Huskfield Meadow | NibWeevil, HuskWeevil, GranaryWeevil, Tuftling, GustTuft, ScrambleSnail | Ribbed seedpods, grain heads, windseed fluff; harvest lane and granary |
| Dewcap Grove | DewBud, NectarBloom, Sporeling, SporeKeeper, MossMole, Skimray | Dew leaves, nectar flowers, mushroom caps, moss logs; pools beside a dry trail |
| Shellstone Tide | SkitterCrab, Conelet, Horseshoe, SiltSkorp, Platefin, SpireSquid | Shell armor and spiral fossils; exposed silt banks, reef pools and jetty |
| Cinderflint Quarry | ember, waggleworm, CoalGrub, CinderCocoon, FurnaceBeetle, Flintling, GeodeGuard | Coal insect silhouettes and faceted minerals; vents, timbers and geode pocket |

The built-in image-generation tool created a new 64-cell concept sheet, with two rows per area. `habitats/prompt.json` preserves the exact prompt and style reference; `habitats/concept.png` preserves the raw output. `habitats/atlas32.png` is the opaque binary 256×256 export (64 tiles, 32×32 each). Tile IDs 81–144 extend the review atlas; they are unrelated to shipping tile IDs. The initial isolated-prop layout was revised after review; connected terrain is described below. Prop alignment remains a visual study.

All four 384×256 arrangements appear on the existing HTML page, each with creature reference cards, three exact 128×64 crops at 1× and 2×, and a working camera inspector. The 48px creature sprites are shown separately at 2× for reference; they are not scaled into world actors. Every world view uses the existing 16×16 player. The earlier dark cave continues to invert only terrain.

Reproduce the habitat artwork exports and scene layouts:

```sh
node tools/export-environment32-spike.mjs docs/assets/world-environment-32-spike/habitats
node tools/export-environment32-spike.mjs docs/assets/world-environment-32-spike/habitats/terrain
node tools/author-creature-habitats.mjs
node tools/compose-environment32-scene.mjs
node --test tools/tests/habitat-terrain_test.mjs tools/tests/environment32-scene_test.mjs tools/tests/environment32-viewer_test.mjs tools/tests/world-arranged-comparison_test.mjs
```

Roster evidence: `data/json/creatures.json`, `docs/assets/battle-sprites48/manifest.json` and reviewed contact sheets. The expansion concept descriptions inform seed, dew, spore, fossil, coal and crystal motifs; their proposed gathering materials are not asserted as installed mechanics.


## Connected habitat terrain · ei6

Owner review rejected the blank white ground and disconnected stamps. The revised four habitats use sixteen full-surface 32×32 textures: each biome has ground, lighter walking surface, dark patch and dense border material. The new built-in imagegen source and exact prompt are preserved under `habitats/terrain/`. The native atlas is 128×128, binary opaque, with opposite edge pixels paired during export so each texture repeats continuously.

Each scene authors a 24×16 material map on a 16px placement scale. Connected winding paths and branches run through stubble fields, a moss pond, a tidal shoreline, and coal/flint patches. Four-pixel ordered-dither transitions soften boundaries. The same 32px surface pattern phase continues across neighboring cells. Ground is textured everywhere; light paths provide player contrast. This is a review renderer, not a shipping autotile format.

Prop composition removes edge-connected white while preserving enclosed white highlights and a one-pixel light rim. This prevents square white backgrounds from erasing the ground. Redundant stamped ground/water tiles were removed, leaving 31/33/18/34 props over the four continuous surfaces. The world player and all earlier scenes retain their original rendering.

Checks verify texture edge parity, nonblank ground coverage, connected paths, players on walking surfaces, exterior-mask compositing, native palette/crop parity and dark-cave player preservation. Creature grouping remains a visual proposal. Terrain pattern repetition is still visible at large map scale; these previews are intended for judging native 128×64 readability.

## Sparse ground and varied terrain · lou

The owner clarified that bare white with small details is useful; the goal is cohesion and variety rather than a continuous dither carpet. This revision supersedes the blanket ground treatment above. Open ground now has small irregularly positioned grass tufts, leaf hints, sand flecks or grit. Paths have still smaller marks and occasional boundary accents. No texture covers the open walking surfaces.

Dense materials are confined to smaller contiguous fields, pond/water areas, coal and mineral pockets. Each habitat has a different path profile. Masked props and material transitions remain, so white backgrounds do not erase surrounding scenery. The generated terrain atlas remains the source for dense regions; sparse accents are authored directly in the review renderer. No new image generation was needed.

Full composed scenes are 77.7% white (Huskfield), 74.9% (Dewcap), 67.6% (Shellstone, including its tidal water), and 76.6% (Cinderflint). Tests permit bare cells and verify sparse open-ground interiors, connected paths, unchanged player pixels and exact composition. The same single-page gallery and native128×64 inspector show the revised balance.

## Playable lure world · 15m

`make lure-demo` now renders the revised sparse dithered environment in the authored8×8 lure pocket at world x8–15,y0–7. The compact128×128 clearing has32px tree/mushroom/fern/log silhouettes on the non-walkable northern band, a thin connected boundary, open ground with small accents, and16px container/plant fixtures at the existing interaction cells. The container is(9,2), plant(14,2), and exit(12,7). The player still boots at(11,5).

`tools/export-lure-world-spike.mjs` builds `images/LureWorld_16x16.png` from the dedicated clarity atlas and sparse review terrain renderer. Owner screenshot feedback prompted quieter white surfaces, stronger contours and reduced shadow dithering. The built-in imagegen source/prompt and authored row bands are preserved under `habitats/lure-clarity/`; fixed-threshold native exports avoid extra Bayer noise. The two fixtures are fitted to16px during export and cropped at native size instead of averaged down. `make gen` invokes it before packing, and `gen-if-needed` runs it before checking the cache so art changes invalidate packed outputs. The export preserves a preview manifest and three native128×64 crops under `lure-world/`, also visible on the existing single-page gallery. Previews include the original neighboring map when the camera reaches outside the pocket and use the actual world player screen position(56,24).

The append-only `lureWorldTiles` FX field stores64 opaque16×16 render frames (2052bytes including its header). The opt-in `CGFX_LURE_WEDGE` renderer selects those frames by world coordinate; it still fills the property window from the original map words. Collision, box/plant hooks, scripts, encounter/lure zones and movement therefore retain their existing data. Default shipping world art keeps the original renderer. All745392 prior FX payload bytes and existing field offsets match the control; appending artwork moves the physical FX data start page, handled by the regenerated header/cart.

```sh
make gen
make lure-demo ARDENS=/path/to/Ardens
node --test tools/tests/lure-world-spike_test.mjs
```

Measured opt-in lure build:24386B program flash (control24278B,+108),1760B staticRAM (no delta),800B static free. Device tests verify exact map rasterization for idle and all four scrolling directions, original player masks and a real lure-zone encounter; map-render headroom659B and integrated painted stack headroom484B.
