# CreatureGathererFX — session reports (active wave)

Older session reports (280 sections through `ogh.17`, 2026-10-15/16) are archived at
`docs/reports-archive/2026-10-16-earlier-sessions.md`. Keep this file to the active wave; rotate
completed sections out per `AGENTS.md` -> "Session efficiency rules".

## ogh.18 — mirror-tier player policy

Measurement/tooling only: **no shipped gameplay constant or data file changed** (off the
AVR/generation lane; no `make gen`/AVR/gates; no git commit/push). Bead left `in_progress`.
Adds the player policy ogh.17 recommended so the paired 3v3 baseline reflects the shipped
up/down creature-type marker instead of understating player skill.

### Policy rule (new `tier` policy)

`tier` mirrors the AI's no-read branch (`aiDuelPick`): score every owned option by
`duelCreatureBonus(state, 0, option.type)` (creature-type x1.5 beats / x0.5 loses against the
foe's active types) and pick uniformly among the best tier via the same player RNG. It reads
only the foe's already-active creature types, never the hidden current pick, and folds through
`duelValue` like the other policies. Deterministic per seed.

### Changed

- `tools/battle-duel-spike.mjs` — `POLICIES` gains `tier` (default 7: random/repeat/counter/
  rotation/swap/tier/anti-repeat); `playerPick` `tier` branch; header/policy comments.
- `tools/tests/battle-duel-spike_test.mjs` — +1 test (`tier` picks a best-tier option for every
  tie-break, replay byte-identical) and JSON coverage assertions (`tier` in artifact
  `config.policies` and in every pair's `cells[]`).
- `docs/battle-simulator.md` — policy list + JSON `config.policies` example updated.

### Baseline before/after (matrix5 team3 bonus cap200 reader, 25 pairs x policies x 5 rates x 1000)

Before = `build/reports/ogh.6/pair-sweep-1000.json` (6 policies, 750k duels).
After = `build/reports/ogh.18/pair-sweep-1000.json` (7 policies incl. tier, 875k duels).

| metric | before (6 pol) | after (7 pol) |
| --- | --- | --- |
| W / L / D / T | 29.5% / 67.8% / 2.7% / 0% | 32.6% / 64.1% / 3.2% / 0% |
| rounds total / avg | 12,756,050 / 17.008 | 14,965,941 / 17.104 |
| cell medianRounds min/med/max | 11 / 15 / 33 | 11 / 15 / 43 |
| cell p95Rounds min/med/max | 11 / 18 / 36 | 11 / 18 / 43 |
| switchRate (switches/duel) | 0.0008 (0.0133) | 0.0007 (0.0114) |
| type skew (n) | water 40.6 / spirit 25.1 / earth 24.0 / fire 7.1 / plant 3.2 (12.7M) | water 42.1 / earth 24.6 / spirit 23.8 / fire 6.4 / plant 3.2 (15.0M) |
| decided pairs (0% or 100%) | 12/25 (8 zero-win, 4 100%-win) | 11/25 (7 zero-win, 4 100%-win) |

The 6-policy mean win rate (the dispatch's "before") is 29.5% overall, **23.6% @rate0**,
**30.0% @rate50**. The `tier` policy alone (the dispatch's "after") is **36.0% @rate0** and
**54.8% @rate50**, matching ogh.17's `mirror tier` probe exactly. The 7-policy mean is 25.4%
@rate0 / 33.6% @rate50. Adding `tier` dissolves one zero-win pair — `retyped vs balanced`
(0.0% → nonzero) — leaving **7/25 zero-win** and 11/25 fully decided, the remaining hard locks
being the mono/single-kit compositions ogh.17 cause 2 (`balanced v anchors`, `support v anchors`,
`retyped v anchors`, `trap v balanced/anchors/support/retyped`).

Per-policy Win% after: random 25.6, repeat 40.1, counter 36.8, rotation 24.5, swap 25.7,
**tier 51.3**, anti-repeat 24.4. The 4 flagged >80%-in-every-cell sweeps are unchanged
(`balancedvtrap`, `anchorsvsupport`, `anchorsvretyped`, `anchorsvtrap`).

### Verify (commands + tails)

```sh
node --test tools/tests/battle-duel-spike_test.mjs tools/tests/duel-kernel-parity_test.mjs
# tests 26 pass 26 fail 0   (+1 vs ogh.17's 25)
make sim-test BUILD_DIR=build CPPFLAGS='-I. -DCGFX_DUEL_BATTLE'
# tests 6 pass 6 fail 0

node tools/battle-duel-spike.mjs --json --pair-sweep --battles 1000 --rounds 200 > build/reports/ogh.18/pair-sweep-1000.json   # exit 0, 23.6 s, 823,759 B
node tools/battle-duel-spike.mjs --json --pair-sweep --battles 1000 --rounds 200 > build/reports/ogh.18/pair-sweep-1000-replay.json
cmp ...pair-sweep-1000.json ...pair-sweep-1000-replay.json   # BYTE-IDENTICAL
shasum -a 256 build/reports/ogh.18/pair-sweep-1000.json      # 3a92b0a2ff4157ded6ca6d0f3904626b760e5ac072a66661050c17f08055f136

# ogh.17 mirror probe now reproducible through the tool at 500 duels/cell:
node tools/battle-duel-spike.mjs --json --pair-sweep --policy tier --read 0  --battles 500   # W 36.0 / L 36.0 / D 28.0
node tools/battle-duel-spike.mjs --json --pair-sweep --policy tier --read 50 --battles 500   # W 54.8 / L 44.3 / D 0.9
```

JSON coverage: the artifact carries `tier` in `config.policies` and a `tier` cell under every
pair's `cells[]`; the replay artifact is byte-identical, so the policy is determinism-safe.

### Storage

`df -h .` 64 GiB free before/after (>=10 GiB reserve held). `du -sh build` 1.9G before and after.
New artifacts confined to gitignored `build/reports/ogh.18/` (1.7 MB: after/replay JSON, tier
500-duel JSONs, before/after analysis, `analyze.mjs`). No build tree created; `sim-test` reused
`build/duel-sim`. Untouched by this bead: pre-existing `CreatureGathererFX.ino` and
`.beads/interactions.jsonl` edits by the generation-lane worker.

### Wall time

Worker ~25 min wall: read bead/output/docs + tool/mock ~8 min; implement policy + tests + docs
~6 min; focused node test + sim-test ~1.5 min; baseline x2 + tier probes ~1.5 min; analysis +
report remainder. No gate/orchestrator run (dispatch leaves the bead in_progress).

## jp8.11.24 — beta party start

### Change

- `CreatureGathererFX.ino` — the ordinary `!restored` branch no longer calls `player.basic()`
  (species-only default = species 0/1/2 at the maximum level). It now applies the generated
  authored `BattlePresets::beta` preset (species 0/3/6, level 3, full HP, authored attack masks
  2/4/2), installs the one Player bag as the battle inventory, then `SaveController::seedParty()`
  seeds the stable v3 instances. The `BattlePresets.hpp` include moved out of the
  trainer/wild-demo guard because the demo bootstraps and the ordinary start now share it. A valid
  restore returns before the branch and keeps its own party + `playerLocation`.
- `tst/save_test.hpp` — new `FreshBetaStartTest` (49 assertions): fresh-start identities, stable
  instances + encoded full HP, empty bag, explicit-save round trip of the party and the (11,5)
  location, and the restored-party negative.
- `tst/battle_presets_test.hpp` — `BattlePresets::beta` checked through the shared
  `applyPlayerPreset` load seam (host).
- `tst/fxdatatest/preset_teams_test.hpp` — `BattlePresets::beta` through the same seam on device
  (`test_battle_presets.ino`).
- `tools/tests/beta-area_test.mjs` — pins that the shipping ordinary branch calls
  `battle::applyPlayerPreset(player, BattlePresets::beta)`, keeps
  `item::setBattleInventory(&player.inventory)`, no longer calls `player.basic()`, and stays inside
  the `if (!restored)` guard.

No data or generated change: `data/beta-area.toml` and the generated `BattlePresets::beta`
(`data/battle-presets.toml`) already agreed, so `make gen` (run by `test-beta-area`) left no drift:
`git status` lists only the five source/test files above plus the pre-existing
`.beads/interactions.jsonl`. Spawn literal (o845) and v3 offsets untouched.

### Commands and results

```
make test BUILD_DIR=build            # Total Passed 240245 / Failed 0; FreshBetaStartTest 49/0;
                                     #   Battle Presets Suite 70/0
make testvm BUILD_DIR=build          # Total Passed 286 / Failed 0
make test-beta-area BUILD_DIR=build  # node: tests 12 / pass 12 / fail 0
make verify-generated BUILD_DIR=build  # exit 0, no manifest delta
make fxtest-headless BUILD_DIR=build ARDENS="$ARDENS" \
  FXTEST_INOS='tst/fxdatatest/test_world_player.ino tst/fxdatatest/test_save.ino \
               tst/fxdatatest/test_stack.ino tst/fxdatatest/test_battle_presets.ino'
  # test_world_player   PASSED=71   FAILED=0  (render headroom 289)
  # test_save           PASSED=3283 FAILED=0
  # test_stack          PASSED=4    FAILED=0  (stack headroom 598)
  # test_battle_presets PASSED=106  FAILED=0
make ram BUILD_DIR=build             # AVR_FLASH_BYTES=22506 AVR_STATIC_RAM_BYTES=1678
```

### Fresh vs restored proof

- Fresh (host `FreshBetaStartTest`, same seams as the sketch): `applyPlayerPreset(player,
  BattlePresets::beta)` + `setBattleInventory` + `seedParty` yields ids 0/3/6, level 3, full HP,
  masks 2/4/2; store slots 0/1/2 hold `exp=levelExpThreshold(3)=27`, `hpEncoded=maxHP+1`,
  `attackMask` 2/4/2; `storePartySlot(HP)` match; the bag is empty and is `item::battleInventory()`.
  An explicit save restores the party and the authored (11,5) location.
- Restored (negative): a valid checkpoint holding a different party (legacy species-only default,
  ids 0/1/2 at level 31, location 0x4321) is loaded with `restored==true`; the ordinary boot
  decision then skips the beta start, and the party ids/level and location are unchanged. The
  shipping guard itself is pinned by the `beta-area_test.mjs` source test, so the fresh-only
  wiring cannot silently regress.

### Resources

- control (`make ram BUILD_DIR=build`, clean tree): Program **22520** B, Data **1678** B.
- candidate: Program **22506** B, Data **1678** B → **-14 B flash, 0 B static RAM** (LTO folds the
  now-shared `applyPlayerPreset` over the inlined `basic()`). Program free 7190 B (target ≤29184,
  hard ≤29696); static free 482 B/2560. Device stack headroom 598 B, unchanged.

### Deviations

- Added `tst/fxdatatest/test_battle_presets.ino` to the focused device run (it now covers the beta
  preset) beyond the three suites named in the dispatch.
- Ran `make clean-build-artifacts BUILD_DIR=build` preview then `CLEAN_APPLY=1` as instructed; it
  removed only 108 `.o`/`.d` candidates (2.60 MB) and preserved the ELF/HEX/log evidence.

### Wall time

Single worker ≈20 min: read bead/source + design ≈6 min; edits + tests ≈4 min; control AVR build
≈1 min; host/VM/node ≈1 min; 4-suite device run ≈46 s; candidate `make ram` + cleanup + report
remainder.

### Storage

`df -h .` 64 GiB free before/after (≥10 GiB reserve held). `du -sh build` 1.9 G before/after.
Logs: `build/jp8.11.24-{test,testvm,test-beta-area,fxtest,ram,verify-generated}.log`; candidate ELF
`build/CreatureGathererFX.ino.elf` (22506 B) retained. No `cgfx-pack.*` stage remained after
`make gen`.

## b2a.7 — legacy sprite sheet disposition

Bead: `CreatureGathererFX-b2a.7` (P3, in_progress — left in_progress per dispatch). HEAD `e01978d`.
Analysis/disposition only; no source edits, no `make gen`, no AVR. No git commit/push.

### Method

Whole-tree word searches (`grep -rIw <symbol>`, excluding `.git`, `build`, `archive`,
`.battle-carve-spike`, `harness/workspaces`) for each carray symbol in `src/`, `tst/`, `tools/`,
`docs/`, plus PNG decode via `tools/creature-sprite-export.mjs` for palette checks. Packed byte
sizes parsed from `fxdata/Sprites.txt`; offsets cross-checked against `src/fxdata.h`.

### Sheet -> reference map

| Packed symbol | Bytes | Shipping `src/` use | Active device-suite use | Source PNG |
|---|---:|---|---|---|
| numbersblack | 1400 | `common.hpp:32` | — | numbersblack_7x8.png |
| numberswhite | 1400 | `common.hpp:25` | — | numberswhite_7x8.png |
| singlenumberswhite | 60 | `common.hpp:24`, `ArenaView.cpp:11,15` | — | singlenumberswhite_3x8.png |
| singlenumbersblack | 60 | `common.hpp:31` | — | singlenumbersblack_3x8.png |
| tiles | 24576 | `draw.h:429` | `blit_test`, `tiles_test` | tiles_16x16.png |
| battleMenu | 384 | `DialogMenu.cpp:73` | `blit_test`, `dialogs_test` | battleMenu_128x24.png |
| creatureSprites | 8192 | none (only comment `Creature.cpp:49`) | none | creatureSprites_32x32.png |
| fieldBacground | 1024 | none | none | fieldBacground_128x32.png |
| characterSheet | 384 | none | `blit_test.hpp:17` | characterSheet_16x16.png |
| NewecreatureSprites | 32768 | none | `blit_test.hpp:18-19`, `battlepresentation_fixture.hpp:45` | NewecreatureSprites_32x32.png |
| ecreatureSprites | 8192 | none | none | ecreatureSprites_32x32.png |
| fightMenu | 1536 | none | none | fightMenu_128x24.png |
| ArduFont | 615 | none | none | ArduFont_5x6.png |
| moveInfo | 240 | none (`draw.h:196` param + `MenuV2::moveInfo()` are other symbols) | `blit_test.hpp:21` | moveInfo_60x30.png |
| worldTiles | 512 | none | none | worldTiles_16x16.png |
| maskedFont | 12288 | none | `blit_test.hpp:11`, `tiles_test.hpp:251` (end-of-`tiles` address marker) | maskedFont_16x24.png |
| npc | 64 | none | none | npc_16x16.png |
| arduboyFont | 1536 | none (only comment `draw.h:189`) | none | arduboyFont_6x8.png |
| blankMenu | 768 | none | none | blankMenu_128x24.png |
| ArduFontTrimmed | 375 | none | `battlepresentation_test.hpp:56,281`, `dialogs_test.hpp:25`, `battle_damage_color_test.hpp:25` | ArduFontTrimmed_5x6.png |
| tilesheet | 1024 | none | none | tilesheet_16x16.png |
| letters | 16384 | none (only comments in `Font4x6.h`/`BattlePresenter.cpp`) | none | letters_16x16.png |

Kept-by-contract (outside the 16): `fontTrimmed` (`Text.cpp:13`, the live 5x6 font), the appended
native sheets `battleSprites48`/`battleOptions48`/`worldPlayerSprites`/`lureWorldTiles`/`duelIcons`/
`duelChartFixture` (shipping renderers), and `doubled` (test-only).

### Dispositions

**Retained — active device-suite reference (5).** Removal would break passing FX suites
(`test_blit`, `test_tiles`, `test_battlepresentation`, `test_dialog`, `test_battle_damage_color`,
all in the `tst/fxdatatest/*.ino` wildcard), so each needs coordinated test-fixture edits plus a
generation-lane regen/rebaseline:
- `characterSheet` (384) — `blit_test` draw case.
- `NewecreatureSprites` (32768) — legacy 32x32 battle parity path; superseded in shipping by
  `battleSprites48` (`BattleScene.cpp:248`, `draw.h:317,325`) but still the fixture's draw source.
- `maskedFont` (12288) — used only as the end-of-`tiles` address marker
  (`(maskedFont - tiles)/32 - 1`); content unused, address load-bearing for two suites.
- `moveInfo` (240) — `blit_test` draw case; the shipping `moveInfo` names are `MenuV2::moveInfo()`
  (RAM buffer) and the `draw.h` parameter, not this packed blob.
- `ArduFontTrimmed` (375) — the live 5x6 glyph source for three FX suites; shipping text uses
  `fontTrimmed` instead.

**Superseded / removable — no shipping, test, or tooling reference (11).** Each appears only in the
generated `src/fxdata.h`, the frozen migration fixture `tools/tests/fixtures/fxlayout-old-304.toml`
and `docs/flash-inventory.md`. Removal is justified by the evidence chain but is a generation-lane
change (see plan) and was NOT executed in this session:
- `creatureSprites` (8192), `ecreatureSprites` (8192), `NewecreatureSprites`-predecessor art —
  superseded by `battleSprites48` native 48px views.
- `fightMenu` (1536) + `blankMenu` (768) — superseded by `battleMenu`/`battleOptions48` and the
  runtime menu.
- `ArduFont` (615) + `arduboyFont` (1536) — superseded by `fontTrimmed`.
- `worldTiles` (512) + `tilesheet` (1024) — superseded by `tiles`/`lureWorldTiles`.
- `letters` (16384) — superseded by the generated string glyphs; owner feedback moved off the
  letter-arrow menu.
- `fieldBacground` (1024) — the battle-field draw was removed by the owner; nothing renders it.
- `npc` (64) — NPC actor renderer is not integrated (`docs/flash-inventory.md` line 127).

**Archived, not deleted.** Every source PNG above is git-tracked authored art. Removal here means
dropping the `fxsprites.toml` declarations and archiving the PNGs with provenance, never deleting
unknown assets.

### Source-palette issues (evidence)

PNG decode (RGBA, 0/255 binary test) confirms the three bead findings:
- `fieldBacground_128x32.png` 128x32: **2760 gray** pixels (every opaque pixel is gray 64).
- `letters_16x16.png` 256x256: **4758 partial-alpha** pixels.
- `blankMenu_128x24.png` source is **128x25**; the packer uses the 128x24 filename frame and drops
  the 25th row (2816 coherent opaque of 3200).
All three are in the removable group, so normalization is moot unless the source is retained.

### Deferred removal plan (generation lane; needs single-lane ownership)

1. Remove the 11 satisfied-break declarations from `fxsprites.toml` set #1 (owned by `86e`;
   serialize), keeping `fxlayout.toml` declaration order ABI intact.
2. `make gen BUILD_DIR=<shared>` → `fxdata/Sprites.txt`, `src/fxdata.h`, `dist/fxdata.bin`, manifest
   all regenerate together; stage the generated set as one commit.
3. `tools/tests/fixtures/fxlayout-old-304.toml` carray entries resolve against the same
   `fxdata/Sprites.txt`; drop the removed symbols there too or the migration/parity fixture fails.
4. Re-baseline `tools/tests/pack-parity_test.sh` `expected` SHA with old->new plus cause.
5. Archive the removed PNGs under `archive/` with provenance; run `make verify-generated`,
   `make test-pack-parity`, and the touched device suites.

### Verification run (read-only, no source change)

```
$ make verify-generated BUILD_DIR=build
(exit 0, no output)

$ make test-pack-parity BUILD_DIR=build
layout equivalence: PASS
layout equivalence: perturbation diagnostic: PASS
pack parity: PASS
(exit 0)
```

`make pack` (invoked by pack-parity) rewrote `src/fxdata.h` + `dist/*`; `git status --short` shows
`src/fxdata.h`/`dist/` byte-identical (deterministic), only `.beads/interactions.jsonl` modified by
`bd`. No `build/cgfx-pack.*` stage remained. No tracked source changes were made by this bead.

### Wall time

~8 min single worker: read bead + repo recon ≈4 min; reference/palette evidence ≈2 min;
verify-generated + pack-parity + report ≈2 min.

### Storage

`df -h .` 64 GiB free before/after (≥10 GiB reserve held). `du -sh build` 1.9 G unchanged; the
pack stage self-cleaned.

## jp8.11.6 — gatherable tier rate

Worker: DeepSeek V4.1 Flash. HEAD e01978d. ARDENS=.../Ardens.app/Contents/MacOS/Ardens.
Bead left `in_progress`; no commit/push.

### Change

`src/engine/world/StepEvent.cpp`: the wild roll now snapshots `LureZone::active()` and the
authored per-tier rate (`item::lureRateOf(LureZone::lureTier(LureZone::id()))`, i.e. `1<<tier`)
before `enterBattle()` and passes them to `battleSession().beginWild(...)`. Previously it always
passed `false, 0`. Inert zones keep the ordinary wild battle (gatherable false, rate 0). Read
once at entry; no per-step/per-frame metadata read added.

Tests: host `tst/encounter_test.hpp` `EncounterWorldIntegrationTest` (inert -> not gatherable /
tierRate 0; two tier proofs), device `tst/fxdatatest/test_world_player.ino` (tier-0 -> rate 1,
tier-1 -> rate 2, plus the gatherable escape now navigates the two-row action page).

### Commands and results

```
$ make test BUILD_DIR=build            # host: Passed 240251 Failed 0 (world suite Passed 258 Failed 0)
$ make testvm BUILD_DIR=build          # Passed 286 Failed 0
$ make test-beta-area BUILD_DIR=build  # tests 12 pass 12 fail 0
$ make fxtest-headless BUILD_DIR=build ARDENS="$ARDENS" \
    FXTEST_INOS='tst/fxdatatest/test_encounter.ino tst/fxdatatest/test_battle_duel.ino tst/fxdatatest/test_stack.ino'
  test_encounter PASSED=47 FAILED=0
  test_battle_duel PASSED=106 FAILED=0
  test_stack PASSED=4 FAILED=0  (mode transition headroom=0x2B7(695), stack headroom=0x256(598))
$ make fxtest-headless BUILD_DIR=build ARDENS="$ARDENS" FXTEST_INOS='tst/fxdatatest/test_world_player.ino'
  test_world_player PASSED=76 FAILED=0
$ make ram BUILD_DIR=build
  RAM_FLASH_BYTES=22564  RAM_STATIC_BYTES=1678  RAM_FREE_BYTES=882
```

### Two-tier proof

Authored mapping `item::lureRateOf(tier)==1<<tier` (pinned in `tst/item_test.hpp` and
`tools/tests/beta-area_test.mjs`: `rate == 1<<lure_tier`). Same encounter tile, two deposits:

- tier 0 -> `gather.tierRate == 1` (host and `test_world_player` both assert).
- tier 1 -> `gather.tierRate == 2`, i.e. the distinct table rate, not a constant.
- inert zone -> `gatherable == false`, `tierRate == 0` (host).

### Resource delta

`make ram` (shipping FX, default flags): flash 22564 B vs reference 22506 B = **+58 B**; static
SRAM 1678 B = **+0 B**. Well inside the 29696 B hard cap; the test sketch
`test_world_player` uses 29030/29696 B (device proof only).

### Deviations

- The bead's historical VERIFY named `build/box-adoption-gate` and older suites; used the
  dispatcher-supplied `BUILD_DIR=build` and suite list, plus `test_world_player.ino` for the
  end-to-end AVR tier proof (only device suite that drives `onStep`).
- No deposit/refusal (`types 4..7`) change was made: the dispatcher SCOPE is the entry tier rate,
  and no reachable first-area lure has type >= 4. No test demands it.
- Had to navigate `MENU_NAV_DOWN` to ESCAPE in `test_world_player` because the lure battle is now
  correctly gatherable (action page gains the GATHER row).

### Storage

`df -h . /private/tmp`: 64 GiB free before/after (>=10 GiB reserve). `du -sh build` 1.9 G
before/after. `make clean-build-artifacts BUILD_DIR=build` preview then `CLEAN_APPLY=1` removed
108 disposable arduino-build objects (2,599,901 B). No generated/tracked drift; only
`StepEvent.cpp`, `tst/encounter_test.hpp`, `tst/fxdatatest/test_world_player.ino` and
`.beads/interactions.jsonl` modified.

### Wall time

~40 min single worker: recon/read ~10 min; source+test edits ~10 min; host/VM/beta-area ~5 min;
device suites (incl. one fix iteration for the lazy property window) ~12 min; ram + clean + report
~3 min.

## jp8.4.11 — menu/dialog residual

Audit/measurement bead. HEAD `58eb8e7`. **No source, test, generated or resource
change.** Follow-up bead filed: `CreatureGathererFX-obbp`. Bead left `in_progress`.
Work was static + host only (off the AVR/generation lane; the single lane was
owned by another worker whose uncommitted `BattleFlow.cpp` / `duel_session_integration_test.hpp`
edits were present mid-run — see Deviations).

### Method / evidence base

- Committed HEAD shipping ELF `build/CreatureGathererFX.ino.elf` (text 22522 +
  data 42 = 22564 B; `build/build.log` "22564 bytes (75%)"), mtime `2026-10-16
  13:04:01`, immediately before commit `58eb8e7` (13:04:53). Working tree at
  audit time: only `.beads/interactions.jsonl`, `src/engine/battle/BattleFlow.cpp`
  and `tst/duel_session_integration_test.hpp` were dirty (all the other worker's,
  not mine); every `src/engine/menu/*` and menu/dialog source file is unmodified
  at HEAD.
- `avr-nm -C --size-sort --print-size` / `avr-size` on that ELF (tool at
  `~/Library/Arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7/bin/`).
- Build config: `AVR_SHIPPING_CPP_FLAGS` (Makefile L36) = `-DCGFX_SHIPPING_NO_USB
  -fno-inline-small-functions -DCGFX_BATTLE_NO_FACTS -DCGFX_BATTLE_COMPACT_RESULT
  -DCGFX_COMPACT_WARP_VM -DCGFX_DUEL_BATTLE`. None of
  `CGFX_FULL_WORLD_VM / TEST / FX_READ_COUNTER / CGFX_TRAINER_DEMO / CGFX_ARENA_DEMO
  / CGFX_WILD_DEMO` is defined, so `CGFX_DIALOG_QUEUE_ENABLED == 0` (globals.hpp:16-25).
- Host: `make test` -> 240251 passed / 0 failed; world suite 258 / 0;
  `make testvm` -> 286 / 0.

### Headline result: zero whole-image win

The default shipping image already excludes **all** superseded menu/dialog code.
`avr-nm -C build/CreatureGathererFX.ino.elf` symbol counts (0 = absent):

```
MenuV2 0  DialogMenu 0  DialogQueue 0  MenuNav 0  menuNavMove 0  menuDescFor 0
printMenu 0  drawPopMenu 0  prepareHead 0  pushMenu 0  pushEvent 0
menuEdgeButtons 0  loadScriptText 0  drawScriptText 0  BattlePresenter 0
```

The ELF carries `_ZL*.lto_priv.*` symbols, i.e. whole-program LTO, plus link GC.
The menu/dialog translation units are still **compiled** (their `.cpp.o` exist in
the arduino cache and in `build/fxtest/*/…/engine/menu/`), then discarded before
the image. Because the TUs are LTO objects, per-TU object size reads `0 text / 0
data` (`avr-size`), so per-symbol/TU arithmetic is not a usable basis; whole-image
symbol absence is. Conclusion: removing the superseded `src/engine/menu/*` sources
would have an attributable shipping delta of **0 B flash / 0 B static SRAM**.
Per the bead's step 3, that is a documented **no-op/reject**, not a retained
cleanup.

### Why the code is already excluded (load-bearing vs dead)

| Residual | Where | Shipping | Status / size basis |
|---|---|---|---|
| `MenuV2Legacy.cpp` (`menuEdgeButtons`, `MenuV2::run(session)`, `MenuV2::printMenu`) | `src/engine/menu/MenuV2Legacy.cpp` 2051 B / 63 L | absent (0 syms) | **Dead in shipping + host** (not in `TEST_SOURCES`, Makefile L141-190). Live only for device fxtest sketches that call `menu.printMenu` (`battlepresentation_test.hpp:163,202,709`, `stack_test.hpp:127`). |
| `MenuV2` navigator | `MenuV2.cpp` 9556 B / `MenuV2.hpp` 1784 B | absent; `MenuV2 menu` has **no shipping definition** (only `extern` in globals.hpp:29); `menu.clear()` guarded `#if TEST||FX_READ_COUNTER` (BattleFlow.cpp:47-49) | Superseded by `battle::BattleMenu` (`mode.ui.menu`, BattleFlow.cpp). Load-bearing for host tests (`menu_test`, `transition_test`, `battle_flow_test`, `duel_menu_test`, `duel_session_integration_test`). |
| `DialogMenu` / popup queue | `DialogMenu.cpp` 3134 B, `DialogQueue.cpp` 1007 B, `DialogMenu.hpp` 478 B | absent; global `dialogMenu` defined only `#if CGFX_DIALOG_QUEUE_ENABLED` (CreatureGathererFX.ino:49-51); `drawPopMenu` call guarded `#if FULL_WORLD_VM||TEST||FX_READ_COUNTER` (.ino:227-231); `dialogMenu.clear()` calls all guarded (BattleFlow.cpp:50-52,87-89, .ino:135) | Producers: `pushMenu` from `ScriptVm` `queueScriptText` — but the only shipping caller is the **full** `ScriptVm::run()` (ScriptVM.cpp:117-151), unreachable under `CGFX_COMPACT_WARP_VM` (World.cpp:162-168 routes to `runWarpVM`, which has no Msg path). So the whole dialog queue is dead in shipping by build configuration. Live for host (`dialog_test`, `world_interact_test`, `step_test`, `transition_test`, `scripts`) and device `test_dialog`/`test_scripts`/`test_stack`. |
| TEXT kind / `pushEvent` | `DialogQueue.cpp:31-34`, `DialogMenu.cpp:62-67,76-81` | absent | No shipping producer (`jp8.4.6` disposition). Retained host-verified kind; same 0-B shipping basis as the rest of `src/engine/menu`. |
| `MenuNav.cpp` (`menuNavMove`, `menuDescFor`) | 1321 B | absent | Functions dead in shipping; only the `MenuNav.hpp` nav-bit **constants** ship (macro/enum, no code) via `.ino battleEdgeButtons`. Live for host `MenuNavSuite`. |
| `PackedMoveInfo.hpp` | 1241 B | absent | Header used only by `MenuV2.cpp`. |
| Redundant copy | `menuEdgeButtons()` MenuV2Legacy.cpp:9-18 == `battleEdgeButtons()` CreatureGathererFX.ino:193-202 | only the .ino copy ships | Byte-identical button-mask builder; the duplicate is dead, so removal saves 0 B but removes a drift hazard. |
| Host stub | `tst/src/DialogMenu.cpp` 118 B | n/a | Host-only stub of the real draw path (`TEST_SOURCES` L146); duplicates the device draw contract by design. |

### Live menu/dialog surface (do not touch)

`battle::BattleMenu` (102 B `beginDuel` present in ELF, remainder inlined into
`main`), `BattleFlow::duelSyncUi` (66 B), `battle::BattleScene`, `battle::cacheCreatureName`
(102 B), `BattleFeedback`, and `ModeState` (193 B) are the shipping menu/dialog
path. `BattleFeedback`/`BattlePresenter` are absent as standalone symbols only
because they are inlined/excluded — the feedback arm feeds non-duel profiles and
`FX_READ_COUNTER` tests, so it is not removable without measurement (below).

### Residual with an AVR-only size (bounded follow-up filed)

One residual cannot be sized off-lane: the shipping duel never uses the feedback
footer, but its code survives in `main`:

- `BattleSession::advance()` duel branch (BattleSession.cpp:847-882) `return false`
  on **every** path; only the non-duel branch (L888-970) sets
  `SessionPhase::Presenting`, and `isPresenting()` is false throughout a duel.
- Therefore `BattleFlow::update` L93-104 (`feedback.update/done`, `finishPresentation`)
  and L147-156 (`beginFeedback`) are unreachable in a duel, and `render()`
  (CreatureGathererFX.ino:257-260) never draws feedback under `CGFX_DUEL_BATTLE`.
- `avr-nm` still shows `battle::BattleSession::finishPresentation` in the ELF, so
  LTO did **not** constant-fold the arm; `BattleFeedback::{begin,update,done,draw,reset}`
  appear inlined into `main` rather than dropped.

Size can only be proven by a whole-image AVR comparison. Filed as
**`CreatureGathererFX-obbp`** (discovered-from jp8.4.11) with the exact control/
candidate recipe, touched device suites and stack-headroom gate. No AVR lane was
consumed here.

### Commands + tails

```
$ make test BUILD_DIR=build          # committed HEAD source (clean snapshot, see Deviations)
Total Passed: 240251
Total Failed: 0
All tests passed!
Total Passed: 258                    # world suite
Total Failed: 0

$ make testvm BUILD_DIR=build
Total Passed: 286
Total Failed: 0
All tests passed!

$ avr-size build/CreatureGathererFX.ino.elf
   text     data     bss     dec     hex filename
  22522       42    1636   24200    5e88 build/CreatureGathererFX.ino.elf

$ avr-nm -C build/CreatureGathererFX.ino.elf | grep -c -E 'MenuV2|DialogMenu|printMenu|drawPopMenu|menuEdgeButtons'
0
```

### Deviations

- **Host runs were on committed HEAD, not the dirty working tree.** The working
  tree carries a concurrent worker's uncommitted `src/engine/battle/BattleFlow.cpp`
  (+22 L) and `tst/duel_session_integration_test.hpp` (+184 L). To keep the audit
  pinned to `58eb8e7` I ran `make test`/`make testvm` in a `git archive HEAD`
  snapshot under the approved temp root, copying the (unmodified-at-HEAD)
  `fxdata/generated/` set in so the host build links without `make gen`. Snapshot
  removed afterward; shared `build/` untouched. The three menu/dialog source files
  audited are unmodified at HEAD, so the static findings are HEAD findings.
- The bead `design` VERIFY lists `tst/fxdatatest/test_menurun.ino`; that suite is
  **retired** (not in `tst/fxdatatest/`). No device run was made regardless (lane
  owned by another worker; this bead changes no source).
- `build/CreatureGathererFX.ino.elf` is pre-existing evidence not produced by this
  audit; it was used read-only and not rebuilt.

### Storage

`df -h .` 64 GiB free before/after (>=10 GiB reserve). Shared `build/` reused
read-only for host objects; one small head-snapshot under the approved temp root
created then removed. `du -sh build` unchanged (1.9 G). No new build trees, no
`make gen`, no pack stage, no AVR artifacts written. `.beads/interactions.jsonl`
was already modified by the tracker (not by me).

### Wall time

Worker ~45 min wall: source/config/eLF recon ~20 min; host+VM runs (dirty tree +
HEAD snapshot) ~5 min; symbol/size audit and follow-up bead ~12 min; snapshot
setup/cleanup + report remainder. Gate/orchestrator not run here.

## jp8.11.7 — gather commit

Worker: DeepSeek V4.1 Flash. HEAD 58eb8e7. ARDENS=.../Ardens.app/Contents/MacOS/Ardens.
Bead left `in_progress`; no commit/push.

### Change

`src/engine/battle/BattleFlow.cpp`: at the Gathered terminal, before `exitBattle()`
destroys the battle union, a new file-local `commitGathered(session)` builds one
8-byte `StoreRecord` from the resident opponent (`active[Opponent]`) and calls the
existing `storeAdd` once:

- `id` = opponent species id
- `exp` = `levelExpThreshold(level)` (fresh instance at its current level)
- `attackMask` = opponent's folded 5-bit duel mask (matches the current StoreRecord
  wire contract; the retired `moves[4]` is not resurrected)
- `reserved[3]` = 0
- `hpEncoded` = `hp >= maxHp ? 0 : hp + 1` (pinned byte7: 0 = legacy/full, else value-1)

No offsets/version change; no new collection path. A refused add (full 64, dirty
journal/index/log) leaves the collection untouched. `finishBattle` runs once per
battle (the terminal deactivates the session), so the commit is exactly once.

Tests:
- Host `tst/duel_session_integration_test.hpp` (duel suite, runs under `make test`):
  `duelGatherCommitOnce` (fields + damaged HP + exactly-once + no re-commit on later
  frames), `duelGatherCommitFullHP` (legacy zero byte round-trips to derived max),
  `duelGatherCommitPersistence` (explicit checkpoint + reboot re-reads the instance),
  `duelGatherCommitFullStore` (full 64 refuses, first/last records untouched),
  `duelGatherIncompleteNoCommit` (a gather that never reaches the need, then Escape,
  commits nothing).
- Device `tst/fxdatatest/duel_session_integration_test.hpp` (runs in `test_battle_duel`):
  `duelDeviceGatherCommit` drives the real flow to the Gathered terminal on AVR and
  asserts the committed species/exp/mask, the damaged-HP byte7 encoding (55 -> 56),
  exactly one occupied slot, and no re-commit.

### Commands and results

```
$ make test BUILD_DIR=build
  host Total Passed: 240348  Failed: 0 ; world Total Passed: 258  Failed: 0
$ make testvm BUILD_DIR=build
  Total Passed: 286  Failed: 0
$ make test-beta-area BUILD_DIR=build
  tests 12  pass 12  fail 0
$ make fxtest-headless BUILD_DIR=build ARDENS="$ARDENS" FXTEST_MS=30000 \
    FXTEST_INOS='tst/fxdatatest/test_save.ino tst/fxdatatest/test_world_player.ino \
                 tst/fxdatatest/test_battle_duel.ino tst/fxdatatest/test_stack.ino'
  test_save PASSED=3283 FAILED=0
  test_world_player PASSED=76 FAILED=0
  test_battle_duel PASSED=115 FAILED=0   (was 106; +9 device gather-commit asserts)
  test_stack PASSED=4 FAILED=0  (mode-transition headroom 0x2B7 (695); stack headroom 0x256 (598))
# extra safety for the shared device duel header:
$ make fxtest-headless ... FXTEST_INOS='tst/fxdatatest/test_duel_ui.ino tst/fxdatatest/test_duel_resources.ino'
  test_duel_ui PASSED=65 FAILED=0 ; test_duel_resources PASSED=16 FAILED=0
$ make ram BUILD_DIR=build
  RAM_FLASH_BYTES=22768  RAM_STATIC_BYTES=1678  RAM_FREE_BYTES=882
```

### Proofs

- Exactly once: host `duelGatherCommitOnce` shows one occupied slot after the terminal
  and still one after 8 further world frames; device `duelDeviceGatherCommit` shows one
  occupied slot and one after 8 further frames. `finishBattle` is a single terminal edge.
- Full-store refusal (lossless): host `duelGatherCommitFullStore` fills all 64 slots
  (with the checkpoint cadence), gathers, and proves `occupied == 64`, first/last record
  ids unchanged, no wrapping slot. This is on top of the existing
  `CollectionCapacityTest`/`test_saveCollectionCapacity` full-64 refusal coverage.
- Failed gather no-commit: host `duelGatherIncompleteNoCommit` (progress 1 of need 8,
  then Escape) leaves the collection empty.
- HP encoding: damaged foe 55 -> byte7 56 -> `storeInstanceHP` 55 (host + device); full
  foe -> byte7 0 -> derived max (host).
- Save/reboot: host `duelGatherCommitPersistence` builds a checkpoint from the journal
  record and boots it back with species/exp/mask/HP intact.

### Resource delta

`make ram` (shipping FX, default flags): flash 22768 B vs reference 22564 B = **+204 B**;
static SRAM 1678 B = **+0 B**. Well inside the 29696 B hard cap and the <=29184 B planning
target; 882 B static free, painted stack headroom unchanged at 598 B.

### Deviations

- Device proof lives in `tst/fxdatatest/duel_session_integration_test.hpp`
  (`test_battle_duel.ino`), not `test_world_player.ino`. The current
  `test_world_player` sketch is already at 29328/29696 B (368 B free); the added
  end-to-end gather block pushed it to 30306 B (102%). The device `test_battle_duel`
  sketch had ~3400 B headroom, so the AVR end-to-end gather-commit proof was moved
  there. `test_world_player.ino` is unmodified.
- Design STEPS step 1's "actual four semantic move IDs" predates the retired
  `StoreRecord.moves[4]`; the current wire contract carries the folded 5-bit
  `attackMask` (jp8.11.22 / 4w89.2), which is what is stored.
- Design STEPS step 3's "explicit save/retry feedback" is not wired: the duel result
  dwell draws the Gathered caption before the terminal commit, there is no authored
  refusal caption in the generated text set, and no world-side post-battle message
  exists. Refusal is truthful and lossless (collection untouched, no false slot), which
  the orchestrator's stated negative verifies; a refusal caption would need generated
  text and is a follow-up if required.
- Run the focused VERIFY suites (host/VM/beta-area + the four named device suites);
  the orchestrator runs the settled full gate.

### Storage

`df -h . /private/tmp`: 64 GiB free before/after (>=10 GiB reserve). `du -sh build` 1.9 G
before/after. `make clean-build-artifacts BUILD_DIR=build` preview then `CLEAN_APPLY=1`
removed 108 disposable arduino-build objects (2,604,375 B). No `cgfx-pack.*` stage; no
generated/tracked drift. Modified: `src/engine/battle/BattleFlow.cpp`,
`tst/duel_session_integration_test.hpp`, `tst/fxdatatest/duel_session_integration_test.hpp`,
`.beads/interactions.jsonl`.

### Wall time

~70 min single worker: recon/read ~20 min; source + host/device tests ~20 min; host/VM/
beta-area iterations ~10 min; flattening the too-large `test_world_player` proof into the
device duel suite + focused device runs ~15 min; ram + clean + report ~5 min.

## b2a.8 — tiles palette normalization

Bead: `CreatureGathererFX-b2a.8` (P3). HEAD `f9438eb`, left `in_progress` per dispatch; no git
commit/push. Scope: normalize the legacy palette of the Tiled source sheet `images/tiles.png`
to the native 1bpp authoring convention without changing any authored pixel the generator
consumes. Another worker owns the AVR lane; the only generation run here was a short host
`make gen` (no AVR build/gate).

### Starting state (measured, HEAD f9438eb)

`images/tiles.png` is RGBA8 256x768 (the cf7 joiner rewrote the former indexed 2bpp source).
Its RGBA histogram carried the legacy four grays: `0,0,0,255` (121027), `170,170,170,255`
(31114), `255,255,255,255` (26185), `85,85,85,255` (18282). `images/tiles_16x16.png` (the packed
`tiles` symbol) was already binary: only `0,0,0,255` (139309) and `255,255,255,255` (57299).
The two sheets differ by 49396 pixels across 334 cells; a per-pixel confusion table showed the
packed sheet is exactly the legacy grays thresholded at 128 (`0/85 -> 0`, `170/255 -> 255`,
alpha 255). `images/tiles.png` is a `cgfx-project.json` (`tileset_png`/`map_tileset_png`) and
`fxdata/generated/manifest.json` input, but the cart `tiles` symbol is packed from
`images/tiles_16x16.png` (per `fxsprites.toml`).

### What was normalized

`tools/join-simple-environments16.mjs` — the only writer of both shipping sheets — now collapses
the Tiled source sheet to the project convention after installing the catalogue cells:
`normalizeBinary()` maps each pixel to binary `0/255` RGB with `0/255` alpha (opaque => luminance
threshold at 128, transparent => transparent black). Scope is the `BINARY_SHEETS` set
(`images/tiles.png`); the packed sheet `images/tiles_16x16.png` is left untouched. Installed GID
cells are already `0/255` opaque so they stay byte-exact. A post-write invariant asserts
`images/tiles.png` is binary. The joiner stays idempotent for every catalogue/review output.

`tools/tests/join-simple-environments16_test.mjs` — refreshed expectation: the
"install byte-exact ... leave every other cell at HEAD" test now expects an untouched
Tiled-source cell to equal HEAD collapsed the same way (`BINARY_SHEETS` + `binaryPixel`), while
`images/tiles_16x16.png` still equals HEAD; test name updated. The expectation is idempotent, so
it holds before and after this change is committed.

### Result (measured)

- `images/tiles.png` RGBA histogram after: `0,0,0,255` (139309) and `255,255,255,255` (57299)
  only. Pixel-identical to `images/tiles_16x16.png` (0 diffs of 196608).
- Installed-cell pixel diffs vs HEAD: **0**. Non-installed cells changed: 334; unchanged: 410
  (cells already binary/empty).
- Pixel-equivalence proof (generator output unchanged): every tracked packed/generated artifact
  is byte-identical to HEAD. `dist/fxdata.bin` = `fcda6f8b59f1301d68724a0845d1123da6763e7746674
  94b58eb1e21d63cf5bb` = the `pack-parity_test.sh` `expected=` and the HEAD baseline;
  `dist/fxdata-data.bin` = `15d428ce48...`; `dist/fxdata-save.bin`, `src/fxdata.h`,
  `src/vm/opcodes.hpp`, `src/flags/*`, `fxdata/generated/*`, `tst/fxdatatest/generated/*` all
  unchanged. Only `fxdata/generated/manifest.json` (gitignored) changed, as it must: it records
  the `images/tiles.png` provenance hash.
- `maps/tiles.tsx` properties unchanged (covered by the join test) and `ardurpg.toml`
  `tileset_path` unchanged.

### Exact commands, rc, results

```
node tools/join-simple-environments16.mjs                 rc=0  0.30 s (idempotent catalogue/docs)
make gen BUILD_DIR=build                                  rc=0  6.5 s (host gen+pack; AVR lane untouched)
make verify-generated BUILD_DIR=build                     rc=0
make test-pack-parity BUILD_DIR=build                     rc=0
    layout equivalence: PASS ; perturbation diagnostic: PASS ; pack parity: PASS
node --test tools/tests/join-simple-environments16_test.mjs
    7 pass / 0 fail
node --test tools/tests/join-simple-environments16_test.mjs tools/tests/simple-urban_test.mjs \
  tools/tests/simple-environments_test.mjs tools/tests/environment32-viewer_test.mjs \
  tools/tests/environment32-scene_test.mjs tools/tests/habitat-terrain_test.mjs \
  tools/tests/lure-world-spike_test.mjs tools/tests/world-arranged-comparison_test.mjs
    68 pass / 0 fail
```

`git status --porcelain`: `images/tiles.png`, `tools/join-simple-environments16.mjs`,
`tools/tests/join-simple-environments16_test.mjs` (plus `.beads/interactions.jsonl`). No stray
`build/cgfx-pack.*` stage remained; `build` = 1.9 GiB shared, 63 GiB free.

### Deviations

- Design step 1 offered "re-author binary **or** record an explicit no-change decision"; the
  binary re-author path was taken.
- Design step 2's "regenerate the viewer and Tiled outputs through the existing joiner" was
  satisfied by the joiner run; the combined16 catalogue, lure-world crops, `docs/*.html`,
  `town.tmj` and `tileset.tsx` were all byte-identical (joiner `write` is compare-before-write).
- Normalization is scoped to the Tiled source (`images/tiles.png`) per the bead; the packed
  sheet is already binary and was intentionally left untouched.

### Wall time

~30 min single worker: recon/PNG analysis ~15 min; joiner + test edits ~5 min; `make gen` +
verification runs ~5 min; report ~5 min.

## ogh.7 — Hedge spcdef trial

Measurement only: **no shipped gameplay constant or authored data file changed** (off the
AVR/generation lane; no `make gen`/AVR/gates; no git commit/push). Bead left `in_progress`.
The candidate seed change was applied **in memory** on the loaded `mock/data.js` record, never in
`data/json/creatures.json`.

### Retired-frame reconciliation (PP / Rejuvenate)

The bead text was authored for the retired Pokemon/SMT engine. **"Keep PP and Rejuvenate
unchanged" is vacuous in the current five-type RPS duel:** neither **PP** (move-uses) nor
**Rejuvenate** (heal) exists in the duel. The shipping duel (`mock/battle-mock.js` duel path,
`src/engine/battle/DuelKernel.hpp`) resolves each round with flat constants
(`FLAT_WIN/FLAT_CHIP/FLAT_TRADE`) plus the type-relation lookup — there is no move-use counter and
no healing. The only authored lever that reaches the duel is the creature record itself, so the
live interpretation of "Hedge spcdef seed 3->5" is the authored `spcdef` stat seed that derives the
mock's `stats.spcDef`. That is the quantity measured below.

### What actually consumes `spcdef`, and what does not

- Authored `data/json/creatures.json` → `spcdef` → generator
  `tools/battle-design-spike-data.mjs` `stat(SPIKE_LEVEL, creature.spcdef)` → `mock/data.js`
  `creatures[].stats.spcDef`. Hedge authored `spcdef=3` → `spcDef = 2*8 + 3*2 = 22`.
- **The five-type duel never reads `spcDef`** (nor `attack`/`defense`/`spcAtk`/`speed`). The only
  creature fields the duel path touches are `maxHp` (derived from authored `hp`, not `spcdef`),
  `types`/`merged`, `moves`, and the frozen 5-bit `attackMask`. `mock/battle-mock.js` reads
  `.stats.attack` **only inside the `stances` variant** (`duelKind === 'stances'`, lines 451-497),
  which the pair sweep does not use. `DuelKernel.hpp` has no stat references at all.
- Therefore changing Hedge `spcdef` **cannot** move a duel outcome; it only changes the
  `fixtures.creatures` provenance hash. This was verified empirically, not assumed.

### Measured seed 3 -> 5 delta (matrix5 team3 bonus cap200 reader, 25 pairs x 7 policies x 5 rates x 1000)

In-memory `hedge.stats.spcDef` 3→5, everything else identical. `A` = control, `B` = seed 5:

| metric | A (spcdef 3) | B (spcdef 5) |
| --- | ---: | ---: |
| W / L / D / T | 285,614 / 561,241 / 28,145 / 0 | 285,614 / 561,241 / 28,145 / 0 |
| W% / L% / D% / T% | 32.6 / 64.1 / 3.2 / 0.0 | 32.6 / 64.1 / 3.2 / 0.0 |
| rounds total / avg | 14,965,941 / 17.104 | 14,965,941 / 17.104 |
| cell medianRounds min/med/max | 11 / 15 / 43 | 11 / 15 / 43 |
| cell p95Rounds min/med/max | 11 / 18 / 43 | 11 / 18 / 43 |
| switches (switchRate) | 10,000 (0.0007) | 10,000 (0.0007) |
| KOs / oneHitKOs | 3,343,204 / 0 | 3,343,204 / 0 |
| type skew (n) | water 42.1 / earth 24.6 / spirit 23.8 / fire 6.4 / plant 3.2 (14,955,941) | identical |
| species skew | hedge 7.8% (13 species) | identical |
| decided pairs (0%/100%) | 11/25 (7 zero-win, 4 100%-win) | 11/25 |
| **pairs changed** | — | **0/25** |
| **cells changed** | — | **0** |

The only byte-level difference between the two artifacts is
`provenance.fixtures.creatures` (f98c47f9… → 65e600d1…). **Every** outcome, pacing, skew and
per-pair/per-cell metric is identical: the seed 3→5 change produces a **+0.0 pt** swing on the
paired 3v3 baseline. `balanced` is the only authored team containing hedge
(`squid,SkitterCrab,hedge`); all pairs using `balanced` as either side are unchanged too, as are all
other pairs.

### Sign-off needed (no constant changed)

- **Recommendation: REJECT this trial as a balance lever.** Hedge `spcdef` does not enter the duel
  decision at all; seed 3→5 changes nothing measurable. Making the seed matter would require the
  duel to consult `spcDef` (a scoring/damage rule change), which is out of this bead's stated
  one-number scope.
- If the owner still wants Hedge to be "more special-defensive," that is a **different** bead:
  either wire `spcDef` into duel damage (new mechanic — needs design + AVR budget), or express the
  intent through a field the duel already reads (types/kits/maxHp via authored `hp`). Any such
  change to `data/json/creatures.json` requires **owner sign-off** before shipping; this bead leaves
  the authored file untouched.

### Verify (commands + tails)

```sh
# Determinism: the shipped CLI, two runs, byte-identical.
node tools/battle-duel-spike.mjs --json --pair-sweep --battles 1000 --rounds 200 > build/reports/ogh.7/cli-baseline.json
node tools/battle-duel-spike.mjs --json --pair-sweep --battles 1000 --rounds 200 > build/reports/ogh.7/cli-baseline-replay.json
cmp build/reports/ogh.7/cli-baseline.json build/reports/ogh.7/cli-baseline-replay.json   # BYTE-IDENTICAL

# In-memory trial harness (mutates mock/data.js only) reproduces the CLI baseline byte-for-byte.
node build/reports/ogh.7/trial.mjs                                  --battles 1000 --rounds 200 > build/reports/ogh.7/baseline.json
node build/reports/ogh.7/trial.mjs                                  --battles 1000 --rounds 200 > build/reports/ogh.7/baseline-replay.json
node build/reports/ogh.7/trial.mjs --hedge-spcdef 5 --battles 1000 --rounds 200 > build/reports/ogh.7/hedge-spcdef5.json
cmp build/reports/ogh.7/baseline.json build/reports/ogh.7/baseline-replay.json  # BYTE-IDENTICAL
cmp build/reports/ogh.7/cli-baseline.json build/reports/ogh.7/baseline.json     # BYTE-IDENTICAL
node build/reports/ogh.7/compare.mjs build/reports/ogh.7/baseline.json build/reports/ogh.7/hedge-spcdef5.json
  # provenance fixtures.creatures f98c47f9… -> 65e600d1…; pairs changed 0/25; cells changed 0
  # A W 32.6% L 64.1% D 3.2% T 0.0% ; B W 32.6% L 64.1% D 3.2% T 0.0%
  # decided pairs (0%/100%): A 11/25  B 11/25

node --test tools/tests/battle-duel-spike_test.mjs tools/tests/duel-kernel-parity_test.mjs
# tests 26  pass 26  fail 0
make sim-test BUILD_DIR=build CPPFLAGS='-I. -DCGFX_DUEL_BATTLE'
# tests 6  pass 6  fail 0

git diff --stat -- data/json/creatures.json   # empty: authored file untouched
```

sha256: `baseline.json` = `baseline-replay.json` = `cli-baseline.json` = `cli-baseline-replay.json`
= `3414a37a0b5b4a7514689bca90673fef66956eb45fb626e85d0c5ec07ab90b3c`;
`hedge-spcdef5.json` = `2c028ebdfad3948ad4f2dbbb39d418e523ad22a5666228e2a5e3bbd4e3485489`.

### Deviations (bead verify list superseded by the dispatch scope)

The bead's own VERIFY block assumes a landed data change (`make gen` + `make test` +
`make verify-generated` + `make test-pack-parity` + `make fxtest-headless` + `make ram`). The
dispatch put this worker **off the AVR/generation lane** and forbade `make gen`/AVR/gates and any
shipped-constant change, so those steps do not apply and were not run: the trial is measurement +
proposal only, and no generated artifact, packed image or shipping constant changed. The mandated
gate for this scope is the Node duel suites + `make sim-test` above. No `output.md` gate run.

### Storage

`df -h .` 62 GiB free before/after (>=10 GiB reserve held). `du -sh build` 1.9G before and after.
Artifacts confined to gitignored `build/reports/ogh.7/` (4.0 MB: 4 baseline/after JSON, compare +
trial scratch `.mjs`, `delta.txt`). No build tree created; `sim-test` reused `build/duel-sim`.
Pre-existing tracked edits (`.beads/interactions.jsonl`, plus the generation-lane worker's files)
were not touched.

### Wall time

Worker ~30 min wall: read bead/output/docs/tool/generator ~12 min; harness (~4 min) + baseline x2 +
after + CLI x2 (~4 min incl. ~26 s/sweep x 5); structural compare + metrics ~4 min; node+sim tests
~1.5 min; report remainder. Off the build/generation lane; no gate/orchestrator run (dispatch leaves
the bead `in_progress`).

## jp8.11.3 — reward mappings

Implemented the first-area material/plant/heal reward mapping end to end: authored
`[[reward.heal]]` in `data/beta-area.toml`, native generator validation + packed append-only
`Rewards` FX descriptor, a device fixture, and host/packed-device/source-to-header agreement
tests. Single build lane (`BUILD_DIR=build`); no git commit/push; bead left `in_progress`.

### Design decision (recorded deviation)

`data/beta-area.toml` did not previously name the reward sources explicitly, so the mapping is
derived from the frozen contract the only way that yields the design's pinned `supported types
0..3` for every first-area species:

- ordinary-wild material: for every `encounters.allowed_species`, one **tier-2 lure** whose type
  is the species' authored `type1` folded into the four lure-zone groups (`type_index & 3`, the
  same low-two-bit reduction `LureZone::lureType` applies to a deposited lure). Item id =
  `lure_type*3 + 2` (the shared `item::lureId` layout). Non-first-area species pack `materialItem
  = 0xFF` / `materialCount = 0` sentinels.
- plant: `plant_source` (`lure_type 0`, `lure_tier 1`, `item_id 1`, `count 1`) is the tier-1
  field source.
- heals: two authored `[[reward.heal]]` entries (`item_id 0`,`1`, `count 1`) validated against
  `data/consumables.csv` (must be `Heal` kind).

Host validation rejects: duplicate source identities, missing species, unknown primary type, a
species with no usable attack type or an unknown move type, out-of-range lure type/tier/count,
a plant item id inconsistent with its type+tier, non-`Heal`/out-of-range/duplicate heal ids.

### Generated mapping (data/beta-area.toml → fxdata/generated/rewards.txt)

```
namespace Rewards {
materialItem  = { 8,255,255,11,255,255,5,255,255,2,2,255,8,8,11,255,... }  (64, species-indexed)
materialCount = { 1,0,0,1,0,0,1,0,0,1,1,0,1,1,1,0,... }                    (64)
plantItem = { 1 }; plantCount = { 1 };
healItem  = { 0, 1 }; healCount = { 1, 1 };
}
```
Species→tier-2 material id: 0→8(wind), 3→11(earth), 6→5(water), 9→2(spirit), 10→2, 12→8,
13→8, 14→11. Source-agnostic proof: `node --test tools/tests/beta-area_test.mjs` recomputes each
entry from `creatures.json` `type1` + `allowed_species` and matches the generated file + the
published `src/fxdata.h` `Rewards::*` symbols.

### Commands and results

```
cargo test --offline -p cgfx-core        230 passed / 0 failed  (+6 rewards tests; was 224)
built+installed cgfx-tools identity 0.3.0+ge8afd0a7, binary sha256 c80df77b68e4f47f...
make gen BUILD_DIR=build                rc=0 (~7 s; no build/cgfx-pack.* stage left)
make verify-generated BUILD_DIR=build   rc=0
make test-generated-libs BUILD_DIR=build  generated-libs 48/0 PASS; invariants 5/0 PASS; alias 28/0
make test-pack-parity BUILD_DIR=build   PASS (rebaselined, see below)
make test BUILD_DIR=build               258 passed / 0 failed
make testvm BUILD_DIR=build             286 passed / 0 failed
make test-beta-area BUILD_DIR=build     13 pass / 0 fail
make test-manifest BUILD_DIR=build      ensure-generated PASS
make fxtest-headless BUILD_DIR=build ARDENS=$ARDENS FXTEST_INOS='test_world_player test_items test_stack'
    test_world_player: PASS 76/0
    test_items: PASS 273/0  (static RAM 1266/2160)
    test_stack: PASS 4/0    (stack headroom 598)
make ram BUILD_DIR=build                22768 B flash / 1678 B static  (DELTA 0/0 vs 22768/1678)
```

### Pack parity rebaseline (old → new)

`fcda6f8b59f1301d68724a0845d1123da6763e774667494b58eb1e21d63cf5bb` →
`418ac63b081f69db5c1702d572c70aaee72783a780c2fe68e82ed37d778df26a`.

Two intentional causes, both isolation-checked by `git diff src/fxdata.h`:
1. append-only `Rewards` expand entry: `FX_DATA_BYTES` 748732 → 748866 (+134 = 64+64+1+1+2+2);
   every prior field address is byte-identical (only `FX_DATA_PAGE`/`FX_DATA_BYTES` and the
   appended `Rewards` namespace lines change). Last prior field `Evolution::evolutionLevel =
   0x0B6CB9` unchanged; `Rewards::materialItem = 0x0B6CBC`.
2. tool identity `0.3.0+ga5ffdd1d` → `0.3.0+ge8afd0a7` (sibling HEAD is now `e8afd0a7`): same
   27-byte string length, so only the `generator_version` payload bytes change (no address shift).
   The committed generated set had been stamped with the parent `a5ffdd1d`; the rebuilt tool
   records its true HEAD.

Provenance comment block added to `tools/tests/pack-parity_test.sh`.

### Storage handoff

`df -h . /private/tmp`: 62 GiB free (≥10 GiB). `du -sh build` before/after: 1.9 GiB (≤5 GiB).
`make clean-build-artifacts BUILD_DIR=build` previewed (108 candidates, 2,604,439 B) then applied
`CLEAN_APPLY=1` — only generated `build/arduino-build/fx/**` objects. No `build/cgfx-pack.*` stage
remained. Other lanes (`box-adoption-gate`, `battle-carve-spike`, `native-tools`) untouched.

### Deviations

- Reward source shape: the contract did not pin per-species material types, so the tier-2 type is
  derived (`type1 & 3`) rather than authored, matching the design's `supported types 0..3`; the
  heal reward list is the only new authored reward data. Recorded above.
- `tst/item_test.hpp` was not modified: host-side boundary coverage is the sibling
  `crates/core/src/rewards.rs` unit suite plus the node source↔header agreement test, since the
  host item suite has no path to the packed FX image (no runtime reader exists until jp8.11.4).
- `data/json/creatures.json` needed no change (materials derive from authored `type1`).

### Wall time

~55 min single worker: recon/design ~20 min; generator + wiring ~15 min; tests ~10 min; gen +
gates + device suite ~7 min; report/storage ~3 min.

## ogh.8 — Cloud Sweep reconciliation

Measurement only: **no shipped gameplay constant or authored data file changed** (off the
AVR/generation lane; no `make gen`/AVR/gates; no git commit/push). Bead left `in_progress`.
The candidate is applied **in memory** on the loaded `mock/data.js` cloud record, never in
`data/json/creatures.json`.

### Retired-frame reconciliation (Cloud Sweep / survival-order gate / KO risk)

The bead text was authored for the retired Pokemon/SMT engine (per-move power, ordered turns,
SPDDWN status, an AI survival/order gate, projected KO risk). **None of those concepts exist in
the five-type RPS duel.**

- `src/engine/battle/Ai.cpp` (the only SPDDWN/survival/order code) is the **legacy** AI, guarded
  by `#ifndef CGFX_DUEL_BATTLE` and not compiled in the duel build. Its retired Sweep predicate is:
  status moves run only when a damage move deals >0 and `self.hp > maxHp/2` (survival), and the
  SPDDWN branch fires only when `enemy staged speed >= self staged speed` (order). Tie handling,
  status-stage limits and RNG are all stat/effect mechanics the duel does not have.
- The duel (`mock/battle-mock.js` wheel path, `src/engine/battle/DuelKernel.hpp`) has **no moves,
  no power, no PP, no status, no speed, no stat reads**. Damage is the flat type web scaled by the
  creature-type matchup: `relation(moveType,moveType)` (WIN 14 / CHIP 4 / TRADE 6) times
  `creatureModifier` (x1.5/x1.0/x0.5). The AI's *only* decision is `aiDuelPick`: no-read prefers
  the best `duelCreatureBonus` tier; read counters the guessed type or switches to a bench counter.
  **Neither branch reads HP, damage, or a move-use count** — there is no survival gate, no order
  gate, and no per-move selection at all.
- "Cloud Sweep" folds to **the cloud creature's kit**: authored `cloud` (id 13, wind) has moves
  `["water","fire"]` -> duel `attackMask` 10, maxHp 62; the `support` sample team is the only team
  containing it (retyped `cloud`->water). The retired `sweep` move is authored type **`status`**,
  which is not in the 5-type matrix, so `foldedKit` **filters it out of the duel kit entirely**.
  The move is not "gated"; it is absent.

### Measured effect (matrix5 team3 bonus cap200 reader, 25 pairs x 7 policies x 5 rates x 1000)

Control/candidate are the tool's own versioned `--json` pair-sweep artifact over the real
`tools/battle-duel-spike.mjs` seam. `A` = tier-aware control (reproduces ogh.18: W32.6/L64.1/D3.2/T0.0).

| candidate (in-memory only) | W% | L% | D% | pairs changed | cells changed |
| --- | ---: | ---: | ---: | ---: | ---: |
| A control (tier-aware baseline) | 32.6 | 64.1 | 3.2 | — | — |
| **B1 "Cloud Sweep" (`cloud.moves` += `status`)** | 32.6 | 64.1 | 3.2 | **0/25** | **0/875** |
| B2 sensitivity: `cloud.moves` = `["water"]` (drop kit slot) | 36.5 | 59.8 | 3.7 | 6/25 | 220/875 |

- **B1 is exactly inert.** Only `provenance.fixtures.creatures` changes
  (`f98c47f9…` -> `3aec3c54…`); every outcome, pacing, skew and per-pair/per-cell metric is
  byte-identical (`diff` of the two artifacts is identical modulo the 64-hex hashes). This is the
  precise proof: the retired Sweep's `status` type folds out of the duel kit, so re-adding it
  cannot move a duel. **Every** support-team pair is unchanged (`support vs balanced` 19.9%,
  `balanced vs support` 73.6%, `support vs trap` 52.1%, `retyped vs support` 7.6%, etc.).
- **B2 confirms the duel *does* read cloud's folded kit** (6/25 pairs move; e.g.
  `support vs trap` 52.1% -> 100%), so the inertness is specific to the move/status/gate concept,
  not to cloud wholesale. But B2 is a *kit* change, not "Cloud Sweep": it is not this bead's
  candidate and is reported only to isolate the null.

### Projected KO risk (support team, shipped flat constants)

No AI branch gates on this; it is the only KO notion the duel has (`build/reports/ogh.8/ko-risk.txt`):

| tier | WIN | CHIP | TRADE |
| --- | ---: | ---: | ---: |
| x1.5 | 21 | 6 | 9 |
| x1.0 | 14 | 4 | 6 |
| x0.5 | 7 | 2 | 3 |

cloud (maxHp 62), kit water/fire: best hits-to-KO vs bell 4, vs CoalGrub 3, vs cloud 5. Slowest
9 hits. `oneHitKOs 0` (max hit 21 << 52 min maxHp). The clock is flat and AI-agnostic; "projected
KO risk" in the retired sense (survival/order gating on it) does not exist.

### Sign-off recommendation

- **Recommendation: REJECT this trial as a balance lever.** "Cloud Sweep" is undefined in the
  five-type duel: the move is a status type that folds out, and the survival/order gate and
  per-move KO-risk policy live only in the unbuilt legacy `Ai.cpp`. The lever is **inert** (0/25
  pairs, 0/875 cells), so there is nothing to keep or tune and **no shipped-constant sign-off is
  requested** (no authored value changed).
- If the owner wants cloud/cloud-adjacent behavior to matter, it is a **different** bead: either a
  kit/type change (a shipped data edit — needs owner sign-off and AVR measurement), or a new duel
  AI mechanic (design + budget), expressed through fields the duel already reads
  (`moves`/kits, `types`, `maxHp`). This bead leaves `data/json/creatures.json` untouched.

### Incidental finding (out of scope, not fixed)

`mock/battle-mock.js` `aiDuelPick` (bonus no-read branch) dereferences `pool[rng.int(pool.length)]`
with no zero-option guard; a creature whose kit folds to **no** usable attack type crashes the
mock (`options.length === 0` -> `pool=[]` -> `undefined.index`). `DuelKernel.hpp` handles the same
state as WAIT. A parity gap worth a follow-up bead, but unrelated to this trial and not edited.

### Verify (commands + tails)

```sh
node build/reports/ogh.8/trial.mjs --battles 1000 --rounds 200 > build/reports/ogh.8/control.json          # exit 0, 22.6 s
node build/reports/ogh.8/trial.mjs --battles 1000 --rounds 200 > build/reports/ogh.8/control-replay.json   # exit 0, 24.4 s
cmp build/reports/ogh.8/control.json build/reports/ogh.8/control-replay.json                               # BYTE-IDENTICAL
sha256 control.json = control-replay.json = 0e7af696077da99069edf6b845c70021f630c2905ab5f0bd4683be7f8992d837
node build/reports/ogh.8/trial.mjs --battles 1000 --rounds 200 --variant status > build/reports/ogh.8/status.json
node build/reports/ogh.8/compare.mjs control.json status.json
  # pairs changed 0/25 ; cells changed 0/875 ; only provenance.fixtures.creatures changed
node build/reports/ogh.8/trial.mjs --battles 1000 --rounds 200 --variant drop-fire > build/reports/ogh.8/drop-fire.json
node build/reports/ogh.8/compare.mjs control.json drop-fire.json                                           # pairs changed 6/25 ; cells 220/875
node build/reports/ogh.8/ko-risk.mjs                                                                       # KO-risk tiers + support hits-to-KO

node --test tools/tests/battle-duel-spike_test.mjs tools/tests/duel-kernel-parity_test.mjs   # tests 26  pass 26  fail 0
make sim-test BUILD_DIR=build CPPFLAGS='-I. -DCGFX_DUEL_BATTLE'                              # tests 6  pass 6  fail 0
git diff --stat -- data/json/creatures.json data/json/moves.json                             # empty: authored files untouched
```

### Deviations (bead verify list superseded by dispatch scope)

The bead's own VERIFY block assumes a landed data change (`make gen` + `make test` +
`make verify-generated` + `make test-pack-parity` + `make fxtest-headless` + `make ram`). The
dispatch put this worker **off the AVR/generation lane** and forbade `make gen`/AVR/gates and any
shipped-constant change, so those steps do not apply and were not run: the trial is measurement +
proposal only. The mandated gate for this scope is the Node duel suites + `make sim-test` above.
A `--variant no-attack` probe (`cloud.moves=[]`) was dropped because it triggers the incidental
mock crash above; it is not the candidate.

### Storage

`df -h .` 62 GiB free before/after (>=10 GiB reserve held). `du -sh build` 1.9G before and after.
New artifacts confined to gitignored `build/reports/ogh.8/` (5 JSON + 3 scratch `.mjs` + txt,
~3.4 MB). No build tree created; `sim-test` reused `build/duel-sim`. Pre-existing tracked edits by
the generation-lane worker (`.beads/interactions.jsonl`, `src/engine/battle/BattleFlow.cpp`,
`tst/duel_session_integration_test.hpp`) were not touched.

### Wall time

Worker ~25 min wall: read bead/output/docs/Ai.cpp/mock/kernel + reconcile ~10 min; harness
(~4 min); control x2 + status + drop-fire (~1.5 min incl. 4x ~24 s sweeps); compare + KO-risk
~3 min; node+sim tests ~2 min; report/storage remainder. No gate/orchestrator run (dispatch leaves
the bead `in_progress`).

## jp8.11.4 — KO material award

### What changed

- `src/engine/battle/BattleFlow.cpp`: new `awardWildKoMaterial(state)` called from
  `finishBattle` on `Outcome::Win` only. Reads the generated `Rewards::materialItem`/
  `materialCount` mapping (one byte each) at the single terminal edge before `exitBattle`,
  skips `0xFF` item sentinel / zero count / `state.trainer` / `state.gatherable`, and
  `inventoryAdd`s the authored tier-2 lure into `item::battleInventory()` (the one Player
  bag). No per-frame/per-step reads; no EventSink or reward queue.
- `tst/duel_session_integration_test.hpp` (host): six focused tests pinning award-once,
  0xFF sentinel, gatherable-KO exclusion, trainer-flag exclusion, non-Win (escape) exclusion
  and full-bag saturation. The item/count fixture bytes are installed at the published
  `Rewards` addresses with the count offset derived from the addresses (no hardcoded layout).
- `tst/fxdatatest/duel_session_integration_test.hpp` (device): one `duelDeviceWildKoMaterialAward`
  driving the real packed mapping for species 0 through the real flow to a wild Win terminal,
  plus the sentinel-4 and gatherable negatives and a full-bag saturate case. Expected item/count
  are read from `RewardFixture` (already proven equal to the packed bytes by `tst/fxdatatest/items_test.hpp`).

### Commands, results, tails

```
make test BUILD_DIR=build
  -> "Total Passed: 240363  Total Failed: 0" (host) + world "Total Passed: 258  Total Failed: 0"
     (host delta +15 assertions from the six new cases)
make testvm BUILD_DIR=build
  -> "Total Passed: 286  Total Failed: 0"
make test-beta-area BUILD_DIR=build
  -> tests 13 / pass 13 / fail 0
make fxtest-headless BUILD_DIR=build ARDENS="$ARDENS" FXTEST_MS=30000 \
  FXTEST_INOS='tst/fxdatatest/test_battle_duel.ino tst/fxdatatest/test_world_player.ino tst/fxdatatest/test_items.ino tst/fxdatatest/test_stack.ino'
  -> test_battle_duel PASSED=125 FAILED=0   (was 115; +10 new assertions)
     test_world_player PASSED=76 FAILED=0
     test_items PASSED=273 FAILED=0
     test_stack PASSED=4 FAILED=0   (stack headroom=598, mode transition headroom=695)
     AVR footprint: test_battle_duel sketch 29250/29696 B, statics 1794 B.
make ram BUILD_DIR=build
  -> RAM_FLASH_BYTES=22980 (limit 29696) ; RAM_STATIC_BYTES=1680 (limit 2560) ; RAM_FREE_BYTES=880
```

### Exactly-once / sentinel / exclusion proofs

- **Once, no re-award:** host `duelWildKoAwardsMaterialOnce` and device
  `duelDeviceWildKoMaterialAward` award the authored material on an ordinary wild KO and then
  run 8 further world frames with the stack still at the awarded count (the award lives only in
  the single `finishBattle` terminal edge; `exitBattle` makes later updates return early).
- **0xFF sentinel:** host `duelWildKoSentinelAwardsNothing` and the device species-4 case assert
  `inventoryNonZeroCount == 0` — no phantom item.
- **Exclusions:** gatherable-target KO, trainer/boss flag, and a non-Win escape terminal all
  leave the bag empty (host + device).
- **Full bag:** pre-filled to the 99 cap, the terminal still completes and the stack stays 99 with
  no extra stack (`inventoryAdd` saturates).

### Flash / SRAM delta

Control (jp8.11.3 HEAD, `RAM_FLASH_BYTES=22768 / RAM_STATIC_BYTES=1678`) -> candidate
**22980 B flash (+212) / 1680 B static (+2)**. Target ≤29,184 B, hard ≤29,696 B, static ≤2,160 B:
all met with 6,716 B flash and 880 B static free. Device `test_stack` headroom (598) is well above
the reserve.

### Deviations

The bead's legacy VERIFY block named `BUILD_DIR=build/box-adoption-gate` and a non-existent
`tst/fxdatatest/test_battlesession.ino`; the dispatch pinned `BUILD_DIR=build` and the four
`test_*.ino` suites listed above, which were run. No commit/push; bead left `in_progress`.

### Storage

`df -h .` 62 GiB free before and after (>=10 GiB reserve held). `du -sh build` 1.9G -> 1.8G.
`make clean-build-artifacts BUILD_DIR=build` previewed (108 candidates, 2.61 MB) then applied;
additionally removed the positively identified disposable device staging `build/fxtest` (~118 MB).
Retained: `build/CreatureGathererFX.ino.elf` + `make ram` report. No repository snapshot copied.

### Wall time

Worker ~50 min wall: recon/design ~12 min; implementation ~5 min; host gates ~4 min;
device fxtest (4 suites, plus a `test_battle_duel` size-fit iteration) ~20 min; ram + report +
cleanup remainder. No gate/orchestrator run (dispatch leaves the bead `in_progress`).

## v8u3 — mock zero-kit guard

### Fix

Mock duel AI/pick path had no zero-option guard: `aiDuelPick` fell through to
`state.rng.int(options.length)`, and `duelResolve` dereferenced `foeMove.type`/
`yourMove.type` on an empty kit, throwing where the device kernel returns WAIT.

- `mock/battle-mock.js`
  - Added pick encoding `WAIT_PICK = -2` and `isWaitPick(pick)` (matches the
    kernel's `ActionKind::Wait`; distinct from the switch encodings `-1`/`>=100`).
  - `aiDuelPick` returns `WAIT_PICK` for a zero-option kit *before* consuming RNG,
    mirroring `duel::chooseAction`'s early `maskOptionCount()==0 -> Wait` return.
  - `duelResolve` wheel path: a side with no usable attack waits. Both wait -> line
    `WAIT`, zero damage; one waits -> the attacker lands the default win (bonus
    matchup scaled via `duelCreatureBonus`), matching `resolveRound`'s wait /
    unusable-kit branches. Empty option lists now yield `null` moves instead of
    `undefined`, so no `.type` deref throws.
  - `step` choice phase: `A` with no options and no living bench commits
    `WAIT_PICK` (kernel Skip -> Wait); still `-1` (swap) when a bench exists.
  - Exported `WAIT_PICK`, `isWaitPick`.
- `tools/battle-duel-spike.mjs` (same missing guard, fixed consistently)
  - `playerPick` returns `mock.WAIT_PICK` (was `0`, an out-of-range attack pick)
    when the player kit has no options.
  - `playDuel` switch metric now counts `-1`/`>=SWITCH_BASE` only, so a WAIT pick
    is not miscounted as a switch.

Interface: `mock/battle-mock.js` exports `WAIT_PICK = -2`, `isWaitPick(pick)`.

### New test

`mock/battle-mock.test.js` -> `a zero-option kit yields WAIT and never throws
(kernel unusable-kit parity)`: empties one authored creature's `moves` (restored in
`finally`), then asserts the AI pick is `WAIT_PICK`, `isSwitchPick` is false, an
attacker default-wins the waiting side without throwing (attacker untouched), a
mutual wait yields `WAIT` with zero damage, and a zero-kit benchless player commits
`WAIT_PICK` through `step()`.

### Commands

- `node --test mock/battle-mock.test.js` -> tests 24 / pass 24 / fail 0 (0.16s).
- `node --test mock/battle-mock.test.js tools/tests/battle-duel-spike_test.mjs
  tools/tests/duel-kernel-parity_test.mjs` -> tests 50 / pass 50 / fail 0 (1.16s).
  Parity driver (`tools/duel-parity/duel_kernel_driver.cpp`, host `c++` build) still
  matches the mock on fixed/randomized/full-duel rounds; no parity regression.
- Extra safety: `node --test tools/tests/battle-matrices-spike_test.mjs` -> 5/5 pass.

### Deviations / notes

No `make gen`/AVR/gate (node lane only). No commit/push; bead left `in_progress`.
`mock/battle-mock.test.js` uses a scoped `DATA.creatures[i].moves` override to author
the zero-kit state, restored in `finally` (authored folding never yields an empty
kit, so this is the only reachable construction).

### Wall time

Worker ~12 min wall: recon (bead/kernel/mock/parity) ~8 min; implementation ~3 min;
verification ~1 min.

## jp8.11.10 — XP award

### What changed

- `src/engine/battle/BattleFlow.cpp`: new `awardOrdinaryWildExp(state)` called from `finishBattle`
  on `Outcome::Win` (with `awardWildKoMaterial`, now sharing one `!trainer && !gatherable` guard).
  Per living occupied party instance (HP != 0, stable slot present): read the stable `StoreRecord`
  once, add exactly `8 * opponentLevel`, saturate at `levelExpThreshold(31)`, derive the level via
  `Creature::load(record)`, and `storeWrite` before installing the live creature. A level increase
  preserves missing HP: `liveHP += (newMax - oldMax)` capped at `newMax`; KO stays 0 with no reward.
  A refused `storeWrite` leaves accepted state untouched (no invented XP). No v3 layout change.
- `tst/duel_session_integration_test.hpp` (host): seven focused tests — exact amount+level-stable,
  below/at/over-threshold with full + damaged HP growth, level-31 cap, once-only, non-win
  (escape/gatherable/trainer) exclusion, KO-member exclusion, and exact-exp survival across a v3
  checkpoint+reboot.
- `tst/fxdatatest/test_win_exp.ino` + `tst/fxdatatest/win_exp_test.hpp` (new device suite): the
  same exact-amount/once/boundary/HP-growth/cap/KO proof against the real FX save flash. Added
  `test_win_exp` to the Makefile's duel-flag list. Device XP proof moved out of the near-limit
  `test_battle_duel` sketch (which keeps 14 B free); the dedicated suite uses 16660/29696 B.
- `tst/fxdatatest/duel_session_integration_test.hpp`: +1 line (`ReadData.hpp` include).

### Commands, results, tails

```
make test BUILD_DIR=build            -> host 240407/0 (+44) + world 258/0
make testvm BUILD_DIR=build          -> 286/0
make test-beta-area BUILD_DIR=build  -> tests 13 / pass 13 / fail 0
make fxtest-headless BUILD_DIR=build ARDENS="$ARDENS" FXTEST_MS=30000 FXTEST_INOS='...test_battle_duel.ino ...test_save.ino ...test_world_player.ino ...test_stack.ino ...test_win_exp.ino'
  test_battle_duel PASSED=125 FAILED=0    (sketch 29682/29696, 14 B free)
  test_save        PASSED=3283 FAILED=0   (sketch 29674/29696; unchanged baseline)
  test_world_player PASSED=76 FAILED=0    (sketch 29684/29696, 12 B free)
  test_stack       PASSED=4 FAILED=0      (stack headroom 598, transition headroom 695)
  test_win_exp     PASSED=12 FAILED=0     (sketch 16660/29696)
make ram BUILD_DIR=build             -> RAM_FLASH_BYTES=23198 ; RAM_STATIC_BYTES=1680 ; RAM_FREE_BYTES=880
```

### Exact-XP / bounded-level proofs (host, mirrored on device)

- Exact once: exp 27 (`threshold(3)`) +32 -> 59 stored exactly, level unchanged (no rounding).
- Boundaries (gain 32): 31 -> 63 (no level up); 32 -> 64 = `threshold(4)` -> level 4; 100 -> 132 ->
  level 5 (remainder kept).
- HP: a full member that levels reaches its new derived maximum; a member 5 below max grows by
  `newMax-oldMax` and stays 5 below. KO (HP 0) -> exp unchanged, HP stays 0.
- Cap: `threshold(31)-1` +248 -> exactly 29791 at level 31.
- Once-only: 16 further world frames never re-award. Non-win (escape / gatherable KO / trainer)
  awards nothing. Save+reboot preserves the exact accepted exp.

### Flash / SRAM delta

Control HEAD `d4173e3` 22980 flash / 1680 static -> candidate **23198 flash (+218) / 1680 static (+0)**.
Shipping targets (<=29184 plan / <=29696 hard, static <=2160): met with 5,986 B flash and 880 B
static free; `test_stack` headroom 598 B (>=150 B reserve).

### Deviations

- Device XP proof lives in a new dedicated suite (`test_win_exp`) plus a Makefile duel-flag entry,
  not in `test_battle_duel`: that sketch was at 446 B free at baseline and the production change
  costs +432 there, so a case relocation would have been required. This follows the new
  session-efficiency rule ("trim or relocate ... rather than oscillating at the limit"). The extra
  suite was added to the dispatch's FXTEST_INOS and run.
- **Near-limit warning (per new rule)**: `test_battle_duel` (14 B free) and `test_world_player`
  (12 B free) now sit at the hard floor; `test_save` was already at 22 B free. Pre-existing
  `test_battlepresentation` overflowed at 29744 B **before** this change (identical size with it)
  and is out of scope — the wildcard final gate needs a suite trimming/relocation bead.
- Bead's legacy VERIFY named `build/box-adoption-gate`; the dispatch pinned `BUILD_DIR=build`, used.
- No commit/push; bead left `in_progress` per dispatch.

### Storage

`df -h .` 63 GiB free before/after (>=10 GiB reserve held). `du -sh build` 1.4G -> 1.2G.
`make clean-build-artifacts BUILD_DIR=build` previewed (108 candidates, 2.6 MB); retained the
active `build/native-tools` (963M) and `build/arduino-build` compiler cache. Removed the disposable
`build/fxtest` staging (180M). No `cgfx-pack.*` stage remains. Retained `build/CreatureGathererFX.ino.elf`.

### Wall time

Worker ~95 min wall: recon/design ~25 min; host implementation+tests ~10 min; device size
investigation (LTO is non-linear — four production variants measured across three sketches) ~40 min;
device runs + host gates + storage ~20 min. No orchestrator gate run (bead left `in_progress`).

## 5j1n — device sketch headroom

Restored the wildcard device-gate flash margin. Test CASES only were relocated to co-located
sketches; **no `src/` change and no reduction of jp8.11.10 behavior**. Every `tst/fxdatatest/*.ino`
now builds with >=1 KB free (min 3,012 B).

### Before / after free flash (29696 hard limit)

| suite | before B (free) | after B (free) | where the cases moved |
|---|---|---|---|
| test_battlepresentation | 29744 (OVERFLOW, -48) | 16530 (+13166) | kind/effectiveness/HP-track/move cases -> new `test_battlepresentation_moves` |
| test_battlepresentation_moves | — | 25100 (+4596) | (new) |
| test_battle_duel | 29682 (+14) | 8540 (+21156) | session flow -> new `test_duel_session` |
| test_duel_session | — | 26684 (+3012) | (new) |
| test_world_player | 29684 (+12) | 24292 (+5404) | bag v3 snapshot/deposit -> new `test_world_bag` |
| test_world_bag | — | 12214 (+17482) | (new) |
| test_save | 29674 (+22) | 23978 (+5718) | interrupted-write cut cases -> new `test_save_cuts` |
| test_save_cuts | — | 17708 (+11988) | (new) |

All other suites unchanged; full run shows the smallest margin at `test_duel_session` 3012 B.

### What moved where

- `test_battle_duel.ino`: kernel only (`battle_duel_device_test`). New `test_duel_session.ino` runs
  `test_duel_session` (the header already documented this wiring). Expanded the Makefile duel-profile
  `elif` so `test_duel_session` compiles with `-DCGFX_BATTLE_NO_FACTS -DCGFX_BATTLE_COMPACT_RESULT
  -DCGFX_DUEL_BATTLE` (the header's `#error` guard proves the flags reached every TU).
- `battlepresentation_test.hpp`: split `test_battlepresentation` into a core entry (playback x2 +
  painted-stack/frame-budget/caption-chain proofs, dqxm) and `test_battlepresentation_moves`
  (switchedSprites, damageLines, effectivenessFeedback x5, feedbackCategories, moveLabels,
  attackMoveRasters, hpTrackEdges, both menu blocks). No assertions changed.
- `test_world_player.ino`: the v3 bag snapshot/deposit block moved verbatim to new
  `test_world_bag.ino`; world/encounter/battle-resume cases stay.
- `save_test.hpp`: `test_save_cuts` holds the three interrupted-write cases
  (`BoxedHPPublicationCuts`, `ControllerCaptureHPCuts`, `CheckpointCuts`); durable-state cases stay.
  The Makefile `fxtest-run` gives `test_save_cuts` the same extended `FXTEST_MS_SAVE` window as
  `test_save`.

### Coverage preserved (device assertion counts)

duel 47+78=125; presentation 102+222=324; world 65+11=76; save 304+3001=3305 — each split sum equals
the pre-split total (same functions, only regrouped). All PASS, 0 FAILED.

### Commands

- `make fxtest-build BUILD_DIR=build` — all 33 wildcard suites; every ELF <=29696 with >=1 KB free
  (log: `build/fxtest-build-all.log`).
- `make fxtest-headless BUILD_DIR=build ARDENS=.../Ardens FXTEST_MS=30000 FXTEST_INOS='<mandated 5>'`
  — test_battlepresentation/duel/world_player/win_exp/stack all PASS.
- Same target with the new suites (`test_duel_session test_world_bag test_battlepresentation_moves
  test_save_cuts test_save`) — all PASS.
- `make test BUILD_DIR=build` — host suites pass. `make ram BUILD_DIR=build` — shipping 23198/29696
  flash, 1680/2560 static (fresh rebuild).

### Storage

`df -h .` 63 GiB free (>=10 GiB). `du -sh build` 1.4G (<=5 GiB). `make clean-build-artifacts
BUILD_DIR=build` previewed (108 candidates, 2.6 MB, shipping arduino-build cache) then applied.
Retained `build/native-tools` (963M) and the `build/fxtest` device stages (212M, within budget).
Log kept at `build/fxtest-build-all.log`.

### Wall time

Worker ~70 min wall: recon ~25; implementation ~10; wildcard build 5:10 + targeted builds ~6; two
device runs ~1:45; host+ram+storage ~3; report ~5. No commit/push; bead left `in_progress`.

## jp8.11.12 — apply beta evolution

### What changed

- `src/creature/Evolution.hpp` (new): `CreatureEvolution::targetFor(species, level)` resolves the
  packed first-beta edge mapping (0->1, 3->4, 6->7 @ level 8). The three parallel arrays are emitted
  contiguously in `fxlayout` declaration order, so `kEdgeCount = evolutionTarget -
  evolutionSource` (= 3, `static_assert`ed); the source column is read once, then the matched row's
  threshold/target. No id adjacency is inferred — a species matches only an authored source row and
  only at/above its authored threshold; otherwise `NO_TARGET`. Transition-only single-byte reads.
- `src/engine/battle/BattleFlow.cpp`: `awardOrdinaryWildExp` now, on the same Win terminal edge and
  in the same single `StoreRecord` write, replaces a living member's species with its explicit
  target when its derived level has reached the edge. exp, stable slot and attack mask (moves) are
  preserved; the target's stats/types are recomputed at the unchanged derived level and a living
  member keeps its missing HP (a KO member is skipped before the reward path). A refused write
  leaves the old species, so the pending evolution stays derivable from exp and retryable.
- `src/save/Compaction.cpp`/`.hpp`: `storeEvolve(slot)` evolves one boxed stable instance in place
  on the collection open/assignment transition when its accepted storedXP already qualifies; exp,
  slot and attack mask are preserved and HP is re-encoded for the target at the same level (living
  missing-HP kept, KO kept KO). Refusal leaves the record untouched. No v3 offset/version change and
  the boxed HP encoding is unchanged.
- Tests: host `tst/creature_test.hpp` (edge resolution / no-adjacency), `tst/save_test.hpp`
  (`BoxedEvolutionTest`: each edge, no-edge species, duplicates, injured/KO HP, refusal/retry),
  `tst/duel_session_integration_test.hpp` (five terminal tests: evolve at edge, no-edge/no-adjacency,
  once, missing-HP+KO, save/reboot+reload); device `tst/fxdatatest/win_exp_test.hpp`
  (`duelDeviceWildWinEvolution`, `duelDeviceBoxedEvolution`).

### Commands, results, tails

```
make test BUILD_DIR=build            -> host 240506/0 (+99) + world 258/0 (baseline 240407)
make testvm BUILD_DIR=build          -> 286/0
make test-beta-area BUILD_DIR=build  -> tests 13 / pass 13 / fail 0
make fxtest-headless BUILD_DIR=build ARDENS="$ARDENS" FXTEST_MS=30000 FXTEST_INOS='test_win_exp test_save test_world_player test_stack'
  test_win_exp     PASSED=36  FAILED=0   (22176/29696, 7520 B free)
  test_save        PASSED=304 FAILED=0   (23978/29696, 5718 B free)
  test_world_player PASSED=65 FAILED=0   (24502/29696, 5194 B free)
  test_stack       PASSED=4   FAILED=0   (17268/29696, 12428 B free; stack headroom 598, transition 695)
make fxtest-headless BUILD_DIR=build ARDENS="$ARDENS"   # full wildcard
  -> all 33 device suites PASS, 0 FAILED; min free flash 2752 B (test_duel_session)
make ram BUILD_DIR=build             -> AVR_FLASH_BYTES=23380 (free 6316) ; AVR_STATIC_RAM_BYTES=1680 (free 480)
```

### Proofs

- Edge: crossing level 8 evolves all three stable instances in place (0->1, 3->4, 6->7), exp
  preserved exactly, stable slot and attack mask (moves) unchanged, derived level reaches the edge.
- No-edge: species without an authored edge (2/5/9, and neighbours 1/4) never evolve even at level 8
  or 31; a below-threshold level-up changes no species (no id adjacency).
- Once: the evolved target has no further edge, so a later win keeps the target and only grows exp.
- HP: a living member keeps its missing HP across evolution; a KO member neither evolves nor revives.
- Persistence: after evolution an explicit v3 checkpoint + `storeBoot` and `SaveController::load`
  return the evolved species and exact exp (host) and the evolved species survives a real
  checkpoint/reboot through the FX save flash (device).
- Boxed: a boxed slot whose storedXP already qualifies evolves through the real packed mapping,
  preserving exp/moves; below-edge and no-edge species stay put; duplicates evolve independently;
  a saturated journal refuses without changing the species and the evolution retries after the
  drain (retryable-on-refusal).

### Flash / SRAM delta

Baseline (jp8.11.10) 23198 flash / 1680 static -> candidate **23380 flash (+182) / 1680 static (+0)**.
Shipping targets (<=29184 plan / <=29696 hard, static <=2160): met with 5,804 B flash and 480 B
static free; `test_stack` headroom 598 B (>=150 B reserve, unchanged).

### Deviations

- No "save-needed"/"evolved!" UI feedback and no per-transformation banner: the dispatch's SCOPE and
  VERIFY require only the stable-instance rewrite and its proofs; jp8.11.10 likewise omitted
  deferred-growth feedback. The pending evolution is retryable by construction, so nothing is lost.
- Boxed collection-open wiring is delivered as the `storeEvolve(slot)` seam, not a call site: the
  collection view (jp8.11.8) and party-slot replacement (jp8.11.9) are still open, so no collection
  open/assignment transition exists yet. `storeEvolve` is proven directly on host and device and is
  what that transition will call. It is dropped by LTO from the shipping image until referenced.
- The evolve conditions include `target != record.id` (dead for generator-validated data; keeps the
  host fixture deterministic when an unrelated fake data base reads zeroed bytes).
- Device checkpoint loops call `FX::waitWhileBusy()` (the established device save pattern); without
  it the loop spins while flash is busy.
- The boxed device proof lives in `test_win_exp` (7520 B free) rather than `test_save`; both are in
  the dispatch's FXTEST_INOS and both run green.
- Bead's archived VERIFY named `build/box-adoption-gate`; the dispatch pinned `BUILD_DIR=build`, used.
- No commit/push; bead left `in_progress` per dispatch.

### Storage

`df -h .` 63 GiB free before/after (>=10 GiB reserve held). `du -sh build` 1.4G -> 1.2G.
`make clean-build-artifacts BUILD_DIR=build` previewed (108 candidates, 2.6 MB) then applied; removed
the disposable `build/fxtest` device staging (231M). Retained `build/native-tools` (963M, active
generator cache), `build/arduino-build` (49M) and the `build/evo-*.log` evidence. No `cgfx-pack.*`
stage remains; no generated/tracked files changed.

### Wall time

Worker ~95 min wall: recon/design ~25; implementation ~15; host iteration ~6; device focused build +
run ~15 (one earlier 13-min cold LTO build), full wildcard 7:41; ram + storage ~5; report ~10. No
orchestrator gate run (bead left `in_progress`).

## lyd8 — random demo + .arduboy

Worker spent ~55 min wall (recon ~20, impl ~8, host ~4, focused device build+run ~12, arena-package/ram ~8, storage/report ~3). Single build lane, `BUILD_DIR=build`. No commit/push; bead left `in_progress`.

### Changed

- `src/engine/arena/ArenaDemo.cpp`: `boot()` now rolls a random player team + opponent via the shared RNG (`rollRandomPair`) and calls `startMatch()` immediately; `finishBattle()` re-rolls both and restarts (continuous random 3v3). `update()` reduced to the shared `navigate` path (removed the demo-only A-confirm/random-opponent branch and the pre-existing dead `edgeDirection`/`moveCursor` helpers). `applyPlayerTeam`/`readOpponentId`/`startMatch` and the duel/shipping builds are unchanged. RNG already seeded in `setup()` from `Arduboy2Core::generateRandomSeed()`, so successive boots vary; tests pin it with `rngSeed`.
- `Makefile`: new `arena-package` target (+ `ARENA_PACKAGE`/`ARENA_PACKAGE_INFO` overrides, `.PHONY`, help). Builds the `ARENA_DEMO_CPP_FLAGS`/`ARENA_DEMO_BUILD_DIR` firmware then writes a fixed-order `zip -X` with normalised mtimes. `arena-demo`/`arena-test` untouched (dry-run confirmed they still build+launch Ardens).
- `tools/arena-demo-info.json`: schemaVersion 3, `binaries[0].filename`=`CreatureGathererFX.ino.hex`, `flashdata`=`fxdata.bin`, description/buttons updated for the random-match duel.
- `tst/arena_lifecycle_test.hpp`: rewritten for the new boot contract (host RNG stub returns 1 → rolls team 1 / opponent 1; asserts BATTLE + valid in-range pair, party rebuilt from the rolled FX members, invalid-selection no-op, terminal re-roll).
- `tst/fxdatatest/arenademo_test.hpp` + `test_arenademo.ino`: new device suite (random valid pair + variation + determinism).
- `README.md`: arena demo section updated (random boot, continuous 3v3, `arena-package`, current sizes).

### DEVICE RANDOM-PAIR PROOF

`rngSeed(0xABCD)` then computing the first two `randomRoll` draws vs `arena::boot()`:

```
rngSeed(0xABCD); draw0(0..2)=0 draw1(0..4)=4   # expected
rngSeed(0xABCD); arena::boot() -> playerTeam=0 opponentTeam=4, state=BATTLE, partyCount[0]=3 partyCount[1]=3
arena::finishBattle(Win) -> state=BATTLE, pair (1,4) != (0,4)   # consecutive rolls advance the RNG
rngSeed(0xABCD); arena::boot() -> (0,4) again                    # deterministic seam
```

`test_arenademo` output: `PASSED=15 FAILED=0` / `P`.

### VERIFY (exact commands + tails)

- `make test BUILD_DIR=build` → `Total Passed: 240501  Total Failed: 0` / `All tests passed!`; arena suite `Arena mode lifecycle … Passed: 54 Failed: 0`. World suite `Passed: 258 Failed: 0`.
- `make testvm BUILD_DIR=build` → `Total Passed: 286  Total Failed: 0` / `All tests passed!`.
- Focused device (`ARDENS=/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens`):
  `make fxtest-headless BUILD_DIR=build ARDENS="$ARDENS" FXTEST_INOS='tst/fxdatatest/test_arenademo.ino tst/fxdatatest/test_arenacatalog.ino tst/fxdatatest/test_arenaview.ino tst/fxdatatest/test_battle_duel.ino tst/fxdatatest/test_stack.ino'`
  → `test_arenademo PASS(15)` `test_arenacatalog PASS(46)` `test_arenaview PASS(70)` `test_battle_duel PASS(47)` `test_stack PASS(4)`; free flash: arenademo 9870B used (19.8 KB free), arenacatalog 9104, arenaview 7286, battle_duel 8540, stack 17268 — all far above the ~1 KB floor.
- `make arena-package BUILD_DIR=build` → arena firmware `Program: 23382 bytes` vs `ARENA_DEMO_FLASH_BUDGET=29184`; `unzip -l dist/CreatureGathererFX-ArenaDemo.arduboy`:
  ```
  696    01-01-2001 00:00   info.json
  65783  01-01-2001 00:00   CreatureGathererFX.ino.hex
  781824 01-01-2001 00:00   fxdata.bin
  ```
  Re-ran `make arena-package` → identical `shasum -a 256` = `887590b3f640415afd0993285b1a8a5c93b527c99fd32cbb982147e33e7f7f7f` (reproducible).
- `make ram BUILD_DIR=build` → `RAM_FLASH_BYTES=23380 RAM_STATIC_BYTES=1680` (shipping unchanged, matches 23380/1680).
- `make test-manifest` / `make test-generated-libs BUILD_DIR=build` → PASS (incl. arena-demo-data + first-unqualified-alias). `git status`: no generated/tracked artifact drift (`src/fxdata.h`, `fxdata/generated/*` clean).

### Arena firmware budget

`RAM_FLASH_BYTES=23382` vs `ARENA_DEMO_FLASH_BUDGET=29184` (5.8 KB headroom); static 1765/2560. Whole-image, not per-symbol.

### Deviations / notes

- Picker: boot/finishBattle never open the team picker; the picker screens remain compiled only as the `startMatch()` failure fallback (empty/invalid authored catalog — a real state per AGENTS). Added device+host assertions that boot lands in `BATTLE` (no picker). Chose this over `#ifdef`-stripping the shared screens to avoid touching the frozen seams and host navigation fixtures.
- Device proof shape: `startMatch` → `beginTrainer` → `resetCursor(Choice)` commits the opponent's hidden pick from the shared RNG (2 extra draws) and loads the duel chart with one FX read not accounted by `beginTrainer`'s `transitionExact`. Both are pre-existing and only observable in `FX_READ_COUNTER` test builds (the shipping arena demo does not define it), so the device suite pins the exact *first-two-draw* boot pair plus re-roll variation and same-seed determinism rather than a mid-stream pair or `markUpdate`. Not fixed here (out of scope; no shipped impact).
- Removed pre-existing dead helpers in `ArenaDemo.cpp` (unreferenced before this change) — part of the same file's rewrite.

### Storage

`df -h .` 60 GiB free before → 59 GiB after (>=10 GiB reserve held). `du -sh build` 1.5G → 1.3G.
`make clean-build-artifacts BUILD_DIR=build` preview (108 candidates, 2.6 MB) then `CLEAN_APPLY=1`; also applied for `build/arena-demo` (2.7 MB). Removed the disposable `build/fxtest` staged device builds (238M). Retained `build/native-tools` (963M active generator cache), `build/arduino-build` (49M), `dist/CreatureGathererFX-ArenaDemo.arduboy` (deliverable). No `cgfx-pack.*` / `arena-package.*` stage remains.

## ob83 — fxtest device gate: one persistent stage per flag profile

WHAT: Makefile `fxtest-build` now compiles device suites through one persistent
`$(FXTEST_BUILD_DIR)/profiles/<profile>/suite` stage per flag profile (default / duel / facts /
compact) with a stable `suite.ino`; inputs refreshed once per profile per run (build/ kept for
object reuse). Per suite: copy the `.ino` to `suite.ino`, compile with the profile's exact current
build properties, copy `suite.ino.hex`/`.elf` to `$(FXTEST_BUILD_DIR)/<name>/output/<name>.ino.hex`
/`.elf`, run `check-fxtest-ram.sh` on the copied ELF. `FXTEST_CLEAN=1` wipes the profile stages.
`help` text and `docs/dev-flow.md` device-loop notes updated. No source/harness/tool changes.

COMMANDS + NUMBERS
- `make test` → 258 pass / 0 fail, 3.43 s. `make testvm` → 286 pass / 0 fail, 1.15 s.
- Baseline `test_stack` spike (BEFORE edit, real 11.18 s): sketch 17268 B, globals 1737 B,
  RAM data 64 / bss 1673 / used 1737 / limit 2160 / free 423; mode-transition headroom 0x2B7 (695);
  stack base 0x7C9 (1993) / top 0xA8A (2698) / low 0xA1F (2591) / headroom 0x256 (598);
  `test_stack PASSED=4 FAILED=0` PASS.
- Baseline focused (BEFORE, real 55.95 s): blit/items/save/duel_ui all PASS.
- Focused AFTER: `make fxtest-headless FXTEST_INOS='...test_blit ...test_items ...test_save
  ...test_duel_ui' ARDENS=...` → 4 PASS, real 33.57 s.
- `make fxtest-build` warm full wildcard: real 57.08 s (34 suites). Cold full:
  `make fxtest-build FXTEST_CLEAN=1` → **real 75.29 s (1:15)** vs 5:40 baseline (<=1:40 target met).
- `make fxtest-headless ARDENS=...` full: **34 PASS / 0 FAIL**, real 75.06 s.
- Determinism: `shasum -a 256 build/fxtest/test_save/output/test_save.ino.hex` =
  `b1b860665a8dfdf4e77c16aa4d4fe9a3a6a5ac4a2735f651a6bdb04e2779b516` before and after rebuild.
- `test_stack` spike AFTER: real 2.35 s, all RAM/stack numbers identical to baseline
  (17268/1737/64/1673/1737/2160/423; 0x2B7(695); 0x256(598); PASSED=4) — PASS.
- `make test-fxtest-ram` → `fxtest RAM guard: PASS`.
- `FXTEST_CLEAN=1` probe (isolated `FXTEST_BUILD_DIR=build/ob83-clean-probe`,
  `FXTEST_INOS=test_version.ino`): real 11.04 s, layout `profiles/default/suite` + `test_version/
  output`, probe removed afterward.

STORAGE
- `du -sh build/fxtest`: 6.1 M (single warm stage) before → 35 M (5 old-layout stages, ~7 M each)
  after baseline runs → **18 M** after the change (`profiles/` 13 M + 34 per-suite `output/`).
  Old full 34-stage layout ≈ 240 M. `df -h .` 59 GiB free (>=10 GiB reserve).
- Removed stale old-layout per-suite `build/`/`src`/`harness` trees and unused
  `output/*.eep`/`*.with_bootloader.*`; no `cgfx-pack.*` stage remains.

NOTES
- Wildcard is **34** suites (fxdatatest.ino + 33 `test_*.ino`), not 35 as the bead text states.
- Interactive arena-demo Ardens (pid 83703, fxport=d1) left untouched; serial captures did not
  conflict.
- Scope: only `Makefile` (fxtest-build recipe + help) and `docs/dev-flow.md` changed. No commit
  (orchestrator commits).

## rhs7.1 — archive legacy battle contract run (host suites + dormant TUs)

HOST-ONLY. No AVR build run; no `src/` or `tst/fxdatatest/` change.

### Changed
- `tst/main.cpp` — dropped all `#ifndef CGFX_DUEL_BATTLE` includes/registrations
  (legacy contract run) and the orphan `"BattleItemSuite finished"` print.
- `Makefile` — removed `src/engine/battle/{Ai,Damage,Effects}.cpp` from
  `TEST_SOURCES`; kept `Resolve.cpp` (live duel helpers, see Deviations); added a
  4-line comment recording the split.
- Deleted from `tst/` (17 legacy headers): `battle_effects_test.hpp`,
  `battle_setup_test.hpp`, `mode_state_test.hpp`, `transition_test.hpp`,
  `battle_test.hpp`, `battle_ai_test.hpp`, `battle_resolve_test.hpp`,
  `battle_faint_test.hpp`, `battle_session_test.hpp`, `battle_playback_test.hpp`,
  `battle_flow_test.hpp`, `battle_item_test.hpp`, `battle_utility_test.hpp`,
  `battle_slim_session_test.hpp`, `battle_slim_menu_test.hpp`,
  `battle_slim_feedback_test.hpp`, `moves_test.hpp`.
- `archive/battle-legacy/tst/` — added `transition_test.hpp`, `mode_state_test.hpp`,
  `moves_test.hpp` (the 14 other suites were already archived byte-identical to
  the live files).
- `archive/battle-legacy/README.md` — provenance section naming rhs7.1 and the
  reason (duel replaced the ordered-turn engine).
- `archive/battle-legacy/INVENTORY.txt` — appended SHA-256/bytes for the 3 added
  headers under a labeled `rhs7.1 supplement` comment.

### Legacy surface inventory (exact)
`#ifndef CGFX_DUEL_BATTLE` registrations in `tst/main.cpp`: transition, battle,
battle_ai, battle_resolve, battle_faint, battle_session, battle_playback (included
but **never registered** — `BattlePlaybackSuite` had no call site), battle_flow,
battle_item, battle_utility, battle_slim_session, battle_slim_menu,
battle_slim_feedback, battle_effects, battle_setup, mode_state; plus orphan
`moves_test.hpp` (`MoveSuite`, no include/call site). TU lifeline in `TEST_SOURCES`:
`src/engine/battle/{Ai,Damage,Effects,Resolve}.cpp`.

### Commands + results
- BEFORE (clean tree 780cbb5): `make test` → host 240501 / 0, world 258 / 0;
  `make testvm` → 286 / 0.
- AFTER: `make test` → host 240501 / 0, world 258 / 0; `make testvm` → 286 / 0.
  Deltas 0/0/0: the removed suites never ran under `-DCGFX_DUEL_BATTLE`
  (`CPPFLAGS ?= -I. -DCGFX_DUEL_BATTLE`), so the counts are identical and the
  removal is fully attributable.
- `git diff --stat -- src/ tst/fxdatatest/` → empty (untouched). No `src/` change
  ⇒ whole-image byte-identity expected; `make ram` not run (AVR off-lane).
- `grep -n '<legacy suites>' tst/main.cpp Makefile` → none. `ls archive/battle-legacy/tst`
  → archived copies present.

### Deviations / overlaps
- **`Resolve.cpp` kept.** First attempt removing all four TUs failed to link:
  `battle::sideDefeated` / `battle::canSwitch` (defined unguarded in
  `Resolve.cpp:460,478`) are called by the live duel path in `BattleSession.cpp`
  (e.g. `duelResolveRound`, `submitIntent`). `Ai/Damage/Effects.cpp` are fully
  `#ifndef CGFX_DUEL_BATTLE` and were removed. Bead premise that all four are
  dormant is corrected here.
- **Five extra legacy suites beyond the bead text** were also `#ifndef`-only and
  are included: `battle_effects`, `battle_setup`, `mode_state`, `battle_slim_menu`,
  `battle_slim_feedback`. All were already in `archive/battle-legacy/tst/`
  (identical bytes), confirming they were always part of the legacy archive.
- **No legacy-only Makefile helper variable** found to remove (step 3 was
  conditional); only the `TEST_SOURCES` entries were legacy-specific.
- `tst/encounter_test.hpp` retains an inline `#ifndef CGFX_DUEL_BATTLE` assertion
  arm; it is a live suite, so it is left for rhs7.2.

### Wall time
≈30 min worker (recon ~15, edits + 2 host gates ~15). Files changed, no commit
(orchestrator commits).

## rhs7.2 — delete dead legacy arms + dormant Move/Effect/PP surface

WHAT: removed every `#ifndef CGFX_DUEL_BATTLE` arm from the shared battle sources and
unwrapped the now-vestigial `#ifdef CGFX_DUEL_BATTLE` guards into the single duel path;
deleted the dormant `BATTLE_SIMULATOR` blocks (1cge); pruned includes that only the dead
arms needed. Also removed the duel-unreachable BattleFlow feedback arm (obbp) after a
measured win. Owned the single AVR lane.

### Premise check (no-macro legacy build)
`make build ... AVR_SHIPPING_CPP_FLAGS='<relax> -DCGFX_SHIPPING_NO_USB -DCGFX_TRAINER_DEMO'`
(the `battle-test` trainer-demo profile, the only no-`CGFX_DUEL_BATTLE` build) FAILS at HEAD
2b33e4b: `BattleFlow.cpp:63: 'const struct battle::Combatant' has no member named 'attackMask'`
(build/reports/rhs7/battle-test-probe.log). So the `#ifndef` arms compile in no build — the
bead premise holds. Probe tree removed.

### Changed (12 files, +26 / -749)
- `CreatureGathererFX.ino` — render() BATTLE case unwrapped; dropped the dead
  `#ifndef` feedback-draw arm (obbp).
- `BattleState.hpp` — Combatant/BenchSlot/BattleState reduced to duel fields; dropped
  Move/Stats/StatusEffect/StatModifier includes and the legacy AVR size asserts.
- `BattleSetup.cpp` — dropped legacy clear/copy/switch arms, `BATTLE_SIMULATOR` from the
  species-count guard, `EMPTY_MOVE`.
- `BattleSession.{hpp,cpp}` — unwrapped duel arms; deleted `beginPrepared` + simulator
  validators, legacy submitPlayerAction/advance/view/moves arms; removed unused
  NEXT_END_TURN/INVALID_*/otherSide/sideBit.
- `BattleFlow.cpp` — unwrapped duel arms; dropped dead beginOptions/beginReplacement branch
  and the obbp feedback arm (`beginFeedback`, isPresenting block, advance()-result block).
- `BattleMenu.{hpp,cpp}` — duel pick/action/party pages only; dropped MovePayload/legacy
  options+moves+replacement pages, PpGlyph PP glyphs, MoveIds/ReadData/BattleFeedback/PpGlyph
  includes, PAGE_REPLACEMENT/PARTY_FORCED; added explicit `<Arduboy2.h>` (was transitive).
- `BattleScene.{cpp,hpp}` — unwrapped duel HUD arms; comment updated.
- `MoveUses.hpp` — kept only `resetMoveUses` (duel setup zeroes the attack-mask bytes);
  deleted moveUseLimit/remainingMoveUses/spendMoveUse/UNLIMITED_MOVE_USES.
- `tst/encounter_test.hpp` — dropped the inline `#ifndef` stat arm.

### AVR whole-image A/B (pinned lane)
`make ram BUILD_DIR=build/rhs7-lane ARDUINO_BUILD_PATH=build/rhs7-lane/arduino-build ARDUINO_BUILD_CACHE_PATH=build/rhs7-lane/arduino-cache`
- CONTROL (HEAD 2b33e4b): 23380 flash / 1680 static (text 23338 + data 42; bss 1638).
- Candidate A (dead arms only): 23380 / 1680 → **0 B**.
- Candidate B (final, obbp arm removed): **23268 / 1680 → -112 B flash, 0 B static**.
- `avr-nm` candidate B: `battle::BattleSession::finishPresentation()` and every
  BattleFeedback symbol are GC'd (present in control/A). SRAM unchanged ⇒ BattleUi stays 27 B.
- ELFs: `build/reports/rhs7/{control-rhs7.2,candidate-rhs7.2-A,candidate-rhs7.2-B}.elf`
  (+ `*-size.txt`, `*-nm.txt`).

### Verification
- `make test BUILD_DIR=build/rhs7-lane` → host 240501/0, world 258/0.
- `make testvm BUILD_DIR=build/rhs7-lane` → 286/0.
- `make fxtest-headless ... FXTEST_BUILD_DIR=build/rhs7-lane/fxtest` (focused 4): battlepresentation
  102/0, dialog 9/0, mode_state 239/0, stack 4/0 — all PASS. `test_stack` headroom 0x256 (598 B),
  unchanged from the ob83 baseline.
- Full `make fxtest-headless` → **34/34 suites PASS, 0 FAIL**.
- `grep -rn BATTLE_SIMULATOR src/ tst/ tools/ Makefile` → 0 hits.
- `git diff --stat` → only the 12 files above; no generated/`fxdata.h` drift; `tst/fxdatatest/` untouched.

### Move/Effect/PP disposition (what remains + why)
- Kept with live callers: `Move.hpp` (MenuV2.cpp, draw.h), `MoveIds.hpp` (MenuV2.cpp, draw.h,
  renderer_test, duel_session_integration host+device), `Effect.hpp` (Type.hpp, StatusEffect.hpp,
  ReadData.hpp, ActionResult.hpp, effect_test, world_interact_test), `PackedMoveInfo.hpp`
  (MenuV2.cpp, menu_test). `MenuV2`/dialog deps stay per the bead; PpGlyph.hpp stays
  (BattleScene/LureZoneView/draw.h/renderer+blit device suites).
- Duel-reachable headers now free of the packed `Move`/`MoveBitSet`/`PpGlyph`/`PackedMoveInfo`
  types. `Effect` remains in `ActionResult.hpp` (host non-compact `Consequence` fact surface) and
  `Effects.hpp`; `MoveSnapshot` is a 9-byte uint8 POD (BattleView.hpp), not the packed type.
- Out of OWNERSHIP (left, all dead in every compiling build): `Resolve.cpp`, `Ai.cpp`,
  `Damage.cpp`, `Effects.cpp` legacy arms. `Resolve.cpp`/`Ai.cpp` still call the removed
  `remainingMoveUses`/`spendMoveUse` in their uncompiled `#ifndef` arms — harmless.

### Deviations
- BattlePresenter/BattleFeedback had no `CGFX_DUEL_BATTLE` arms (only `__AVR__`/COMPACT), so no edit.
- `finishPresentation` retained (device `test_stack` calls it); `isPresenting`/`finishNormalResult`/
  `emitReplacement`/`beginResultIfNeeded` remain declared but GC-excluded.
- Kept `build/rhs7-lane` (100 M) for orchestrator re-verification; no other tree touched.
  `df -h .` 59 GiB free.

### Wall time
≈70 min worker (recon ~30, edits + host/device loops ~25, AVR A/B ~15). No commit (orchestrator commits).

## rhs7.3 — Measured duel stat collapse: retire five dead stat seeds to hpSeed

WHAT: `CreatureData_t` shrank 11 → 6 B (`{id,type1,type2,evoLevel,hpSeed,attackMask}`);
`Creature` now carries one `uint8_t hp` instead of `stats_t statlist`; authoring JSON
dropped atk/deff/spcatk/spcdef/spd. HP formula unchanged (`2*level + hpSeed*(level/3) + 30`).
`StoreRecord` and the save wire format untouched. Owned the single AVR lane.

### Tools repo (CreatureGathererTools, from HEAD 55501d1; 3 pre-existing dirty files untouched)
- `crates/core/src/model.rs` — `Creature` drops the five stat fields, keeps `hp`.
- `crates/core/src/builder.rs` — `build_creatures` emits 6-byte rows in the pinned order;
  `build_fx_creature_descriptor` emits 6 fields; test vectors/lengths updated.
- `crates/core/src/fixtures.rs` — device `creature_data.hpp` rows now `{id,t1,t2,evo,hp,mask}`.
- `crates/core/src/store.rs` — fixture-field + `json_keys_match_go_tags` asserts trimmed.
- `duel.rs`, `evolution.rs`, `battle_presets.rs`, `arena_demo.rs`, `rewards.rs` — test
  `creature()` builders lose the five seeds.
- `tests/go_parity.rs`, `tests/fixtures.rs` — 6-byte reference vectors; mutation probe
  switches `atk=31` → `hp=31`.
- `testdata/creatures.json`, `testdata/fxdata/data/creatureData.txt` (re-emitted by cargo test)
  — 6-field canonical fixtures.
- `ui/src/editorTypes.ts` — stat fields now optional (keeps pre-existing dirty `App.test.tsx`
  compiling; editor never writes them back); `CreatureEditor.{tsx,test.tsx}`,
  `lib/validation.{ts,test.ts}` drop the stat fields/errors/factories.

Verification: `cargo test -p cgfx-core` → all suites green (unit 230 + integration, 0 failed);
`cd ui && npm test` → 199/199; `npx tsc --noEmit` → clean.

### CLI install
`cargo build --release -p cgfx-core --bin cgfx-tools` → copied to
`~/Applications/CreatureGathererTools/bin/cgfx-tools`.
Installed binary sha256 `5f46d0edff8be079b3321f18f909e10fb450f6050e400b7d85e19bef6ffe3c3e`,
size 3,895,392 B (prior `c80df77b…`). **Deviation**: the Tauri app bundle was intentionally not
rebuilt (CLI-only install, as instructed). Generator identity moved `0.3.0+ge8afd0a7` →
`0.3.0+g55501d10` (installed tool was stale relative to tools HEAD).

### Game repo edits
`data/json/creatures.json` (64 species, stat keys dropped via `jq del`); `src/lib/DataTypes.hpp`
(6-byte `CreatureD` + static_assert); `src/lib/Stats.hpp` (`stats_t` removed; `StatType` kept);
`src/creature/Creature.{hpp,cpp}`; `src/save/SaveController.cpp`; `src/player/Player.cpp`;
`src/engine/arena/ArenaDemo.cpp`; `src/engine/battle/{BattleFlow,BattlePresets,BattleSetup}.cpp`.
Consumer tests: `tst/arena_lifecycle_test.hpp`, `tst/battle_presets_test.hpp`,
`tst/creature_test.hpp`, `tst/duel_session_integration_test.hpp`, `tst/encounter_test.hpp`,
`tst/fxdatatest/creatures_test.hpp`, `tst/fxdatatest/preset_teams_test.hpp`,
`tst/fxdatatest/save_test.hpp`, `tst/fxdatatest/test_world_player.ino`,
`tst/fxdatatest/win_exp_test.hpp`, `tst/opponent_test.hpp`, `tst/save_test.hpp`,
`tst/world_interact_test.hpp` (`statlist.hp` → `hp`, retired `statlist.*`/seed-field asserts).
Spike: `tools/battle-design-spike-data.mjs` derives every mock stat from the one HP seed;
`mock/data.js` regenerated.

**Unplanned-required fix (documented):** `Creature()` previously left `id`/`level`
indeterminate; the smaller record changed stack garbage so `player = Player()` no longer read as
an empty party. `Creature()` now zeroes `id`/`level` deterministically (empty-party entry is a
real state), and `tst/duel_session_integration_test.hpp::beginWildKoSession` dropped its
redundant `player = Player()` (it wiped the prepared preset party, relying on the same UB) —
`preparePlayer()` already installs a preset party over a fresh bag.

### Generation / host / VM
- `make gen` (new CLI) → captured `fxdata/data/creatureData.txt` + `fxdata/generated/creatureData.txt`
  + `tst/fxdatatest/generated/creature_data.hpp` all 64 rows × 6 fields. No separate 32-species
  fixture path exists: the pre-expansion (0..31) and expansion (32..63) rows live in the one
  64-row set; `battlepresentation` exercises index 31, `test_creatures` sweeps all 64.
- `make verify-generated` → PASS; `make test-manifest` → PASS.
- `make test` → host **240493/0**, world **258/0**. `make testvm` → **286/0**.
- `make test-generated-libs` → 48/0 + 5/0 + alias 28/0 PASS. `make test-beta-area` 13/0;
  `make sim-test` pass; `make test-encounter-profile` PASS.
- Makefile has **no `gen-check` or `test-tools` target** (brief assumed both); used
  `make verify-generated` + `make test-generated-libs` + the tools `cargo test` instead.
- Second `make gen` left tracked generated files byte-identical (deterministic).

### Device suites (single lane, Ardens)
`make fxtest-headless FXTEST_INOS='tst/fxdatatest/test_{creatures,save,win_exp,battle_duel,battle_presets,arenacatalog,arenademo,stack}.ino' ...`
— all 8 PASS: creatures 418/0, save 304/0, win_exp 36/0, battle_duel 47/0, battle_presets 106/0,
arenacatalog 46/0, arenademo 15/0, stack 4/0.
`test_stack` headroom **0x26C (620 B)** vs control **0x256 (598 B)** (+22 B headroom);
mode-transition headroom 0x2CB.

### AVR whole-image A/B (`make ram`, pinned BUILD_DIR=build/rhs7-lane)
- CONTROL (HEAD 19642d2): **23268 flash / 1680 static**.
- Candidate rhs7.3: **23158 flash / 1665 static** → **-110 B flash, -15 B static**.
- Delta explanation: the 64×5 = **-320 B** creature payload lives in the packed FX cart image
  (`dist/fxdata.bin`), **not** in the app program flash — the firmware only compiles the
  `src/fxdata.h` addresses. App flash moves by code only (`setStats` loses five `seedToStat`
  calls, `Creature()` ctor shrinks) = -110 B. Static RAM -15 B = 3 party `Creature` instances ×
  5 B (6 → 1 stat byte).
- ELFs: `build/reports/rhs7/candidate-rhs7.3.elf` (+ `candidate-rhs7.3-ram.log`); control
  `candidate-rhs7.2-B.elf` / `final-gate/ram.log`.

### Rebaselined fixtures
- `tools/tests/pack-parity_test.sh` cart sha256 `418ac63b…` → **`6bbcabeb…`**. Cause: creatureData
  -320 B/64 rows, appended field addresses shift, pack input loses the columns, and the rebuilt
  core stamps generator identity `0.3.0+g55501d10`. Layout-equivalence fixtures unchanged; a
  second `make gen` produces an empty diff. `make test-pack-parity` → PASS.
- `tst/fxdatatest/generated/version_data.hpp` — generator identity `ge8afd0a7` → `g55501d10`.
- `fxdata/data/creatureData.txt`, `tst/fxdatatest/generated/creature_data.hpp`, `src/fxdata.h`
  regenerated by `make gen`.

### Frozen / not-updated (recorded, not edited)
- `data/Creaturesheet.csv` + `data/CreatureGatherer Stat Sheet.xlsx` — frozen authoring
  provenance: no build path reads them (only the dead `tst/src/parseCSV.hpp` names the CSV).
- `tst/src/parseCSV.hpp` — already-uncompiled legacy (references retired seed *and* move fields);
  left untouched, not in any test build.
- `mock/battle-mock.js` (stances variant) unchanged; it consumes only `stats.attack`, now sourced
  from the HP seed by the spike generator.

### Storage / wall time
`du -sh build/rhs7-lane` 103 M; `~/code/creaturegather/CreatureGathererTools/target` 1.2 G;
`df -h .` **62 GiB free** (>10 GiB). No commit/push in either repo; bead not closed.
Wall time ≈55 min worker (tools recon/edit ~15, install ~2, game edit + host loops ~20,
gen/VM/manifest ~5, device suites ~8, A/B + report ~5).

## rhs7.4 — Evolution as mask progression: full starter chains and target-kit adoption

WHAT: authored the full three-link starter chains in `data/json/creatures.json`
(species 1->2, 4->5, 7->8; evelevel 16 already authored; terminals at 24 inert)
and made evolution adopt the target species authored kit mask, restoring the
instance-mask == species-mask invariant on both growth paths. Owned the single
AVR/generation lane (pinned `BUILD_DIR=build/rhs7-lane`, evidence
`build/reports/rhs7/`).

### Game repo edits
- `data/json/creatures.json` — `evolves_to` 2 on species 1, 5 on species 4, 8 on
  species 7.
- `src/engine/battle/BattleFlow.cpp` `awardOrdinaryWildExp` — inside the existing
  evolved block: `record.attackMask = readCreatureMask(target);` before the same
  `grown.load(record)` target read. exp / stable slot / missing-HP / retry
  semantics unchanged.
- `src/save/Compaction.cpp` `storeEvolve` — same adoption before `maxHP(record)`.
- `src/lib/ReadData.{hpp,cpp}` (new `readCreatureMask`), device body reads the
  authored per-species mask as one byte from `AttackMasks::attackMask` (the
  table is generated from the same moves as `CreatureData_t.attackMask`).
- `tst/src/ReadData.cpp` — host `readCreatureMask` returns
  `getCreatureFromStore(id).attackMask` (fixture), matching how host already
  substitutes `getCreatureFromStore`.
- `tst/creature_test.hpp` — 6-edge fixture + chain/threshold/terminal asserts.
- `tst/duel_session_integration_test.hpp` — 6-edge fixture; crab mask 4->12 on
  the BattleFlow path; new `duelWildWinEvolutionChainMask` (snail 2->18, squid
  2->10 at 16, terminal inert).
- `tst/save_test.hpp` `BoxedEvolutionTest` — 6-edge fixture; crab 4->12 dup
  assert + snail/squid chain cases (Compaction path).
- `tst/fxdatatest/win_exp_test.hpp` — BattleFlow crab 4->12 assert; boxed crab /
  snail / squid adoption cases.

**Design deviation (documented):** the direct `getCreatureFromStore(target)
.attackMask` read measured **+130 B** app flash (a fresh readDataObject<
CreatureData_t> path), and a `loadTypes`/`setStats` restructure **+324 B**. A
shared 1-byte `readCreatureMask` helper costs **+26 B** (see A/B below), so the
read was factored into `ReadData` — still inside the same transition read, once
per evolution.

### Tools repo (CreatureGathererTools, crates/core/src/evolution.rs only; 3 pre-existing dirty files untouched)
Added a `chained_edges()` fixture plus `accepts_three_link_chains_and_pins_their
_arrays`, `a_chain_middle_is_not_rejected_as_a_cycle` and
`a_back_edge_in_a_chain_is_rejected`. The existing validator already accepts
three-link chains (the cycle walk follows `evolves_to` forward and only rejects a
back edge). Validation-only change: the installed CLI is unchanged
(`cgfx-tools 0.3.0+g55501d10`), so no rebuild/reinstall.
`cargo test -p cgfx-core` → lib **233 passed / 0 failed** (+3), all integration
suites green.

### Generation / host / VM
- `make gen` (installed tool 0.3.0+g55501d10) → `fxdata/generated/evolution.txt`
  (gitignored) `evolutionSource {0,1,3,4,6,7}`, `evolutionTarget {1,2,4,5,7,8}`,
  `evolutionLevel {8,16,8,16,8,16}` (+9 B cart payload vs the 3-edge baseline).
- `src/fxdata.h` `FX_DATA_BYTES` 748546 → **748555** (+9); `evolutionTarget`
  0x0B6B79, `evolutionLevel` 0x0B6B7F; `Rewards.*` shift +9.
- `tst/fxdatatest/generated/evolution_data.hpp` 3 → 6 edges.
- `make verify-generated` PASS; `make test-manifest` PASS; `make test-generated-libs`
  PASS; `fxdata/generated/*` + `dist/*` are gitignored build outputs.
- `make test` → host **240518/0**, world **258/0**. `make testvm` → **286/0**.

### Device (single lane, Ardens)
`make fxtest-headless FXTEST_INOS='test_win_exp test_save test_battle_duel test_stack'` → all PASS:
win_exp **49/0** (+13 new asserts), save **304/0**, battle_duel **47/0**, stack **4/0**.
`test_stack` headroom **0x26C (620 B)** = control; mode-transition 0x2CB.

### AVR whole-image A/B (`make ram`, pinned dirs)
- CONTROL (HEAD ceecdfe, rhs7.3): **23158 flash / 1665 static**.
- HEAD code + regenerated fxdata.h: **23172** (+14).
- Constant-stub line (`record.attackMask = 12`): **23176** (+4 code).
- CANDIDATE rhs7.4: **23202 flash / 1665 static** → **+44 B flash, 0 B static**.
- Breakdown: **+9 B** FX payload lives in the cart (`dist/fxdata.bin`), not app
  flash. **+14 B** app flash from the reshuffled `fxdata.h` address/`FX_DATA_BYTES`
  constants. **+26 B** for the shared 1-byte target-mask read on both paths.
  **+4 B** for the added store. Compaction change measured **0 net**. Static RAM
  unchanged (no new globals). ELF:
  `build/reports/rhs7/candidate-rhs7.4.elf`; log `candidate-rhs7.4-ram.log`;
  rebuilt control `control-rhs7.4-ram.log`.

### Pack-parity rebaseline
`tools/tests/pack-parity_test.sh` cart sha256 `6bbcabeb…` → **`f8bd14fd96b6f430c98a76e4266ff60a13ddb3f7871ebb16b233060bc66e9307`**
(+9 B evolution payload, +9 shift of Rewards; generator identity unchanged
0.3.0+g55501d10). `make test-pack-parity` PASS.

### Storage / wall time
`du -sh build/rhs7-lane` **104 M**; `df -h .` **57 GiB free** (>10 GiB). Scratch
A/B source copies removed; evidence kept. No commit/push in either repo; bead not
closed. Wall time ≈75 min worker (recon edit ~15, tools test ~3, gen/host/VM/
generated ~10, device ~8, A/B + size-trim loop ~30, report ~9).

### Orchestrator follow-up: test-beta-area gate failure + fix
First `make final-gate` failed at `test-beta-area`: the contract pinned exactly
three first-stage edges and `creatures.json` now authors six. Fixed inline:
`data/beta-area.toml` gains the three second-stage edges (1->2, 4->5, 7->8 at
level 16; comment updated) and `tools/tests/beta-area_test.mjs` expects the six
pairs with per-edge `level == source evelevel`. `make test-beta-area` 13/13;
second final gate PASS 23202/1665. The worker's focused suites did not include
test-beta-area; noted for future cross-check dispatch prompts.
