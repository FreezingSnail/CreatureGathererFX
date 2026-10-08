# Environment and location tilemap spike

[Open the viewer](world-environment-spike.html).

This is a separate environment atlas and two authored review maps. Existing
installed atlases, maps, v4 and v5 redraws remain unchanged. The current version uses simple black outlines, white ground and sparse detail.
The earlier creature-style variant is retained for comparison.

The native atlas is 256×256: 256 tiles of 16×16 pixels. Rows 0–7 cover meadow,
woodland, coast, marsh, mountain, snow, village and ruins. Columns 0–3 in each row
are ground, path, water and solid terrain; columns 4–15 are environment props.
Rows 8–11 contain four 4×4 building assemblies: cottage, woodland cabin, shop and
stone tower. Rows 12–15 contain four 4×4 interior assemblies: cottage room, shop,
ruined shrine and crystal cave.

`assets/world-environment-spike/overworld.tmj` is a 32×32-tile environment sampler
with eight 16×8 areas, a connecting path, gathering props and building placements.
`interiors.tmj` is a separate 32×8-tile interior sampler. These are Tiled JSON maps
with an embedded tileset pointing to `atlas-native.png`; tile IDs are 1–256, with
0 reserved for empty tiles. Location rectangles are in the Locations object
layer. The PNG previews are assembled from those exact tile IDs, not generated
map illustrations.

The original built-in imagegen concept and targeted terrain correction are preserved.
Only the correction's first four columns of the first eight rows are accepted
into the native export, retaining all props/building/room pixels from the first
concept. Export uses centered nearest-neighbor sampling and a binary threshold.
Prompt and references are stored in `assets/world-environment-spike/prompt.json`.

Rebuild the earlier detailed exports with:

```sh
node tools/generate-world-environment-spike.mjs \
  docs/assets/world-environment-spike/atlas-concept-v1.png \
  docs/assets/world-environment-spike/atlas-terrain-correction.png
```

This does not generate or modify game artifacts. Native dimensions and exact map
composition are verifiable, but generated cell outlines and seams still require
art review. Collision, encounters and entrance transitions have not been assigned.
The map is a visual environment sampler, not a shipping world layout.

At 1bpp the entire atlas would require 8,192 bytes of FX pixel payload before
headers or masks. The two maps contain 1,280 tile IDs (2,560 bytes at 16 bits each).
These are raw size estimates, not measured changes to the packed game. Firmware
flash and SRAM are unaffected by this review-only spike.

## Simplified revision

The user identified boxed black prop backgrounds and excessive ornamentation.
The current revision uses plain white ground, simple black outlines and fewer
interior marks. Generated source is preserved as `atlas-simple-concept.png`.
The earlier native atlas, maps and previews remain under `v1/`, with checksums.
The HTML viewer has a Version selector for comparing both.

Rebuild the current simplified previews with:

```sh
node tools/generate-world-environment-spike.mjs \
  docs/assets/world-environment-spike/atlas-simple-concept.png \
  docs/assets/world-environment-spike/atlas-terrain-correction.png
```

The quiet terrain correction is reused in the first four columns of the top half
to preserve the ground/path/water/wall slots. Props, buildings and rooms come
from the new simplified concept.

## Player-scale building revision

Current exteriors are 4×3 tiles (64×48) inside their preserved 4×4 atlas slots:
first slot row blank, building starts in row 1. Doors occupy slot(column1,row3)
with the approach at(column1,row4), directly below. The first 48 source building
rows are reduced to 32 native rows; the final 16 doorway rows remain full height.
Only the building strip is replaced; terrain, props and interiors are retained.
The earlier simplified atlas/maps are saved under `v2/`; original detailed
exports remain under `v1/`. The HTML defaults to atlas and compares all versions.

