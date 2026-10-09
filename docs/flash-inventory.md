# Feature and flash inventory

Measured 2026-10-08 for `CreatureGathererFX-ax8`. Committed reference: `5835223`; the recovered baseline retains independent working-tree changes.
The current working tree also contains the uncommitted M1 checkpoint draft,
inventory screen, and owner edits. This is an inventory, not a proposal to remove gameplay.

## Landed reductions after the audit (2026-10-09)

The tables below retain the original audit controls. These subsequent isolated
reductions preserve tactical AI and battle visuals; whole-image deltas differ
between the accepted baseline and the uncommitted M1 draft.

| Change | Accepted baseline flash / static saved | M1 draft flash / static saved |
|---|---:|---:|
| Remove unused Animator | 228 / 28 B | 152 / 28 B |
| Remove redundant move snapshot clearing | 42 / 0 B | 44 / 0 B |
| Compact authored world-warp execution and omit unavailable world dialogs | 812 / 0 B | 868 / 0 B |

Accepted firmware now measures **26,596 B flash / 1,759 B static SRAM**.
The original journal-backed M1 draft measures **30,046 / 1,842 B**, still
350 B above the hard flash limit. Full world commands and dialogs remain
available through `CGFX_FULL_WORLD_VM`; default builds reject incompatible
authored content. All three existing warps remain. The damage-only AI experiment
was rejected by the owner and was not adopted.

The separate working-sector storage prototype with the compact world profile
measured **29,152 / 1,807 B**, only 32 B below the planning target. It is not
adopted: batched updates, recovery and painted-stack acceptance remain pending.

## Physical budgets and current builds

| Image | Firmware flash | Static SRAM | Flash headroom against 29,696 B |
|---|---:|---:|---:|
| Recovered baseline | 27,678 B | 1,787 B | 2,018 B |
| Current 64-slot checkpoint draft, after trim1 | 31,110 B | 1,870 B | **−1,414 B** |
| Net draft increase | **+3,432 B** | +83 B | |

Firmware flash is `.text + .data`; SRAM is `.data + .bss`.
The draft is blocked and has not passed device verification or the final gate.
Its fresh isolated control reproduces the existing shipping ELF exactly.
Static SRAM is below 2,160 B; that does not establish stack safety.

Creature instance records, species/move tables, maps, scripts, text rasters and
sprites live on external FX flash. They do not occupy application flash merely
because they exist in the cart. Their reader, rendering and game logic do.
Collection instance capacity and the number of authored species are separate limits.
The draft has 64 instance slots including three party slots; ordinary species
bounds remain 32, with the expansion roster available in opt-in demos.

## Measured current feature costs

These are **whole-image removal deltas**, measured with the same AVR compiler
and shipping flags. Each row removes the specified entry points from an isolated
copy and recompiles with LTO. These diagnostic images are intentionally incomplete
games. Savings overlap and **must not be added**. They are not promises that a
small redesign of the feature will recover that many bytes.

| Removed or simplified boundary | Baseline bytes released | Draft bytes released | Draft image after change |
|---|---:|---:|---:|
| Battle runtime, battle menu display, scene, encounter startup and initial battle transitions | 18,372 | **18,314** | 12,796 |
| All sketch-facing save paths: boot load, seed, advance/status and journal init | 2,262 | **5,718** | 25,392 |
| World frame update plus map/player drawing | 4,514 | **4,166** | 26,944 |
| Script execution at the world interaction boundary | 1,368 | **1,242** | 29,868 |
| Scene rendering: battle scene, world map and player; dialogs/feedback retained | 2,194 | **2,038** | 29,072 |
| Tactical AI replaced with damage-only AI | 998 | **952** | 30,158 |
| Battle presenter replaced with immediate completion; scene/menu retained | 3,906 | **3,848** | 27,262 |
| Battle menu preparation, input and drawing disabled; battle flow retained | 3,574 | **3,080** | 28,030 |
| Plant step-growth hook disabled; save serialization retained | 86 | **86** | 31,024 |
| Generic per-frame Animator call disabled; global object retained | 220 | **144** | 30,966 |

