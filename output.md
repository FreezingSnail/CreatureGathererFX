# CreatureGathererFX — session reports (active wave)

Older session reports (previous wave through jp8.11.5, including ogh.18..jp8.11.27 inputs) are archived at
`docs/reports-archive/2026-10-18-active-wave.md` and `docs/reports-archive/2026-10-16-earlier-sessions.md`.
Keep this file to the active wave; rotate completed sections out per `AGENTS.md` -> "Session efficiency rules".

## jp8.11.27 — first-session onboarding design (brief only)

Design-only deliverable; **no source/AVR/asset/map/generator/build/gate change** (off the AVR lane).
Bead closed after the brief + leaf follow-ups landed.

### Changed / created (exact paths)
- `docs/first-beta-onboarding-brief.md` (new) — first-session script, minimal affordance set,
  per-moment state map, bounded estimates, observable acceptance, OWNER-PENDING list.
- `README.md` — one pointer line to the brief after the intro paragraph.
- `output.md` — this section.

### What the recon found (current source, not historical)
- Shipping ordinary fresh boot shows no onboarding: `LureZone::showIntro()` boot card is
  `CGFX_LURE_WEDGE`-only (INO:169-174); compact profile has no VM message path
  (`World.cpp:133-141`).
- Already-shipped live affordances: distinct tile art; the **native** lure picker overlay on the
  deposit tile (`World.cpp:461-463` → `LureZone::openPicker`/`drawOverlay`, INO:259; not a VM
  popup); plant plant/harvest with native "Gathered" message (`World.cpp:474-491`); duel pick row
  with up/down matchup markers (`DuelView.hpp:97-104`, `BattleScene.cpp:122-127`), SWAP/WAIT cells,
  HP numbers+bars, team pips, sprite type badges, per-round captions.
- Real gaps that render nothing today: **plant PlantStage** (map draws one static plot frame),
  **gather progress** (`state.gather.progress/need` not drawn), and the wild **GATHER action is
  hidden behind `B`** (`BattleMenu.cpp:169-179`) with no cue.

### Selected approach
Affordance-only (existing art + native picker + live duel HUD), with only the invisible state made
visible. Rejected: one-shot hint flag (ABI), scripted-tour warp (pocket already exists), full-VM
(no popups). Teaching moment → affordance → state table in the brief; deposit-charge and rest cues
are called out as sufficient-for-first-capture and deferred OWNER-PENDING.

### Created beads + edges
- `CreatureGathererFX-jp8.11.30` P1 — visible plant plot growth/ripeness (state
  `plants.plantStages.getStage(0)`; est. ~80-200 B flash, 0 static).
- `CreatureGathererFX-jp8.11.31` P1 — surface GATHER in the shipping duel pick row for gatherable
  encounters (state `state.gatherable`; est. ~120-300 B flash, 0 static).
- `CreatureGathererFX-jp8.11.32` P2 — gather progress/need cue in the duel HUD (state
  `BattleState::gather`; est. ~40-120 B flash, 0 static).
- Edges: `bd dep add CreatureGathererFX-jp8.11.15 {30,31,32}` → jp8.11.15 now depends on all three.
Labels `beta,first-beta,roadmap,impl-ready`; parent epic jp8.11. No gates touched; no other bead
status changed. Estimates are from the dated `d8cfd1c` control (23,252 B / 1,665 B / 620 B stack);
whole-image A/B decides.

### Verification (read-only; no build/gate run)
- `bd show CreatureGathererFX-jp8.11.27` acceptance satisfied: brief has script + affordance set +
  state needs + observable acceptance + filed leaves.
- `bd show CreatureGathererFX-jp8.11.15` → DEPENDS ON .30/.31/.32 present; `.31` BLOCKS .15.
- `git status` → only `.beads/interactions.jsonl`, `README.md`, new brief.
No commit, push, publication or full gate (per instructions). Wall time ≈25 min.

### OWNER-PENDING (unchanged, not approved)
15-25 min session length; deposits/wins; capture attempts; KO/loss cost; explicit-only power-loss
window.

## jp8.11.25 — author the first-beta map: encounters, deposits, warps and gating