Scale evidence: `src/engine/draw.h` declares PLAYER_SIZE16 and draws a16×16
characterSheet frame; `WorldEngine::moveChar` commits movement after TILE_SIZE
steps. `WorldEngine::interact` dispatches A to the adjacent faced tile.
`onStep` currently handles plants/encounters, not a doorway warp. The spike's
Entrance review objects therefore specify UP-facing A interaction; they are not
installed game events. The HTML entrance panels use the actual first player
frame, recolored black on transparent terrain, retaining all original ink pixels.

Use 1×1 tiles for small props, 2×2 for trees/large rocks, 3×3 or 4×3 for buildings;
large buildings can span more tiles when their function needs it. Tile size stays
16×16: the renderer reads ordinary map cells, not a new large-sprite mechanism.
Assemblies add FX asset/map data, not a new rendering feature. Repeated walls,
roof edges and ground should reuse tiles when installation is scoped.

Rebuild the current player-scale exports:

```sh
node tools/generate-world-environment-spike.mjs \
  docs/assets/world-environment-spike/atlas-simple-concept.png \
  docs/assets/world-environment-spike/atlas-terrain-correction.png \
  docs/assets/world-environment-spike/buildings-simple-concept.png
```

## Refined buildings and larger fixtures

The current fixture atlas is `fixtures/atlas-native.png`:256×256, 16×16 cells,
16 assembly slots of4×4 tiles. Buildings in the first four slots use compact
4×3 silhouettes, with pitched/asymmetrical roof shapes, eaves and broken tower
outline. A targeted first-strip correction fits roofs below the empty top slot
row and places the doorway in the second column. The first strip also replaces
the main atlas's building strip; all other main atlas pixels remain unchanged.
Earlier square buildings/maps are retained under `v3/`.

Assemblies add horizontal and vertical wooden bridges, pond/shoreline, cascade,
broadleaf tree, tree grove, boulders, shrine arch, fishing dock, marsh boardwalk,
gathering camp, and well/garden. Land fixtures use white ground; water fixtures
use black water. Bridge ends and middle cells are candidates for reuse and span
extension; their repeated seams still need art review. Assemblies are not
single-tile icons. Empty/duplicate source tiles can be deduplicated later.

`fixtures/overworld.tmj` is a32×32 showcase with two embedded tilesets:main atlas
firstgid1 and fixture atlas firstgid257. It assembles all16fixtures with appropriate
ground/river contexts; PNG is composed from these IDs. The HTML defaults to this
atlas, retains previous versions, and offers the assembled showcase and individual
fixture crops. Interiors are disabled when browsing the fixture-only set.

Rebuild current fixture/main building previews:

```sh
node tools/generate-world-fixture-spike.mjs \
  docs/assets/world-environment-spike/fixtures/atlas-concept.png \
  docs/assets/world-environment-spike/fixtures/buildings-correction.png
```

The tool reads the preserved `v3/atlas-native.png` baseline for the unchanged main
tiles. It never modifies installed game files. Additional raw fixture pixel
payload is8192bytes at1bpp before headers/masks; this is FX artwork, not firmware
code. No measured game flash/SRAM change. Source prompts are in
`fixtures/prompt.json`. Tile-ID composition is checked; art-cell alignment,
bridge repetition and shoreline transitions still need review before integration.

## Single combined atlas — current export

The published atlas is now `atlas-native.png`,256×512:16columns×32rows of16×16
cells,512tiles. Environment IDs1–256 remain in the top half; fixture IDs257–512
remain in the bottom half. No tile ID changes are needed. Both environment maps
and the fixture showcase now use a single Tiled tileset pointing to this sheet.
The old component PNGs are retained as source/history,not separate published
map tilesets. The split-atlas snapshot is under `v4/`.

The viewer opens on the combined atlas. Overworld, interiors and Fixture showcase
are views of maps using the same atlas. Fixture gallery crops now read the lower
half of the combined sheet. The current rebuild command above publishes this
combined output automatically; prior section dimensions describe earlier stages.
Map pixels are unchanged by packing. Raw1bpp payload remains16384bytes total,
the same sum as the two prior8192byte sheets; no game/cart/firmware change.

