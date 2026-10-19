# First-beta onboarding brief — the first session, without dialogue

Design record for `CreatureGathererFX-jp8.11.27` (contract revision 2026-10-17,
scope `first-beta`). This is a read-only design input: it authorizes **no**
runtime/AVR change, no asset or map change, no generator change, no new flag and
no popup. It names the small implementation leaves it requires; those leaves
block `CreatureGathererFX-jp8.11.15` (integration proof).

The goal in one line: a first-time player boots a fresh ordinary game and reaches
a **first capture** using only on-screen affordances, without external
instructions and without failing the gather window.

## Why this exists

Shipping ordinary fresh boot has no onboarding. `CreatureGathererFX.ino` shows
the six-line boot card only under the opt-in `CGFX_LURE_WEDGE` demo
(`LureZone::showIntro()`, INO:174, guarded at INO:169), and the boot card is a
text screen the ordinary profile never draws. The compact world VM cannot queue
messages (`README.md` "World script profile"; `interactionDialogActive()` returns
`false` outside `CGFX_FULL_WORLD_VM`, `World.cpp:133-141`). No screen explains
deposit, gather, the x1.5/x0.5 matchup markers, SWAP or WAIT, so a fresh player
must discover the whole loop from tile art and gestures alone.

## Authored contract this script is written against

Pinned by `data/beta-area.toml` and asserted by `tools/tests/beta-area_test.mjs`:

| Pin | Value | Source |
| --- | --- | --- |
| Fresh start | `(11,5)` | `[area] start_x/start_y`; INO:162-163 |
| Plant plot | `(14,2)` GID 551 (`PROP_PLANTABLE`) | `[area] plant_x/plant_y` |
| Deposit tile | `(9,2)` GID 545 (`PROP_SPARE`, "the box") | `[area] box_x/box_y` |
| Rest | `(10,4)` faced-tile trigger | `[area] rest_x/rest_y`; `World.cpp:505-508` |
| Boss | `(21,9)` | `[area] boss_x/boss_y` |
| Encounter tables | 5 tables, levels 2..5, allowed species set | `[encounters]` |
| Party | species 0/3/6 at level 3, full HP | `[party.member]` |

Encounter tiles and any added deposit tile are authored in
`CreatureGathererFX-jp8.11.25`, which owns the packed map; this brief pins only
the route properties that authoring must satisfy, not new coordinates.

## The one approach: affordance-only, with the invisible state made visible

**Selected: affordance-only.** Teach with tile art, the already-shipped native
lure picker, and the already-shipped duel HUD. No dialogue, no scripted message
box, no warp tour.

Reasons, in priority order:

1. **No message channel exists in the shipping profile.** Only native overlays
   run (`LureZone::drawOverlay`, INO:259). Any "explain it" text would be a new
   popup, which the scope forbids.
2. **No new flags.** The v3 snapshot reserves eight flag bytes
   (`SaveFile.hpp:11`) and generated indices are ABI (`jp8.11.13` flag
   amendment). One-shot hint bits and scripted-tour state are exactly the casual
   flag growth this task rules out.
3. **The authored pocket already is the "scripted tour".** Start, plant, box and
   rest all sit in one clearing (`x8..15, y0..7`), so a warp-gated teaching pocket
   would duplicate the area `jp8.11.25` is authoring.
4. **The live duel already speaks fluently.** The compact pick row shows one type
   icon per attack with an **up/down** matchup marker
   (`DuelView.hpp:97-104`, drawn `BattleScene.cpp:122-127`), a **SWAP** cell, a
   **WAIT** cell, numeric HP + bars, team pips and sprite type badges
   (`BattleScene.cpp:60-103`), plus per-round captions
   (`BONUS/WEAK/RPS WIN-LOSE`, `TRADE`, `DEFAULT WIN`, `GATHER`, `GATHERED`, …).
   These teach the duel with zero new code.

Rejected: **one-shot hint** (needs a new flag → ABI risk, forbidden scope);
**scripted tour** (redundant pocket, new warp/content); **full-VM opt-in**
(README: the compact profile cannot queue messages; owner has not reversed it).

## Teaching moments: affordance, state, status

| Moment | Live affordance | State it reads | Status |
| --- | --- | --- | --- |
| Where do I go? | distinct tile art; small authored clearing | packed map (`raw_map_data`) | **sufficient** (art owned by assets/`.25`) |
| Plant the plot | plant GID 551; `A` plants | `plants.plantStages.getStage(0)` (`World.cpp:474-477`) | **gap**: stage renders nothing |
| It grew / it is ripe | (none today) | stage `0..3` (1=planted, 2=growing, 3=mature; `PlantStage.cpp`) | **gap** → leaf A |
| Harvest a lure | native "Gathered" message (`World.cpp:491`) | inventory add `lureId(0,1)` | **sufficient** once growth is visible |
| Deposit the lure | `A` on the box opens the native lure picker (`World.cpp:461-463`; `LureZoneView.cpp`) | lure inventory, `LureZone::deposit` | **sufficient** (existing native overlay, not a popup) |
| Zone is live | per-encounter tables switch on `LureZone::tableOffset()` | `LureZone::active()/charge()` | **sufficient for first capture**; charge cue deferred (OWNER-PENDING) |
| Walk into a creature | encounter tile rolls wild 10%/step (`StepEvent.cpp:29`) | `PROP_ENCOUNTER` RAM window | **sufficient** (placement owned by `.25`) |
| Read the battle | pick-row icons + up/down markers + SWAP/WAIT + HP/pips/badges + captions | `BattleState` (resident) | **sufficient** |
| Capture the creature | `GATHER` action row on the `B` page (`BattleMenu.cpp:169-179`) | `state.gatherable` | **gap**: `B` page has no cue → leaf B |
| Did gather work? | per-round `GATHER` caption | `state.gather.progress/need` | **partial**: each round shows, progress does not → leaf C |
| Recover after a loss | `A` facing `(10,4)` heals, no cue | `player.healParty()` | **sufficient for first capture**; rest cue deferred |