The save row includes the existing snapshot save machinery as well as M1, so the
checkpoint draft does not cost 5,718 B on its own. Its measured net change, including adapters and serialization/compiler effects, is
3,432 B. Removing save call paths from the two trees leaves images differing by
24 B because the draft also changed plant serialization and compiler decisions.
World removal also disconnects encounters and script callers, while battle
removal disconnects battle menus, presentation and some graphics. These are broad
feature boundaries, not exclusive accounting categories.

Battle resolution, effects, AI and presentation account for the largest retained
firmware family. Presentation is substantial; changing sprite artwork alone is
cheap in firmware because the raster data is external.

## Complete runtime feature inventory

Cost labels below refer to the measured boundaries above. A shared label avoids
inventing exclusive costs for subfeatures inlined or shared by the optimizer.
“Unlinked” means no gameplay caller and no identifiable retained implementation
were found. Missing named symbols alone cannot rule out inlining; only removal
measurements establish a net cost. Wiring these APIs into gameplay can add code.

| Feature | Current behavior and reachability | Firmware cost evidence |
|---|---|---|
| Hardware/startup | FX/SPI/OLED init; button input; 52 fps; one update/render/display; DOWN bootloader recovery | Shared platform/runtime; USB already omitted |
| 1bpp graphics | Masking, clipping, sprites, text, glyphs and number rendering | Scene boundary 2,038 B; feedback/dialog blitters also shared |
| World/map | One canonical 256×256 map, chunk addressing, streamed viewport and scrolling | World family 4,166 B |
| Movement/collision | Four directions, 16 ticks per tile, bounds, cached occupied/walkability properties | Inside world family |
| Player walking | Directional idle and two walk poses; no extra animation state | Historical isolated addition +36 B |
| Interaction | A-button reads the current chunk's 128 B script and executes it | World/VM shared |
| Script VM | Messages, tile-sensitive messages, teleport, flag branches/set/clear; ReadFlag only consumes operand | VM boundary 1,242 B |
| Dialogs | Six-entry bounded FIFO, prepared two-line text, A dismissal and movement pause | VM/world/graphics shared; not independently measured |
| Creature/party | Three expanded members, persistent HP, species/types/stats/four moves | Battle/save shared |
| Plant growth | Step ticker; stage update each 128 steps | Step-growth boundary **86 B**; save storage shared; planting/harvesting and area UI absent |
| Encounters | Cached zone tables, ten-slot selection, average-party-level adjustment | Battle/world shared; **current map has no encounter tiles** |
| Wild battles | Regular world-to-battle path wired; opt-in demo forces first-step encounter | Battle family 18,314 B; ordinary map cannot currently trigger it |
| Trainer battles | Three-on-three resolution available through trainer and arena demos | Battle shared; no ordinary world trainer trigger |
| Turn/damage/type rules | Priority/speed, integer physical/special damage, STAB, dual-type chart | Battle shared; stored accuracy has no resolution caller |
| Move effects/status | Stat stages, status gates/ticks, healing, faint, defeat, replacement, switching, escape | Battle shared |
| Tactical AI | Damage choice, lethal preference, setup/regeneration, threatened switching and lock | **952 B over damage-only AI** |
| Move uses/PP | Packed spent counters, move refusal, PP display, exhaustion pass; resets per battle | Battle/menu shared; historical PP first slice +408 B |
| Battle menus | Options, four moves, bench selection, forced replacements, cancellation and cached labels | Battle menu boundary **3,080 B**, overlaps scene and battle family |
| Battle presentation | Announcements, feedback, HP treatment, impacts/shake/blink, switches, faints, terminal dwell, A advancement | **3,848 B presenter boundary**; scene retained |
| Battle scene/HUD | Native 48px sprites, HP bars and player exact HP, compact FOE/YOU HUD | Scene/battle shared; recent HUD addition +66 B |
| Gathering | Progress, refusal/flee/result resolution and feedback exist | Battle shared; current demo passes gatherable=false; no acquired-record insertion |
| Item inventory | 24 lure IDs, eight consumable IDs, bounded counts, names/definitions, key APIs | Helpers largely unlinked; no runtime bag acquisition/ownership integration |
| Inventory screen | Four-row cached scroll/select/render implementation in working tree | No runtime open/draw caller or identifiable retained implementation; no isolated zero-cost claim |
| Battle consumables | Heal resolver and SelectItem exist; no installed runtime inventory or selection entry | Battle-shared partial code; Cure/Charge effects unfinished |
| Lures | Type/tier/charge/rate helpers exist | Live prototype was removed with **0 B flash change**; activation, decay and influence absent |
| Save loading | Boot restores snapshot/party state | Save family 5,718 B including draft |
| Explicit saving | SAVING update/render branches wired; multi-frame save code reachable from them | Save shared; **no gameplay save action calls begin** |
| M1 collection | Draft 64 stable 8 B records, duplicate species, journal/index/replay, coherent checkpoints and boot recovery | **+3,432 B net over baseline**; uncommitted and over budget |
| Collection gameplay | Collection UI, acquired-creature insertion, XP award, evolution and move learning | Not integrated; no current feature cost measurement |
| Arena | Three player teams, five opponents, previews, selection, random match and return | Opt-in only; historical net **+1,004 B** on its contemporaneous baseline |
| Expanded species roster | Extra authored species/sprites available to expansion demo | External data; historical demo bootstrap **+188 B** firmware |
| Generic Animator | Per-frame call remains; no production push caller found | Current call removal **−144 B** (−220 B baseline); historical object-removal prototype −150 B not landed |
| Legacy helpers | Event-table loader, Action wrapper and external Font4x6 lack gameplay callers | No standalone current cost established; much is linker-discarded |
| Development checks/demos | Trainer presets, utility/switch/expansion demos, HP/presentation spikes, FX read counter | Opt-in; default flags exclude them; tests and desktop simulator are not game firmware |
| Additional game systems | NPC actor renderer, shops, quests, crafting, audio, title/pause/world-options UI | No integrated runtime implementation found; WORLD_OPTIONS returns no intent |