## Classic top-down player scale — current revision

The64×64 illustrative fixtures were too large for this128×64screen. Ourplayer
frame and movement cell are16×16,so the viewport spans8×4cell lengths. The
[Red/Blue sprite facing data](https://github.com/pret/pokered/blob/master/data/sprites/facings.asm)
positions four8×8sprite pieces at(0,0),(8,0),(0,8),(8,8),forming16×16.
[Movement source](https://github.com/pret/pokered/blob/master/engine/overworld/movement.asm)
uses16-pixel coordinate/step conventions. These support the player-relative
approach; the sizes below are this project's design choices,not claims about all
Pokemon Blue building sizes. Ourmap grid remains16×16 regardless of Game Boy
hardware tile/block formats.

Current targets:building3×2tiles(48×32),tree2×2,grove3×2,boulders2×1,shrine2×2,
pond3×2,cascade2×2,pier2×2,camp2×2,well/garden3×2. Bridge decks are one walking
tile wide; span length can vary. Roof/tree crown is viewed from above,with a
shallow building front face; ground stays quiet. The512-tile combined atlas and
IDs remain. Earlier large-fixture version is preserved under `v5/`.

Generated subject rows were32–73,83–135,144–188,196–249 rather than exact64px
quarters. The exporter now locates the four separated occupied bands,extracts
complete subjects,and fits them into declared native footprints. This avoids
clipped roofs and doors. Land pixels outside footprints are white; bridge decks
are extracted and fitted to one-tile walking width over reused quiet water.
Footprints/offsets are recorded in fixtures/manifest.json. Rendering and source
art seams remain review items before installation.

The HTML includes a128×64browser movement preview using actualplayer/camera
origins(56,24). Village lane,one-tile bridge and woodland gap let the user move
with arrows and test the faced door with A. Scenery collision in these review
scenes follows tile footprints. This is a browser prototype,not installed game
movement/warp code. The generated static PNGs show the exact same camera crop.

Rebuild the current atlas and scale scenes:

```sh
node tools/generate-world-fixture-spike.mjs \
  docs/assets/world-environment-spike/fixtures/atlas-concept.png --classic
node tools/generate-world-traversal-spike.mjs
```

No additional tile IDs or pixel payload beyond the existing singleatlas;no
firmware,save or packedcart changes. Prompts/sources for earlier artwork remain
inside their preserved version folders.

## Smooth motion and people

The browser preview now scrolls through each16-pixel step over approximately308ms
(one native pixel per52Hz frame). Camera and people move together; player remains
at(56,24). Hold arrows to chain steps; release finishes the current step. Collision
is checked before movement, and A interactions wait until the step finishes.
Changing scenes cancels movement; losing window focus stops held-key walking.

Eight original NPC review sprites are exported as binary16×16 cells in
`assets/world-npcs/sprites16.png` (64×32):gatherer,farmer,shopkeeper,herbalist,
fisher,ranger,miner,child. The gatherer replaces the arrow in movement and doorway
previews. Five stationary NPC placements demonstrate collision and A dialogue.
These are down-facing idle frames; walking/directional animation is not included.
Generated concepts and both exact generation prompts are preserved beside the
native assets. Native exporter crops occupied bounds, fits within16×16, samples
areas and thresholds to black/white; finer concept details may disappear.

```sh
node tools/generate-world-npc-spike.mjs
node tools/generate-world-traversal-spike.mjs
node --test tools/tests/world-traversal-viewer_test.mjs
```

The eight frames would occupy256bytes of raw1bpp pixels if later installed,
before headers/masks/directional frames. This spike changes no firmware or FX
cart bytes. Terrain still uses its single512-tile atlas; NPCs are a character sheet.

NPC transparency revision: each16×16cell now reserves a1pxwhite outline around
its silhouette, including corners/tools. Remaining exterior pixels have alpha0;
white interior regions stay opaque. Native figures fit within14×14before the
outline so it cannot clip against cell edges. Both canvas and staticPNGpreview
compositing preserve underlying terrain outside the mask. Sheet dimensions and
movement cell size remain unchanged. Device installation would require masking;
the earlier256byte estimate described pixel data only, not a transparency mask.

## Chibi player · four directions

Player now has four distinct16×16idleframes:DOWN(front),UP(back),LEFT,RIGHT.
The32×32player sheet is `assets/world-player/sprites16.png`;individual frames,
nearest8×preview,originalgeneratedconcept and exactprompt are in the samefolder.
Proportions emphasize a largerhead,shortbody and tinyfeet. Eachframe retains the
1pxwhiteoutline and transparentbackground. NPC artwork is unchanged.

The browser selects the matchingframe when movement begins or a blockedmove
turns theplayer. Scene changes restore initialfacing. Doorwaypreview usesUP;
statictraversalPNGpreviews use eachscene's declaredfacing. These are directional
idleframes,not a walkcycle. Camera still scrolls smoothly on movement.

```sh
node tools/generate-world-npc-spike.mjs --player
node tools/generate-world-traversal-spike.mjs
node --test tools/tests/world-traversal-viewer_test.mjs
```

Browserreview only;no firmware/FXcart changes. Four16×16frames represent128bytes
raw1bpp pixels,plus128bytes for a1bpptransparency mask if installed uncompressed.

## Walking cycle

The player now alternates two walking poses per direction, synchronized to each
16px camera step:poseAfor pixels0–7,poseBfor pixels8–15. Eachpose lasts roughly
154ms at the current308msstep. Heldmovement repeats thecycle;releasedmovement
finishes thecurrenttile thenreturns to thecorrect idlepose. Blockedmovement
turns inidle;scenechanges cancelwalking. This is a two-posewalkcycle with idle
at rest,not extra flashing/bobbing of the whole sprite.

Nativewalking sheet:`assets/world-player/walk/sprites16.png`,64×32,eight16×16
cells ordered DOWN_A,DOWN_B,UP_A,UP_B,LEFT_A,LEFT_B,RIGHT_A,RIGHT_B. Existingidle
andNPC sheets remain separate andunchanged. Originalconcept/exactprompt,
individualframePNGs,8×preview andmanifest live beside thewalkingsheet.

```sh
node tools/generate-world-npc-spike.mjs --player --walk
node --test tools/tests/world-traversal-viewer_test.mjs
```

Browser-only;no firmwareflash/FXcart change. If installed uncompressed,the eight
walkingframes would add256bytes1bpppixels plus256bytes1bppmask,beforeheaders.

## Front/back repair and game installation

The originalwalking artwork changed UP/DOWN head/torso betweenposes. Native
front/back frames now retain idleblack/white pixels throughrow12 inclusive.
Onlyfootrows13–14 alternate:one foot ends at13,the other at14,and the nextpose
swaps them. Thechest/backpack stays present. Outline masks are rebuilt after
compositing so the legs retain1pxwhitecoverage. Originalgeneratedwalkconcept
and correctionconcept remain as references. Nativefix takes precedence over
independentresizing. Sidewalking frames retain their originalartwork.

Rebuild the corrected authored PNG input,then use the project's generationentry:

```sh
node tools/generate-world-npc-spike.mjs --player
node tools/generate-world-npc-spike.mjs --player --walk --front-back
node tools/generate-world-npc-spike.mjs --player --walk
node tools/export-world-player-game.mjs
make gen
```

The chibiplayer is now connected to the shippingworldrenderer. Its appendedFX
field stores12maskedframes inDirectionenum order UP,RIGHT,DOWN,LEFT,withidle/A/B
for eachdirection. This preserves existingFXdeclarations and data. Eachframe
is16×16;packedfield size772bytes (768pixel/maskpayload plus4dimensionbytes).
The renderer derives itsframe directly fromWorldMotion direction andstep;
there is no additionalRAM animationstate. ExistingNPC/map browserassets remain
reviewexports;this installation concerns the player.