Outcome: **closed, committed**. Content-authoring bead on the single build lane (`BUILD_DIR=build`;
`cgfx-tools 0.3.0+g55501d10`; headless Ardens `~/code/arduboy/Ardens/build/Ardens.app`).

### Changed (exact paths)

- `fxdata/data/encounters.txt` — zones now chunks 1/33/65/66 (was 0/1/33), nibbles 1..4, typeMask 15,
  lvlOffset 0; tables replaced with the five contract tables (base, spirit, water, wind, earth;
  floor 2 / ceil 5).
- `maps/world_map.json` — NPC/script markers only: in-bounds `@beta_door` (12,7) <-> `@beta_gate`
  (12,9) reciprocal `tp` pair; out-of-area `@test_house`/`@test_teleport` warps removed. No tile or
  prop changed (raw_map byte-identical).
- `tools/tests/beta-area_test.mjs` — +5 tests (packed tables == contract; zone coverage exactly the
  route chunks with lure->table mapping and level band; reciprocal in-bounds door pair + sealed
  reachable set == authored walkable cells; reachable-chunk zone coverage; unpacked grids inert).
- `tools/tests/pack-parity_test.sh` — intentional rebaseline `a91840f3... -> 2e56abb2...` (cart
  bytes changed; all symbol addresses and `src/fxdata.h` byte-identical).
- Test fixtures/suites moved with the markers: `tst/chunk_test.hpp`, `tst/fxdatatest/generated/
  script_data.hpp` (regenerated), `tst/fxdatatest/{scripts_test,tiles_test,mode_state_test}.hpp`,
  `tst/script_tests/warp_test.hpp`; `README.md` wild-demo note.
- `tst/fxdatatest/mode_state_test.hpp` (orchestrator fix): chunk 65 is now authored, so the battle
  round trip loads its zone on the transition; the suite asserts the packed zone payload instead of
  "cache cleared" and keeps the cleared-path check via a zoneless chunk (chunk 0).

### Stale premise (recorded)

The bead description's "zero encounter tiles" is inaccurate at HEAD: GID 275 (tsx 274) already
carries `walkable+encounter`, so the authored area's floor was already encounter-flagged; the real
gaps were fixture tables, missing zones for chunks 65/66 (lure cleared, no rolls) and in-area test
warp/NPC fixtures. Worker authored to the acceptance; test additions pin all of it.

### Verification (all after regen; logs `build/final-gate/{check,ram}.log`)

- `node --test tools/tests/beta-area_test.mjs` — 20 passed / 0 failed (gen up to date).
- `make test` 258/0; `make testvm` 286/0; `make test-encounter-profile` PASS; `make test-pack-parity`
  PASS (layout equivalence + perturbation + parity).
- `make final-gate ARDENS=<headless>` — **PASS** (34 device suites, all `FAILED=0`;
  `test_stack` headroom 622 B; `test_readcounter` 11/0; `test_world_player` 77/0; `test_encounter` 47/0;
  `test_battle_duel` 47/0; `test_mode_state` 259/0).
- AVR A/B (`make ram`, same source control `d8cfd1c`): flash 23,252 -> 23,252 B (free 6,444);
  static RAM 1,665 -> 1,665 B (free 495); stack headroom 620 -> 622 B. No resource cost; reserve intact.

### Failed attempts (recorded)

1. First `final-gate` run: `fxtest-headless: BLOCKED` — `/Applications/Ardens.app` lacks
   `captureserial`; re-ran with `~/code/arduboy/Ardens/build/Ardens.app` (flake path fixed for this
   wave; jp8.11.29 owns the default-path/CI pin).
2. Second run: `test_mode_state FAILED=16` (stale expectation above); fixed, focused suite 259/0,
   full gate then PASS.

### Process / storage / wall time

Worker (`slugineer-worker`) cancelled by a server restart after edits + `make gen` + host build;
orchestrator finished verification inline (outcome recorded, no partial files left). No abandoned
`build/cgfx-pack.*` stages; `build/` 1.3 GiB; disk free 48 GiB. Wall time: worker partial ~45 min
(cancelled, no report), orchestrator inline ~40 min.

## 9ha9.1 — Pack theme streams + catalog (worker)

