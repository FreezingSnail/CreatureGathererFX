# First-beta pacing and lure economy — brief

Pinning input, 2026-10-17 (CreatureGathererFX-jp8.11.26). Scope: `first-beta`.
This document records control numbers and candidate levers. It authorizes **no
production number changes**: `data/beta-area.toml` is the contract and its
numbers change only with owner acceptance. Anything labelled **OWNER-PENDING**
is a proposal, not an accepted target.

## What the contract already implies

Every number below is derived from committed sources; the exact formula and the
file that owns it are named so drift is checkable. `tools/tests/beta-area_test.mjs`
now asserts the load-bearing ones (curve shape, gain constant, win bounds, boss
level) and fails loudly on a rebalance.

### Experience curve and the level-8 evolution edge

- `src/creature/LevelCurve.hpp:16` — `LEVEL_EXP[n] = n^3` (cumulative; entries 0
  and 1 both map to level 1). `LEVEL_EXP[3] = 27`, `LEVEL_EXP[8] = 512`.
- Party starts at **level 3** (`data/beta-area.toml` `[[party.member]] level = 3`
  ×3). The first-stage evolution edge is **level 8**
  (`data/beta-area.toml` `[[evolution.edge]] level = 8`, three edges
  `0→1, 3→4, 6→7`).
- Exp to the edge: `512 − 27 = **485**`.

### Ordinary-win experience

- `src/engine/battle/BattleFlow.cpp:85,102` — an ordinary (non-trainer,
  non-gatherable) wild Win grants `8 × opponentLevel` to every living occupied
  party instance, once, at the terminal edge.
- Authored ordinary foes are **level 2..5**
  (`data/beta-area.toml` `[encounters] level_min = 2`, `level_max = 5`), so a win
  yields **16..40 exp** (`8×2 .. 8×5`).

### Wins to the level-8 edge (exact)

`ceil(485 / gain)` over the authored gain range:

| Path | Gain/win | Wins |
| --- | --- | --- |
| Fastest (all level-5 foes) | 40 | `ceil(485/40) = **13**` |
| Slowest (all level-2 foes) | 16 | `ceil(485/16) = **31**` |

**Corrected bound: 13..31 wins.** The jp8.11.26 bead description's "roughly
12-30 wins" is a rounded approximation; the exact bounds are 13 (fastest, level-5
foes) and 31 (slowest, level-2 foes). A mixed 16..40 run lands inside that band.

### Lure economy (deposit → gather)

- Plant: `data/beta-area.toml` `[plant_source]` yields **one type-0 / tier-1
  lure** (`item_id = 1`, `count = 1`). Cadence: `src/plants/PlantGamestate.cpp:3`
  advances plant stages once per **128 overworld steps** (`ticker == 128` in
  `PlantGameState::tick`, called every step from `src/engine/world/StepEvent.cpp:110`).
- Spend: `src/engine/world/LureZone.cpp` `pickerDeposit()` **consumes one lure**
  (`inventoryTake(..., Lure, itemId, 1)`) and deposits it; `deposit()` sets the
  zone charge to `lureChargeOf(tier)`.
- Charge per deposit: `src/item/ItemIds.hpp:24` `lureChargeOf(tier) = 16 << tier`
  → **tier-1 = 32 steps, tier-2 = 64 steps** of active zone.
- Gather rate vs need: `[renewable] rate = 2` (`1 << tier`, `src/item/ItemIds.hpp:25`
  `lureRateOf`) against `need = clamp(8 + level/2, 8, 24)`
  (`src/engine/battle/BattleSetup.cpp:114` `wildNeed`). Authored foes 2..5 give
  **need 9..10** (`8+2/2=9 .. 8+5/2=10`). The flee window is **6 rounds**
  (`src/engine/battle/BattleSetup.cpp:178` `fleeTurns = 6`).
- Gather rounds to acquire: `ceil(need/rate) = ceil(9/2..10/2) = **5 rounds**`
  (`2×5 = 10 ≥ 10`). Margin: `rate×turns = 12 ≥ 10` — the **12-vs-10 gather
  margin** is the authored "first battle with no KO risk" requirement, kept
  intentional, not slack. Acquisition also blocks if either active fainted that
  round (`src/engine/battle/BattleSession.cpp:624`).