## External FX flash inventory

The generated cart contains **742,765 B data**, **32,768 B reserved save space**
and 147 B data-page padding: **775,680 B total**. Against 16 MiB this is about
4.62%. External capacity is not the present limit.

Largest field extents below are the distance to the next generated offset; they
can include field padding. `build/flash-audit/fx-fields.csv` inventories all fields
and its extents reconcile exactly to 742,765 B.

| External field | Bytes |
|---|---:|
| Chunk scripts | 262,144 |
| map_data | 131,072 |
| raw_map_data | 131,072 |
| Native battleSprites48 | 73,732 |
| NewecreatureSprites | 32,768 |
| tiles | 24,576 |
| letters | 16,384 |
| maskedFont | 12,288 |
| creatureSprites | 8,192 |
| ecreatureSprites | 8,192 |
| Four beam/wave fields | 8,192 combined |
| Other data and field padding | 34,153 |
| **Total** | **742,765** |

These older asset fields may be unused by runtime code while still remaining in
the append-only packed layout. Removing their bytes would reclaim external
storage, not firmware code automatically. Do not reorder the FX ABI.

The reserved save region is eight 4 KiB sectors. The M1 draft uses store_a and
store_b for checkpoints, save_log for journal updates, leaving save_main and
four reserved sectors unused by v3. Each checkpoint occupies 624 B inside its
sector: 16 B header + 96 B snapshot + 64×8 B records. Increasing 32 to 64 slots
adds 256 B **external records per checkpoint**, not 256 B firmware.

## Linked-symbol accounting and its limits

| ELF accounting | Baseline | Draft |
|---|---:|---:|
| .text | 27,588 | 31,014 |
| Unique sized text/data-in-text symbol ranges | 27,388 | 30,814 |
| Unsized text ranges, vectors and padding | 200 | 200 |
| .data initialization image | 90 | 96 |
| **Total firmware flash** | **27,678** | **31,110** |

Aliases at the same address are counted once. Debug sections in the ELF occupy
host disk space and are not flashed. The draft's `main` alone is 8,896 B and
contains inlined pieces from multiple features. AVR-GCC LTO debug attribution is
unreliable in this build: unrelated functions resolve to DialogQueue.cpp:15.
Those source-group numbers were discarded. Exact sized-symbol CSVs are retained;
whole-image ablations provide the usable feature measurements.