Changed: `data/music/{overworld,combat,victory,defeat,opening,village,forest,coast,marsh,ruins,exploration}.bin`
(42584 B total, upstream ATMlib2FX `c3d5e7c`, all magic 0xC1), `data/music/README.md`,
`data/music/catalog.tsv`, `fxlayout.toml` (11 named raw entries appended before `[save]`),
`src/audio/MusicCatalog.hpp` (enum + `kMusicTracks[11]`), `tst/audio/music_catalog_test.cpp`,
`Makefile` (`test-music`, in `check`), `tools/tests/pack-parity_test.sh`.

- `make gen`: PASS, 7.9 s. `src/fxdata.h` diff is append-only: only `FX_DATA_PAGE`
  `0xf4af->0xf408`, `FX_DATA_BYTES` `708801->751385`, plus 11 new `music_*` lines.
- Music address range `0x0AD0C1..0x0B7719` (exclusive end), exactly 42584 B.
- `make test-music`: 104 passed, 0 failed (2.8 s). Covers exact sizes, magic, loop_start_rel
  boundary/length, record framing to END, catalog/header agreement, symbol publish + order.
- `make test-generated-libs`: 59 passed, 0 failed + invariants/arena/alias suites PASS (1.6 s).
- `make test-manifest`: PASS (9.6 s).
- `make test-pack-parity`: PASS after rebaseline `2e56abb2.. -> d6f8daa3..` with append-only note (8.0 s).
- `make test`: 258 passed, 0 failed (2.8 s).
- Header syntax: `avr-g++ -fsyntax-only` on `MusicCatalog.hpp` OK (not part of runtime yet).
- Deviations: none. No commit/push. Storage: shared `build/` 1.3 GiB, disk free 61 GiB.
- Wall time: ~25 min.

## 9ha9.2 — vendor ATMlib2FX + AUDIO toggle (worker)

Created `src/audio/atm/`: upstream `c3d5e7c` `atm_synth.{c,h}`, `osc.{c,h}`, `cmd_parse.c`,
`atm_cmd_constants.h`, `atm_fx_compact.{c,h}`, BSD-3 `LICENSE`, and `README.md` pinning commit +
upstream sha256s. No `atm_fx_stream.*`, `ATMlib.cpp`/`.h`. Added `src/audio/MusicConfig.hpp`
(`CGFX_AUDIO` default 0, compile-time only), `src/audio/Audio.hpp` (inline no-ops under
`!CGFX_AUDIO || TEST`), `src/audio/Audio.cpp` (whole body guarded; vendored C headers in
`extern "C"`; provides `atm_fx_compact_fetch` via `FX::readDataBytes`). INO includes the facade and
calls `arduboy.audio.on(); audio::begin();` under `#if CGFX_AUDIO` after save load/seed. Makefile:
`AUDIO ?= 0`; `AUDIO=1` adds all five defines to both `compiler.cpp.extra_flags` and
`compiler.c.extra_flags`; `AVR_FXTEST_AUDIO_DUEL_BUILD_PROPERTIES` reserved for 9ha9.3; Mini
rejects `AUDIO=1`; help/overrides updated.

Local vendored edits: `atm_synth.c` + `osc.c` outer `#if CGFX_AUDIO` only.
`cmd_parse.c`/`atm_fx_compact.c` keep upstream guards (`ATM_PATTERN_STACK_DEPTH`,
`ATM_FX_COMPACT_IMPL`) and compile empty standalone.

Verification (`BUILD_DIR=build/rhs7-lane`, existing cache):
- Control preserved at `build/rhs7-lane/control-9ha9.2.elf`: `.text` 23210, Program 23252, static 1665.
- `make build/ram AUDIO=0`: `.text` 23210, Program 23252, static 1665; raw flash images identical
  (`avr-objcopy -O binary` both sha256 `ccc80921df886708208c42e5c637960e43cd3c8af56f5bb944d1a234271e6f26`);
  no `atm_`/`osc_` symbols.
- `make ram AUDIO=1`: links; `.text` 26884, Program 26926 (**+3674**), static 1895 (**+230**);
  free flash 2770/29696, static free 665/2560.
- `make mini AUDIO=1`: fails before compile, `mini: AUDIO=1 is not supported; the ATMlib2FX music
  runtime is FX-only`.