- Ordinary wild KO return: one **tier-2** lure, count 1, per species
  (`src/engine/battle/BattleFlow.cpp:70` `awardWildKoMaterial`, item =
  `lureType×3 + 2`; asserted in `beta-area_test.mjs` "generated reward mapping
  agrees with the authored contract and header").
- Encounter roll: an encounter-prop step rolls wild **10%**
  (`src/engine/world/StepEvent.cpp:29` `ENCOUNTER_STEP_PERCENT = 10`). Production
  map data currently authors no encounter tiles; a deposited supported lure sets
  the gatherable zone table (`LureZone::tableOffset()`).
- Healing: only the free **rest tile** (`data/beta-area.toml` `rest_x/rest_y`;
  `src/engine/world/World.cpp:478` `player.healParty()` on a fresh A). No other
  heal path exists in the first area; the authored `[[reward.heal]]` drops are
  items, not free restores.

### Duel damage constants (frozen)

`docs/duel-system-brief.md:36` — **win 14 / chip 4 / trade 6** (each scaled by
type matchup). Frozen by `docs/duel-growth-rearch-brief.md`; not a lever in this
bead.

## Targets

Targets are split into those the contract already implies and those it does not.

| Target | Value | Status |
| --- | --- | --- |
| Wins from level 3 to the level-8 edge | 13..31 | **Implied** by contract (encounter levels + curve + gain) |
| Gathers to acquire one gatherable foe | 5 rounds of 6 | **Implied** (rate 2, need 9..10, flee 6) |
| First-battle KO risk | none (12 ≥ 10 margin) | **Implied** design requirement |
| Boss reachable level | 8 | **Implied** (`[boss] level = 8`) |
| Session length (first beta) | — | **OWNER-PENDING** — propose 15–25 min |
| Wins / lures to reach the boss | — | **OWNER-PENDING** — propose 2–4 gatherable deposits |
| Lure income vs spend per session | — | **OWNER-PENDING** — see lever table |
| Capture attempts per deposited lure | ~3 (32-step tier-1 charge × 10%/step) | Derived control; **target OWNER-PENDING** |
| KO / loss cost | — | **OWNER-PENDING** — rest is free; no authored loss penalty yet |
| Accepted power-loss window | explicit save only | **OWNER-PENDING confirmation** (save model is explicit-only; see `docs/m1-storage.md:112`) |

No target above except the four "Implied" rows is accepted; the OWNER-PENDING
rows are proposals for the owner to accept or revise before contract numbers move.

## Candidate levers (gap → owner → file)

Each row is a gap between an owner-pending target and the current control, with
the single file that would change. Nothing here is applied in this bead.

| Lever | Gap addressed | Ownership | File to change |
| --- | --- | --- | --- |
| Ordinary-wild exp gain constant (`8`) | wins-to-evolve / session length | runtime | `src/engine/battle/BattleFlow.cpp:102` |
| Exp curve `n^3` | wins-to-evolve across the whole game | runtime | `src/creature/LevelCurve.hpp:16` |
| Encounter level range 2..5 | win gain band (16..40) | contract | `data/beta-area.toml` `[encounters]` |
| Plant cadence 128 steps | lure income rate | runtime | `src/plants/PlantGamestate.cpp:3` |
| Plant yield count (`[plant_source].count`) | lure income per harvest | contract | `data/beta-area.toml` `[plant_source]` |
| Tier-2 return per ordinary KO | lure income from fighting | generator | reward mapping (`data/beta-area.toml` allowed_species → `fxdata/generated/rewards.txt`) |
| Deposit charge `16 << tier` | lures spent per session / steps per lure | runtime | `src/item/ItemIds.hpp:24` |
| Gather rate `1 << tier` | capture attempts per lure | contract+runtime | `data/beta-area.toml` `[renewable].rate` + `src/item/ItemIds.hpp:25` |
| Gather need `clamp(8+level/2,8,24)` | capture rounds | runtime | `src/engine/battle/BattleSetup.cpp:114` |
| Flee window `6` / need_max `10` | KO-risk margin | runtime+contract | `src/engine/battle/BattleSetup.cpp:178`, `data/beta-area.toml` `[renewable].turns` |
| Encounter roll `10%` | encounter frequency per step | runtime | `src/engine/world/StepEvent.cpp:29` |
| Encounter tile placement | encounter frequency (currently none) | map content | `maps/world_map.json` + map authoring |
| Rest tile heal / KO cost | loss cost | runtime+map | `src/engine/world/World.cpp:478`, rest tile in `maps/world_map.json` |
| Save cadence (no autosave) | power-loss window | storage | `docs/m1-storage.md`, save controller |

## Verification

- `tools/tests/beta-area_test.mjs` pins the curve shape, the `8×` gain constant
  (read from `BattleFlow.cpp`), the 485 threshold delta, the 13..31 win bounds
  and the boss/edge level 8 (read from the contract). Host-only.
- The renewable rate/need/window test and the reward-mapping test already existed
  and are unchanged.