Nothing else is needed for the first-capture goal. Deposit-charge and rest cues
are real nitpicks but are not load-bearing here; they stay as OWNER-PENDING
polish rather than new leaves.

## First-session script (intended order)

Random encounter tiles can interrupt, but the intended teaching order is:

1. **Boot.** Fresh ordinary game (no restore). No card is shown; the player is at
   `(11,5)` facing down, inside a small clearing.
2. **Walk north and plant.** Reach the plant plot at `(14,2)`. Face it, press
   `A`. With the growth cue (leaf A) the tile visibly changes to "planted".
3. **Roam while it grows.** Encounter tiles on the route roll a wild battle
   (10% per step). The **first encounter is the combat lesson**: the pick row,
   the up/down matchup markers, SWAP and the `B` action page (escape) are all on
   screen. A wild encounter before a lure is deposited is an ordinary battle
   (attack/switch/escape; no capture) and carries no gather-window pressure.
4. **Harvest the lure.** When the plot visibly ripens (≈256 overworld steps from
   planting: `incrementAll` once per 128 steps, `PlantGamestate.cpp:3`), face it
   and press `A`: the native "Gathered" message shows and one tier-1 lure enters
   the bag.
5. **Deposit at the box.** Walk to `(9,2)`, face the deposit tile, press `A`. The
   native lure picker opens ("Set Lure", `A:Set B:Back`); press `A` to set it.
   The zone becomes active for the current pocket chunk.
6. **Pick a fight on purpose.** Step onto an encounter tile inside the deposited
   chunk. The wild battle now uses the lure's table and is **gatherable**: with
   the gather cue (leaf B) the capture action is visible in the HUD.
7. **Capture.** Choose gather each round. Tier-1 rate is 2/round against need 9-10
   (`data/beta-area.toml` `[renewable]`), so 5 gather rounds complete inside the
   6-round window; leaf C shows progress. The fifth round resolves with the
   `gathered` caption and the creature is committed to the 64-slot collection
   (`BattleFlow.cpp:50-66,148`).
8. **After (not part of this acceptance).** `(10,4)` rest restores HP; the fixed
   boss `(21,9)` and ending are `CreatureGathererFX-jp8.11.13`.

## Implementation leaves this brief requires

Three small leaves. Each is state-driven render/menu work with no new flag, no
popup, no VM change. Sizes are **estimates** from the dated control
(23,252 B program flash / 1,665 B static SRAM / 620 B stack headroom at
`d8cfd1c`, `build/reports/rhs7/final-gate-jp8-11-5/`); whole-image A/B decides.

- **Leaf A — visible plant plot growth/ripeness.** Chart the plot at `(14,2)` to
  `PlantStage` 0..3 so planted/growing/mature are distinguishable. Preferred:
  append authored frames for the plot GID and select by stage; reduced: a small
  stage marker above the plot from existing art. State:
  `plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX)`. Estimate ≈80-200 B flash,
  0 new static SRAM.
- **Leaf B — surface the GATHER action for gatherable encounters.** Make the
  capture action visible instead of hidden behind `B`. Preferred: a `GATHER` cell
  in the pick row (after SWAP) when `state.gatherable && !state.trainer`;
  reduced: a compact gather icon in the centre strip. No new state, no flags.
  Estimate ≈120-300 B flash, 0 new static SRAM.
- **Leaf C — visible gather progress.** When gatherable, show
  `state.gather.progress`/`need` (reuse the existing thin track style). State:
  `BattleState::gather`. Estimate ≈40-120 B flash, 0 new static SRAM.

Each leaf must run `make test`, the touched device suite with `test_stack`, and a
same-source `make ram` A/B on the shared `build/rhs7-lane` lane, keeping
`test_stack` ≥219 B with the ~150 B reserve. Leaf B/C share the pick-row
surface, so they serialize on one build lane and one `BattleMenu`/`BattleScene`
edit order (and after any concurrent `jp8.5.19` picker edit).

## Observable acceptance (for `jp8.11.15`)

- An ordinary fresh boot (no demo flag, no full-VM override, empty bag) can, from
  the authored start, plant at `(14,2)`, harvest a lure once the plot visibly
  ripens, deposit at the pinned deposit tile via the native picker, trigger a
  gatherable encounter in the same chunk, and reach `Outcome::Gathered` within the
  6-round window on the first attempt — with the captured instance present in the
  collection — using only on-screen affordances and no external instructions.
- No new flag, no popup/VM message/queued dialogue, no per-frame FX metadata read
  (encounter zone/table reads stay transition-only), explicit-save only.

## OWNER-PENDING (not approved here)

Carried from `docs/first-beta-pacing-brief.md` and untouched by this brief:
session length **15-25 min**; wins/deposits to the boss; capture attempts per
lure; KO/loss cost; accepted power-loss window (explicit save only). No balance
target, no pacing number and no fresh requirement is approved by this task. The
13..31-wins bound and the 12-vs-10 gather margin remain the only contract-implied
expectations.

## Out of scope

No runtime/AVR/asset/map/generator change or device test in this bead; no
deposit-charge or rest cue (deferred, OWNER-PENDING); no shops, currency, arena
or full-VM work.