- `make test`: host 240623 passed / 0 failed, world 258 / 0.
- Header syntax: default and `-DTEST -DCGFX_AUDIO=1` both bind no-op inlines.
- No device gate (per bead). No commit/push. Storage: `build/rhs7-lane` 107M, disk free 61 GiB.
- Wall time ~25 min.

## 9ha9.3 — music service, save pause, audio read accounting, device proof (worker)

Changes: `CreatureGathererFX.ino` calls `audio::service()` after `FX::display` under `CGFX_AUDIO`;
`SaveController::begin` pauses before `captureLiveSaveState`, `advance` resumes on `Done`/`Failed`
(state restore unchanged for Done only); `FxReadCounter` adds `audioReads()`/`recordAudio()`/
`audioCount()`, zeroed by `resetFrame`, never in `frameReads`; `Audio.cpp fetch` records the audio
category; Makefile adds `$(AVR_FXTEST_AUDIO_FLAGS)` (five defines + `ATM_FX_COMPACT_DIAG=1` only
when `AUDIO=1`) to default/no-facts/compact/duel/audio-duel C++ and C flags. `readcounter_test.hpp`
gains the audio proof under `CGFX_AUDIO`; `test_stack.ino`/`stack_test.hpp` start audio, service
once, and pause/resume around `runSave`.

Verification (`BUILD_DIR=build/rhs7-lane`, lane shared with 9ha9.2):
- `AUDIO=1 make fxtest-spike FXTEST_SPIKE_INO=tst/fxdatatest/test_readcounter.ino
  FXTEST_BUILD_DIR=build/rhs7-lane/fxtest-audio`: `test_readcounter PASSED=20 FAILED=0`,
  `audio ticks=107 serviced=100 underruns=0 error=0 max_service_gap=2`; test_stack PASSED=4,
  mode-transition headroom 469 B, save stack headroom 374 B (>=219). readcounter RAM 2070/2160.
- `AUDIO=0` same spike into `fxtest-control`: readcounter PASSED=11, test_stack PASSED=4,
  save headroom 622 B; RAM 1595/2160.
- `AUDIO=0 make ram`: flash 23252, static 1665 (free 6444/895). Control ELF preserved at
  `build/rhs7-lane/control-AUDIO0.elf/.hex`. Raw flash image (`avr-objcopy -O binary`) sha256
  `ccc80921...e6f26` matches the 9ha9.2 control image exactly, so AUDIO=0 stays byte-identical.
- `AUDIO=1 make ram`: flash 27204 (+3952), static 1895 (+230), flash free 2492, static free 665;
  candidate ELF `build/rhs7-lane/candidate-AUDIO1.elf`. Top audio symbols: `atm_fx_compact_state`
  103 B, `channels` 84 B, `osc_params_array` 22 B, `osc_cb` 18 B, `osc_pha_acc_array` 8 B.
- `make test`: host 240623 passed, world 258 passed, 0 failed.
- Deviations: audio proof lives in `test_readcounter` rather than a new `test_audio` suite per
  dispatch; ~100 x 20 ms loop (~2 s emulated) instead of the bead's 30 s. No full AUDIO=1 fxtest,
  no final gate (per dispatch). Save proof writes one checkpoint from blank sectors; assertions were
  not weakened.
- Storage: `build/rhs7-lane` 139M, disk free 61 GiB. No commit/push. Wall time ~20 min.

## 9ha9.4 — route themes by state + docs (worker)

Changes: new host-safe `src/audio/MusicTracks.hpp` (enum + `kResultCueFrames = 104`);
`Audio.hpp/.cpp` add `playTrack`/`playResult`/`currentTrack` behind a 3-byte ready/countdown state
(each also a no-op inline under `AUDIO=0`/`TEST`); `ModeState::enterBattle` -> `Combat`,
`exitBattle` -> `Overworld`; `BattleFlow::finishBattle` plays `Victory` for
`Win`/`Gathered` and `Defeat` for `Lose`/`Draw` after the world state is restored (`Fled`/`Escaped`
keep the plain world theme); `readcounter_test.hpp` routing smoke test (4 assertions);
`README.md` + `docs/dev-flow.md` AUDIO sections.

