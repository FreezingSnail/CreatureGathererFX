# Battle presentation research

Research spike: CreatureGathererFX-11u, 2026-10-06 local time.

Recommend persistent, stationary HP panels, a short visible HP drain/refill,
and a six-pixel diagonal stagger with simple grounding marks. Improve the
connection between the hit and the health loss before expanding move art.
The retained prototype changes playback only with `CGFX_BATTLE_HP_SPIKE`;
the proposed HUD/composition is a follow-up contract, not implemented firmware.

## References and observations

The following are references, not assets to import. No Pokemon artwork is
included in the game or the layout schematic.

| Reference | Identifiable frame/section | Observation | Adaptation |
| --- | --- | --- | --- |
| [Pokemon Blue opening battle screenshot](https://en.wikibooks.org/wiki/File:Criticalhit.JPG) | Static frame: level-5 Bulbasaur versus Squirtle, `Critical hit!`, player HP `10/19`; file history identifies Blue | Opponent front sprite occupies the upper right; player back sprite the lower left. Names and bars have consistent identities; player gets exact HP; feedback has its own box. | Preserve our art's player-right orientation, but stagger it vertically; keep exact player HP visible through feedback. |
| [The same opening-battle frame hosted with Blue](https://www.konsolenkost.de/gameboy-pokemon-blue-edition-gotta-catch-em-all-englische-version-mit-ovp-gebraucht_1002637_4102/) | Listing image `1002637-5.jpg`, same `10/19` / `Critical hit!` frame | Empty bar track remains identifiable beside the remaining fill. | Draw an outlined track even at zero; never imply that a live 1-HP creature is fainted. |
| [Crystal HP animation implementation](https://github.com/pret/pokecrystal/blob/master/engine/battle/anim_hp_bar.asm) | `ShortHPBar_CalcPixelFrame` / `LongAnim_UpdateVariables` | HP presentation is a separate animated operation, rather than just writing a final number. | Use bounded elapsed-time interpolation of existing before/after facts. |
| [Crystal documented HP animation defects](https://pret.github.io/pokecrystal/bugs_and_glitches.html#hp-bar-animation-is-slow-for-high-hp) | High-HP slowness and adjacent low-HP off-by-one sections | HP-proportional pacing and low-HP rounding have real failure cases. | Fixed-duration animation, integer endpoint tests, minimum nonzero fill. |
| [Red/Blue battle animation implementation](https://github.com/pret/pokered/blob/master/engine/battle/animations.asm) | `DrawFrameBlock`, `AnimationSlideMonDown`, `AnimationSlideEnemyMonOff` | Animation has timed frame blocks, directional transformations and explicit creature exits. | Our proposal selects the affected creature, keeps HUD still, and derives bounded reaction offsets from elapsed time. |

Static frame identities are supplied rather than invented video timestamps.
Timing below is our proposed 52-fps contract, not a measurement of Pokemon.
Grounding marks are a proposed solution to this game's flat composition;
they are not claimed to appear in the cited Blue frame. The 160x144 Game Boy
screen has substantially more vertical room than our 128x64 display.

## Audit of this repository

`src/engine/draw.h` uses 32x32 sprites at `(0,0)` and `(96,0)` and 30x2 HP
fills. `battleHpBarWidth(1,100)` is zero. HP has neither a numeric value nor
an explicit outline for the empty track. `drawScene` intentionally omits
background art; its shared offset moves both creatures and both bars.

`BattlePresenter` already retains a borrowed `ActionResult`, elapsed stage
time, two displayed HP bytes and one prepared item. Announce lasts 42 ticks,
Impact 21, Consequence/Faint 31, Terminal 52; A leaves a ten-tick remaining
dwell. The normal path assigns final HP at Impact entry. Sequential EndTurn
facts preserve tick endpoints, including mixed damage/healing with no net
change. Existing tests explicitly pin the immediate HP change, so enabling
drain in production requires replacing those timing assertions deliberately.

`BattleView` has species/HP/maxHP but no level or persistent status labels.
Do not expand it for decorative fields. Session is 163 B, presenter 24 B,
battle mode 187 B, and the world-backed ModeState is 191 B: only four bytes
of union slack. Persistent names need two 24-bit addresses (6 B); allocate a
separate six-byte HUD cache rather than silently enlarging the mode union.
Name widths are already PROGMEM lookups, not per-frame FX table reads.

Asset/generator inspection:

- `images/NewecreatureSprites_32x32.png` is 64x1024: two columns, 32 species,
  row-major frame `id*2` for opponent and `id*2+1` for player. These are paired
  directional silhouettes, not a promised Pokemon back-sprite set. Preserve
  their orientation. Species 0 is a valid raw asset; 255 is absent and must
  never index the table. Party emptiness must follow the existing party contract.
- Beam/wave sheets are 256x32, eight 32x32 frames. The native encoder in
  `CreatureGathererTools/crates/core/src/fxpack/{sprite,sprite_source}.rs`
  derives frame sizes from filename suffixes, traverses rows then columns,
  packs page-major bytes, and interleaves pixel/mask bytes when transparency
  exists. Each masked 32x32 frame occupies 256 B; no per-frame dimension header.
- `Blit::draw` consumes headerless pixel addresses and explicit dimensions.
  Current generated creature symbols point at those pixels. The old presenter
  comment mentioning a +2 prefix is stale for this renderer. `fontTrimmed+4`
  is a separate font field's framing; do not generalize it to creature art.
- Name rasters retain a leading blank five-pixel glyph and are eight pixels
  high. Truncation must not alter a multi-frame stride. HUD names use frame 0,
  skip that leading blank (`address+5`), then clip a single-row raster to the
  agreed width. Leave the feedback panel's complete names unchanged.
- Move 0 is valid; 32 is legacy empty, 35 is an authored gap, 255 is absent,
  and semantic Deluge 44 maps to packed row 32. Animation selection remains
  coordinated with CreatureGathererFX-jp8.3.18, not a new move mapping here.

## Layout comparison

[Open the original-art layout schematic](battle-presentation-layouts.svg).
The schematic uses the game's first creature pair and illustrative text;
the table below, not the browser's font rasterization, pins device geometry.

Coordinates use `(x,y,width,height)` with exclusive right/bottom bounds.
Both options reserve `(0,40,128,24)` for the existing black-on-white feedback
or menu. The move picker may temporarily replace the upper scene with its
existing selected-move/PP panel; do not draw a battle HUD over that panel.

| Element | A: keep sprite positions | B: recommended small diagonal |
| --- | --- | --- |
| Opponent sprite | `(0,0,32,32)` | `(2,1,32,32)` |
| Player sprite | `(96,0,32,32)` | `(94,7,32,32)` |
| Opponent name | `(34,0,60,8)` | `(40,1,50,8)` |
| Opponent HP label | `(34,9,10,6)` | `(40,9,10,6)` |
| Opponent outer track | `(47,9,44,5)` | `(53,9,36,5)` |
| Opponent fill area | `(48,10,42,3)` | `(54,10,34,3)` |
| Player name | `(34,18,60,8)` | `(40,19,50,8)` |
| Player HP label | `(34,27,10,6)` | `(40,27,10,6)` |
| Player outer track | `(47,27,44,5)` | `(53,27,36,5)` |
| Player fill area | `(48,28,42,3)` | `(54,28,34,3)` |
| Player current/max HP | `(42,34,42,6)` | `(44,34,42,6)` |

Option A gives longer bars/names and the least layout churn, but leaves both
creatures on one visual plane and has no two-pixel horizontal shake margin.
Option B retains full 32x32 art, adds a small depth cue and bounds reaction
motion. Prefer B. No full field bitmap: its texture would compete with the
small HUD and cost FX reads. The optional code-drawn ground marks are the
two horizontal stepped strokes `(6,32,24,1)` / `(10,33,16,1)` and
`(98,38,24,1)` / `(102,39,16,1)`, drawn behind masked sprites. No new bitmap
is required for this first composition slice.

## Frozen HUD and animation contract

HUD names show at most ten 5-pixel characters in B. For longer names show
nine plus a code-drawn ellipsis in the final five columns; no scrolling.
Draw in WHITE on the black scene. Empty/absent names draw no raster.
Names stay associated with the outgoing creature until switch Impact, then
the incoming creature. Read two name addresses at battle entry and replace
only the switched side's address at the switch transition. Backing out of a
party/move submenu invalidates no HUD cache. Keep the cache separate from
the party picker's `creatureNameAddresses` so submenu opens cannot overwrite it.

Player numeric HP uses up to three digits per value, one slash and six-pixel
advance: `255/255` occupies 42 pixels. Suppress leading zeroes, center the
actual string within that box. Existing `drawGlyph` cannot draw slash (it
rejects characters below `0`), so draw its 5x6 diagonal directly into the
buffer. Draw `HP` with supported glyphs. Opponent shows a bar without numeric
HP to save space and keep player decisions focused on exact own health.
Actual damage continues to appear in the stationary feedback panel.

For a bar of inner width W=34: clamp HP to maxHP; if maxHP or HP is zero,
fill is zero; otherwise `max(1, floor(uint16_t(hp)*W/maxHp))`. Full HP fills
W. Keep a one-pixel white border and black empty interior. Mark live low HP
when `uint16_t(hp)*4 <= maxHp` with a stationary code-drawn exclamation at
`(90,28,2,5)` for player or `(90,10,2,5)` for opponent (top 3 pixels, gap,
bottom dot). This is a stable shape cue, not color or flashing.

| Stage | Natural ticks at 52 fps | Visible behavior |
| --- | --- | --- |
| Announce | 42 (~808 ms) | Move name/text; animation frames 0..7 remain monotonic. For physical attack only, actor leans 1 px toward target during last six ticks. |
| Impact reaction | elapsed 0..6, within the 21-tick stage | HP remains at prior endpoint. Target x offsets `-2,+2,+1,-1,+1,-1,0`; y is fixed. Actor and HUD stay still. Self-hit selects actor. No reaction for zero damage. |
| Impact HP motion | elapsed 7..20 (~269 ms between endpoints) | Drain/refill interpolates using 14 steps; target number/bar agree; final endpoint is displayed at elapsed 20. |
| Consequence | 31 (~596 ms) per successful fact | Existing semantic feedback, no re-resolution. EndTurn HP facts each receive their own 21-tick Impact. |
| Faint | 31 (~596 ms) | Zero HP/empty track remains visible; last six ticks suppress alternating sprite columns, then hide the creature at stage exit. No vertical slide into the feedback panel. |
| Switch | 42 announce + 21 impact | Outgoing identity remains through announce; swap to fresh view at impact; incoming sprite reveals alternating columns for first six ticks, fully visible thereafter. No HP interpolation between different creatures. |
| Terminal | 52 (1 s) | Existing terminal feedback before mode exit. |

Derive effects from elapsed time; no new actor-motion state or pose assets.
Physical and special effects retain separate groups and cached metadata.
An effect sprite must be clipped to scene bounds, drawn before HUD, and its
anchor must follow the actor/target rather than retain the old middle strip.

The prototype computes `moved=floor(abs(after-before)*step/14)` with
`step=clamp(elapsed-6,0,14)`, then adds/subtracts without unsigned underflow.
It reconstructs a tick's start from earlier HP facts of the same side; at most
three prior facts are examined. It reads no cart metadata and mutates no result.
No result copy, queue, local framebuffer or persistent byte is added.
Presentation HP is never written back to battle/session state.

A fresh A edge retains the existing ten-tick remaining-dwell rule and can
jump the curve forward; held A alone does not create new edges. It never
skips settlement, skips a fact, hides a live creature, or advances the resolver
twice. Tiny 1-HP drains necessarily show the old number until the last step;
the low-HP marker and minimum one-pixel bar keep that state identifiable.

## Edge matrix

| Case | Required result | Evidence / remaining follow-up |
| --- | --- | --- |
| 1 HP / KO | Remain visibly alive until zero; settle before faint | Host/device 1-to-0 curve; minimum fill belongs to HUD slice |
| Full HP / zero HP | Full inner track / empty outlined track | Geometry contract; device framebuffer assertions in HUD slice |
| maxHP=0 / HP over max | Clamp endpoints before interpolation, no divide by zero | Host checks every impact tick for both invalid endpoint cases |
| Three digits / long name | No HUD/panel spill | Frozen bounds above; framebuffer tests in HUD slice |
| Immunity / zero damage | No moving HP or reaction; no-effect feedback remains | Host constant HP; existing presentation immunity suite |
| Opponent hit / self-hit | Animate affected side only | Existing side tests; spike host self-hit |
| Healing | Monotonic refill to exact fact endpoint | Device healing scenario |
| Mixed damage/heal, net zero | Separate 80-to-40-to-80 playback | Device; host 80-to-60-to-80 endpoint sequence |
| A acceleration / faint | Exact zero, then normal hide/terminal order | Device accelerated KO plus existing presenter stage coverage |
| Switch with different maxHP | Immediate new identity/maxHP at switch Impact; no cross-species drain | Spike leaves Switch alone; existing host/device switch coverage |
| Sentinel IDs / ordinary entry | Never index absent entries; valid ID 0 renders | Existing presentation fixtures and generated symbolic species IDs |

## Feasibility and reproduction

Fresh default build: **27482 B flash / 1787 B static RAM** (2214 B physical
flash margin; 373 B margin under the 2160 B static ceiling; 773 B physical
SRAM free before stack). The opt-in whole-game build measures **27828 / 1787**:
**+346 B flash, +0 B static RAM**, leaving 1868 B flash. Presenter remains
24 B, session 163 B, ModeState 191 B. The separately proposed six-byte HUD
cache is budgeted for follow-up; its flash/render cost is not measured here.

The dedicated device spike exercises ordinary hit, KO and healing through
real presenter/view/FX drawing: **448 B painted / 379 B effective headroom**
after a 69 B USB ISR allowance. This fixture has a smaller linked test image
than the full presentation suite, so do not claim a 110 B production stack
improvement by comparing it with historical numbers. Preserve the full
presentation/session/arena checks before production enablement.

Per overlay call: at most three resident fact inspections and one 16-bit
multiply/divide by constant 14; no metadata lookup or extra bitmap blit.
Frame-time/cycle instrumentation was not added, so 52-fps worst-case timing
is a required measurement for the production HUD/composition slice, not a
proven claim from a headless serial loop. No manual Ardens check was required.

```sh
make ram BUILD_DIR=build/battle-feel/baseline
make ram BUILD_DIR=build/battle-feel/candidate \
  AVR_SHIPPING_CPP_FLAGS='-mrelax -mcall-prologues -fno-move-loop-invariants -mstrict-X -DCGFX_SHIPPING_NO_USB -DCGFX_BATTLE_HP_SPIKE'
make test BUILD_DIR=build/battle-feel/host
make testvm BUILD_DIR=build/battle-feel/host
make fxtest-spike BUILD_DIR=build/battle-feel/spike \
  FXTEST_SPIKE_INO=tst/fxdatatest/test_battle_hp_spike.ino FXTEST_MS=10000 \
  ARDENS=/path/to/headless-ardens
make fxtest-headless BUILD_DIR=build/battle-feel/optin \
  FXTEST_INOS=tst/fxdatatest/test_battle_hp_spike.ino FXTEST_MS=10000 \
  AVR_FXTEST_CPP_FLAGS='-mrelax -mcall-prologues -fno-move-loop-invariants -mstrict-X -DFX_READ_COUNTER -DCGFX_BATTLE_HP_SPIKE' \
  ARDENS=/path/to/headless-ardens
make verify-generated
make final-gate BUILD_DIR=build/battle-feel/final FXTEST_MS=10000 \
  ARDENS=/path/to/headless-ardens
```

Final validation/resource figures and complete command outcomes live in
`output.md`; diagnostics are under `build/battle-feel/`. No canonical asset
or packed bytes change in this spike.

## Follow-up decomposition

These are contracts, not an instruction to implement all three during research.

1. **Readable stationary HUD (CreatureGathererFX-1ou).** Use option B HUD bounds while initially retaining
   sprite origins; allocate exactly six cache bytes, outline/minimum fill,
   current/max HP, truncation and low marker. Cache names on battle/switch
   transitions and preserve them through submenus. Permanent framebuffer tests
   cover bounds and every numeric/name edge. Verify `make test`, presentation
   `make fxtest-spike` with stack, `make ram`, metadata-read counts, and final gate.
2. **Enable bounded HP playback (CreatureGathererFX-1vk).** Depends on HUD. Convert the opt-in to the
   selected production contract; replace immediate-impact expectations with
   monotonic/endpoint/fact-order assertions. Include opponent/self-hit, 255-HP,
   healing, net-zero ticks, A and faint/switch ordering. Run host/VM, HP spike
   and full presentation spike, whole-image RAM, session/arena effective stack,
   and final gate. Remove experimental API/flag once callers use one path.
3. **Diagonal scene and target reactions (CreatureGathererFX-nif).** Depends on HUD and HP playback;
   coordinate attack groups with jp8.3.18. Move sprites to B, code-draw ground
   strokes, keep HUD stationary, add bounded lean/reaction and column faint/
   switch reveal. Framebuffer checks cover both sides, masks, clipping and
   effect anchors. Measure draw/update time against 19.23 ms/frame and current
   baseline, flash/static RAM, and effective stack; run presentation/stack spike
   and final gate. Asset additions, if subsequently needed, require `make gen`,
   manifest/generated-library checks and pack parity; preserve layout ABI.

Each slice must independently fit 29696 B flash, 2160 B static RAM and about
150 B effective exercised stack reserve. If a slice fails that budget, trim
before continuing to the next slice.