Largest draft named ranges: `main` 8,896 B; BattleFlow::beginResult 1,746 B;
BattlePresenter::prepare 1,262 B; applySwitch 1,246 B; WorldEngine::moveChar
1,222 B; resolveAttack 1,040 B; storeBoot 852 B; ScriptVm::run 672 B;
MenuV2::openMenu 582 B; checkpoint validate 542 B; MenuV2::update 530 B;
Blit::draw 468 B; chooseVoluntarySwitch 424 B; BattleSession::view 390 B;
journalAppend 340 B. These sizes include inlined callees and are not additive
removal predictions.

## Useful historical measurements

All are historical whole-image comparisons from `output.md`, not current
exclusive component sizes.

| Change | Measured flash delta | Ledger line |
|---|---:|---:|
| Shipping USB/CDC disabled, same source | −2,612 B; −138 B SRAM | 1221 |
| Remove menu optimize("-O0") override | −1,452 B | 1251 |
| Shared prologues | −478 B | 1229 |
| Linker relaxation | −324 B | 709 |
| Renderer consolidation | −2,160 B, includes compiler flags −372 B | 2514 |
| Remove retained legacy battle/Arena paths | −2,476 B | 2355 |
| Remove obsolete battle-dialog dispatch | −1,186 B | 3749 |
| Utility battle wave: PP/stages/status/tactics | +2,078 B combined | 3425 |
| Tactical switching first spike | +838 B; later trims shared | 3603 |
| Shared type/status/modifier helpers | −382 B across two measured slices | 5313, 5387 |
| Player walking integration | +36 B | 6678 |
| Promote 48px battle sprites/repair labels | +182 B | 6711 |
| White-on-black feedback | −132 B | 6732 |
| HP ownership clarification | +44 B | 6743 |
| Compact center HUD | +66 B | 6753 |
| HP drain/refill research option | +346 B, opt-in | 5548 |

USB removal and compiler trims are already in the current baseline. Counting
those savings again would invent headroom. Earlier battle playback integration
added 11,548 B while retaining old battle paths; that is not the cost of today's
battle subsystem. Device-test images have USB enabled and are not shipping
comparisons.

## Reproduction and validation

Compiler: AVR-GCC 7.3.0-atmel3.6.1-arduino7; FQBN
`arduboy-homemade:avr:arduboy-fx`. Shipping C/C++/link flags:
`-mrelax -mcall-prologues -fno-move-loop-invariants -mstrict-X`;
C++ additionally `-DCGFX_SHIPPING_NO_USB`. USB-disabled diagnostic ELFs are
measured even when Arduino reports the application-size failure.

Commands run:

```sh
node build/flash-audit/measure.mjs
AUDIT_BASELINE=1 node build/flash-audit/measure.mjs
node build/flash-audit/symbols.mjs
node build/flash-audit/fx-fields.mjs
git diff --check
```

Raw sizes and removal deltas are also preserved in
[flash-inventory-measurements.csv](flash-inventory-measurements.csv).

Scripts, isolated source copies, eleven ELFs per tree, compile logs, measurements
and symbol/field CSVs are in `build/flash-audit/`. The baseline series replaces
only the storage files, plant backing-byte headers and sketch with their HEAD
versions; it retains independent working-tree changes. Its control exactly
matches 27,678/1,787. The draft control exactly matches 31,110/1,870.
No production source was edited for the audit; no feature was removed from the
working game. These were compile/size experiments, not functional acceptance
runs. An integrated gate would still fail the draft's physical flash budget.

## Subsequent cleanup

Animator removal (`575a9af`) measured27,450flash/1,759static on the shipping
baseline and30,958/1,842 on the still-uncommitted checkpoint draft. The table
above records the pre-cleanup audit snapshot. The build scripts copy the current
working tree when rerun; retained original source snapshots and ELFs under
build/flash-audit are the evidence for the original controls. Recompile those
snapshots directly with the documented FQBN and flags to reproduce the audit
after subsequent source changes.