Verification (`BUILD_DIR=build/rhs7-lane`, lane reused from 9ha9.2/3):
- `make test`: host 240623/0, world 258/0. `make test-music`: 104/0.
- `AUDIO=0 make ram`: 23252 flash / 1665 static (free 6444/895). `avr-objcopy -O binary` sha256
  `ccc80921...e6f26` == `build/rhs7-lane/control-AUDIO0.elf`: shipping image unchanged.
- `AUDIO=1 make ram`: 27524 flash / 1898 static (flash free 2172, static free 662). Pre-routing
  9ha9.3 was 27204/1895, so routing = **+320 flash / +3 static**. `kMusicTracks` is PROGMEM
  (`.data` 55 B before the move).
- `AUDIO=1 make fxtest-spike FXTEST_SPIKE_INO=tst/fxdatatest/test_readcounter.ino`
  (`FXTEST_BUILD_DIR=build/rhs7-lane/fxtest-audio`, Ardens = local captureserial build):
  `test_readcounter PASSED=24 FAILED=0`, `audio ticks=107 serviced=100 underruns=0 error=0
  max_service_gap=2`, suite RAM 2100/2160 (60 free); `test_stack PASSED=4 FAILED=0`,
  mode-transition headroom 466 B, stack headroom **373 B** (9ha9.3: 469/374).
- `AUDIO=0` control spike (`fxtest-control`): `test_readcounter PASSED=11`, `test_stack PASSED=4`,
  headroom 622 B / mode 715 B.
- No `make gen`: no authored or generated input changed; `git status` shows no generated diff.

Deviations:
- Enum moved from `MusicCatalog.hpp` to `MusicTracks.hpp`: the facade takes `MusicTrack` in the
  `AUDIO=0`/`TEST` stubs, and `MusicCatalog.hpp` includes device-only `fxdata.h` (`__uint24`), which
  broke the host build.
- First measurement (RAM table) was `AUDIO=1` 27516/1954 and suite 2156/2160 (4 B free); moving the
  theme table to PROGMEM recovered 56 B static for +8 B flash. No other design change.
- Did not run the full final gate or a full AUDIO=1 fxtest sweep (per dispatch).

Storage: `build/rhs7-lane` 139M, disk free 60 GiB. No commit/push. Wall time ~50 min.

## 9ha9 orchestrator closeout

Commits: `bd7e53c` (9ha9.1), `ccbd0c7` (9ha9.2), `bd638d5` (9ha9.3), `4c691a1` (9ha9.4),
`3077848` (defeat-cue test). Default `AUDIO=0` final gate at `build/rhs7-lane/final-gate-9ha9-4`:
PASS, 23,252 B flash / 1,665 B static / 622 B test_stack. `AUDIO=0` `.text` sha256
`359eade6...a083` equals the pre-audio control. `AUDIO=1`: 27,524 B flash / 1,898 B static
(+4,272/+233), free 2,172/662 against the hard limits; focused audio spike
`test_readcounter PASSED=26`, `test_stack PASSED=4`, underruns 0, error 0, max_service_gap 2,
stack 373 B. `test_readcounter` is near the static test limit (2,100/2,160, 60 B free); it is
compiled with audio only for the focused `AUDIO=1` suite. No SFX; remaining area themes are packed
but not selected. No network/download; all runtime sources copied from the local pinned ATMlib2FX
checkout.

## ww7r — combat music in trainer battle-test

`CGFX_TRAINER_DEMO` now starts `arduboy.audio.on(); audio::begin();
audio::playTrack(MusicTrack_Combat)` after `enterBattle`/`beginTrainer` and before its early
return. `BATTLE_TEST_CPP_FLAGS` now derives from the shipping profile so the target has the duel
flags the current source requires. `AUDIO=0` remains silent.

Builds with the battle-test profile (`CGFX_SHIPPING_NO_USB + CGFX_DUEL_BATTLE +
CGFX_TRAINER_DEMO`): AUDIO=0 21,924 B flash / 1,748 B static; AUDIO=1 25,842 B flash / 1,977 B
static (+3,918/+229; 183 B static free). Ardens absim boot of the AUDIO=1 trainer image exited 0.
Default final gate unchanged (audio-less code path).
