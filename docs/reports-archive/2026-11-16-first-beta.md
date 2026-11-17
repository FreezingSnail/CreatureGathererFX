# CreatureGathererFX — session reports (active wave)

Older session reports (previous wave through jp8.11.5, including ogh.18..jp8.11.27 inputs) are archived at
`docs/reports-archive/2026-10-18-active-wave.md` and `docs/reports-archive/2026-10-16-earlier-sessions.md`.
Keep this file to the active wave; rotate completed sections out per `AGENTS.md` -> "Session efficiency rules".

## 2mfl — project-local OpenCode roles (workflow setup)

Added project-local role configuration only. No game content, generation, firmware, package,
device, global-config, or git operations.

### Changed paths
- `opencode.json` — DeepSeek model, `cg-producer` default, `docs/agent-roles.md` instructions.
  Inherited plugins/providers/permissions were not copied or replaced.
- `.opencode/agents/cg-{producer,world,story,art,gameplay,tooling}.md` — six
  `opencode-go/deepseek-v4.1-flash` profiles; producer is primary, specialists are `all`.
  Producer denies all subagent targets then allows exactly the five specialists; specialists deny
  all subagent calls.
- `docs/agent-roles.md` — shared role contract, dispatch inputs, build-lane/gate/report rules,
  direct launch, and Gangway bridge usage.
- `AGENTS.md` — short role pointer plus OpenCode/DeepSeek primary-implementation preference.
- `output.md` — this section.

### Validation
- Node JSON/YAML check: PASS. Six roles, expected modes/models, producer allow-list, specialist
  deny rules, root config, and `docs/agent-roles.md` reference all matched.
- Parent Bun `JSON.parse` + `YAML.parse` check: PASS for all six configs/models/modes/delegation
  lists.
- Installed CLI: `opencode v2.0.15`; top-level `opencode` has no `--agent`; direct selection is
  `opencode run --agent <role>`.
- Live bridge smoke: producer session `ses_e22b28567ffeHCgDzCkWktlzIn` enumerated all five
  specialists and dispatched one read-only `cg-story` child
  `ses_e22b25a36ffemgIchA4I2leyg1`; the child completed `succeeded` with correct boundaries and no
  edits, builds, or Beads mutations. No server restart was needed.
- `opencode debug agents` was interrupted after a service-start hang; no service restart or port
  change was performed. No firmware/generator/device checks were applicable or run.

### Caveats
- A fresh OpenCode instance is troubleshooting only if discovery looks stale; it is not required
  by this setup.
- Root `instructions` is set; role prompts also require reading `docs/agent-roles.md`.
- TUI `@cg-*` mentions remain optional/unverified; `--agent` and the bridge are deterministic.

Wall time: DeepSeek implementation 10m46s and producer smoke 30s (overlapping, from bridge
timestamps); parent setup/review approximately 3 min. No integrated gate.
Storage: 67 GiB free; existing `build/` 109M and role profiles 24K. Removed the owned 20K CLI
scratch at `/private/tmp/cgfx-opencode-role-check` after checks; no build cleanup was needed.

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

## gy31 — stale ARDENS fallback

An exported `ARDENS=$HOME/code/Ardens/...` (nonexistent) blocked `battle-test` before Makefile
discovery. `ARDENS` now prefers the local headless build
`$HOME/code/arduboy/Ardens/build/Ardens.app/Contents/MacOS/Ardens`, falls back from a stale
environment path, and still lets an explicit command-line `ARDENS=` win. Verified `make -n
battle-test` with the stale env selects the local headless path, and a full default `final-gate`
without an explicit ARDENS ran all device suites and passed (23,252/1,665/622).

## kdss — demo targets load ELF

Interactive `run`, `battle-test`, `arena-demo` and `lure-demo` now pass the built
`CreatureGathererFX.ino.elf` to Ardens instead of the HEX. `RUN_HEX` became `RUN_ELF`. Packaging
still uses HEX (`arena-package`), and device suites still load their HEX files. README manual
launch examples were updated to ELF. Verified `make -n` for all four targets, built `AUDIO=1
battle-test` (25,842/1,977), and loaded its ELF through local headless Ardens with
`captureserial` (exit 0).

## fr4g — first-playable-slice planning and queue reconciliation (producer)

Planning-only. No implementation, AVR build, `make gen`/pack, Ardens, install, git mutation
(read-only status only), remote sync, or closure of existing implementation beads. Full plan and evidence live in
`bd show CreatureGathererFX-fr4g` (design + notes); this is the compact session record.

### Reconciled journey

| Step | Current status | Owner / leaf |
| --- | --- | --- |
| Fresh arrival, starter party 0/3/6 L3, empty bag | implemented | closed jp8.11.24 |
| Explore, plant, harvest one fine spirit lure | mechanic implemented; stage 0..3 invisible | art jp8.11.33 -> jp8.11.30 |
| Ordinary wild duel | implemented | closed battle work |
| Deposit lure at (9,2) | implemented native picker; weak visual cue | optional .33 frame 4 |
| Gatherable capture | engine implemented; GATHER hidden behind B; no progress | .31 -> .32 |
| Collection / party swap / bag | missing; bag WIP retained | .5.16 (owner) -> .8 -> .9 |
| Consumable heal in battle | missing (`UseItem` no-op) | .21 -> .19 |
| XP / first evolution at L8 | implemented | closed |
| Fixed boss / persistent ending | missing runtime; (21,9) crystal, only (20,9) approach | .13 after .9/.28 |
| Explicit SAVE/reload | load/recovery implemented; no gameplay save; Failed soft-locks | .23 after .8 |
| Defeat recovery | implemented | closed |

World audit: **no map/TSX/zone change required**. `node --test tools/tests/beta-area_test.mjs`
20/20; 93/93 reachable cells are encounter tiles in six zoned chunks; five tables map
correctly; door pair reciprocal/in-bounds. Boss has no trigger/NPC/script.

### Critical path and waves

`.5.16 -> .8 -> .9 -> .23 -> .13 -> .15 -> .2.7 -> .16 (release gate)`, with side chains
`.21 -> .19`, `.28 -> .13`, `.31 -> .32 -> .15`, `.30 -> .15`, `.29`/b2a.6/.4.10 -> `.16`.
`.5.16` remains in_progress/assigned to FreezingSnail; agents will not claim, reassign or
discard it. The critical chain is not dispatchable until the owner decides.

- **Wave 1 (independent leaves, one lane `build/rhs7-lane`, one cg-gameplay task at a time):**
  cg-art `.33` (no lane); cg-gameplay `.21` then `.28`; `.31` then `.32`; `.30` after `.33`.
  Estimated flash ~0.4-1.1 KB. `.29` waits for verified Tools release artifacts.
- **Wave 2 (after the `.5.16` decision):** `.16 -> .8 -> .9 -> .23 -> .13 -> .15`, then
  `.2.7`, `.4.10`/b2a.6, `.29`, and the `.16` gate. Remaining envelope ~2.9-6.7 KB flash
  against ~5.9 KB planning headroom; `.16` is the largest unknown (stop/trim near 1.3 KB).

### Frozen contracts recorded as bead notes

- `.16` exposes four-row bag only; `.8` owns root BAG/COLLECTION; `.23` appends SAVE.
- `.31`: no 7th pick-row cell; persistent centre-strip `gather` cue + existing B action page
  selector; acceptance amended. `.19`: ITEMS on the same B page, pick row unchanged.
- `.32`: 32px x1px track at (48,47), filled = round(32*progress/need), gatherable only.
- `.30`: separate appended `pocketProps` fxlayout image entry from `.33`; no tiles/sprites edit;
  GID 551 stage frames and GID 545 frame 4 by tile ID.
- `.13`: faced-coordinate trigger (21,9), single L8 species-31 trainer, no `duel.toml` ardu
  demo row; inert-script flag source (`flag_door` stays 0, `flag_beta_complete` next).
- `.28`: lane/`test_stack` corrections; `.23`: fix Failed retry/return soft-lock; `.15`: record
  encounter density (~25 rolls before first harvest) and do not adopt OWNER-PENDING numbers.

### Beads changes

Created `CreatureGathererFX-jp8.11.33` (P1 cg-art pocket props) and dependency `.30 <- .33`.
Appended producer notes to `.30/.31/.19/.32/.13/.28/.29/.8/.16/.23/.15/.21` and the jp8.11
epic; amended only `.31` acceptance. No status changes, reassignment, or closures.

### Owner decisions required

1. `.5.16`: keep the owner WIP, or authorize cg-gameplay to port the stash and finish it?
2. Story register: keeper/predecessor + line `ardu yields the clearing` (recommended) vs blank/feral.
3. Deposit cue: add the 16x16 lure box frame (recommended) vs keep the boulder.
4. Pacing: keep contract-implied 13..31 wins / 15..25 min and measure, or authorize a rebalance
   spike after the `.15` trace (all cells are 10% encounters).
5. Edge policies: full-HP heal refuses without consuming; full-64 collection has no release;
   explicit save only, no extra loss penalty (recommended defaults; confirm/override).
6. Boss: single L8 ardu (recommended) vs 3-member gauntlet; flag: inert script reference
   (recommended, no Tools change) vs explicit Tools declaration.

### Evidence / storage / wall time

Specialist read-only input: cg-story ~30m, cg-world ~30m, cg-art ~30m, cg-tooling ~30m,
cg-gameplay ~55m. Producer recon/reconciliation ~35m. Installed `cgfx-tools 0.3.0+g55501d10`
exposes only `wfc`; no world CLI. Tools Beads/release status unverified (DB lock outside
permitted root; do not retry unchanged). No build lane created; `build/` 109M, disk 67 GiB free.

### zh8c role-contract tightening (2026-11-14)

Changed: `.opencode/agents/cg-producer.md`, `cg-story.md`, `cg-world.md`, `cg-art.md`,
`cg-gameplay.md`, `cg-tooling.md`, `docs/agent-roles.md`, this report.

- Shared doc reduced to 151 lines: mode/model/permission facts, launch/bridge use, role table,
  PLANNING vs IMPLEMENTATION phases, selective dispatch packet/order, PROPOSED authority,
  checkpoints, compact reporting, boundaries/current facts.
- Producer now starts from the next player-facing decision, gathers context once, sends one
  packet, and only adds implementation bead ID/verification/lane when implementing.
- Specialists gained concise domain planning patterns and no-sweep planning rules.
- Frontmatter modes, model `opencode-go/deepseek-v4.1-flash`, and delegation permissions unchanged.

Checks (Bun 1.4.2): parsed all six frontmatter blocks with `Bun.YAML.parse`; modes (producer
`primary`, five `all`), models, and permission lists matched baseline; `opencode.json` parsed with
`default_agent=cg-producer`; final shared doc 151 lines. Result `VALIDATION_PASS`.

Parent review clarified that the producer owns relevant current queue checks, while planning
specialists use supplied excerpts; planning advice need not create a separate child bead.
Added factual/prerequisite review to producer/story instructions after the first smoke draft
invented seeds, deposit-before-harvest and recovery behavior. Creative framing cannot change
existing mechanics; added mechanics must be labeled PROPOSED with dependencies.

Live smoke `ses_e22720888ffedoCqAa2fF1D741`: supplied-context producer/story draft in 37.5s,
exactly one subagent call and no shell/read/Beads/file tools across the session tree (read-only
SQLite audit). Behavioral scope PASS; initial content review FAIL on the assumptions above.
Correction request `cgfx-zh8c-planning-fact-check-20261114` returned in 17s without further tools
and fixed the main causal errors. This is a prompt/dispatch check; no creative proposal is adopted
and content still requires review. Soft checkpoints are instructions, not hook-enforced limits.

DeepSeek edits: 2m06s (bridge timestamps); parent review/smoke roughly 3 min, partly overlapping.
No game content, generation/build/device checks, git operations, hooks or global settings changed.
Only this setup bead was created/claimed; no existing gameplay/planning beads were rewritten.
Storage: 67 GiB free; existing `build/` 107M, profiles 24K; no build lane or scratch tree created.

## jp8.5.16 — four-row inventory bag (cg-gameplay)

Owner released the preserved WIP; cg-gameplay ported it into current source and added the
fresh-B world bag overlay. Root menu stays out of scope (`.8` owns BAG/COLLECTION; `.23` adds SAVE).

Changed: `src/engine/menu/InventoryMenu.hpp` (new), `InventoryMenu.cpp` (new), `MenuIntent.hpp`
(`SelectItem` appended), `draw.h`, `Makefile`, `tst/inventory_menu_test.hpp` (new), `tst/main.cpp`;
plus reported necessary out-of-ownership wiring in `src/engine/world/World.cpp` and
`CreatureGathererFX.ino`. File-static 40 B cache (AVR), not ModeState.

Checks (lane `build/rhs7-lane`): `make test` 240,689/0 (host; +66), world 258/0; `make testvm`
286/0; `fxtest-spike test_items` PASS + `test_stack` PASS, headroom 622 B; `make ram` candidate.
Whole image 23,252 -> 23,980 flash (+728), 1,665 -> 1,704 static (+39), 456 B static free;
~55% of the 1.3 KB stop threshold. Control/candidate ELFs retained under
`build/rhs7-lane/evidence/`. No final gate per gate cadence. Unverified: no device suite draws
the bag page; frame time not measured. Wall time ~15 min. Lane 74 MB; 69 GiB free.

## jp8.11.33 — pocket prop frames (cg-art)

New `images/pocketProps_16x16.png` (80x16, 5 frames: empty plot, sprout, growing, ripe, lure box)
and permanent `tools/tests/pocket-props_test.mjs` (5/5 pass: dims/order, opaque black/white,
distinct stages, increasing ink 31/41/47/70, box distinct). 1x/4x review artifacts under
`build/pocket-props-review/`; no shipping-sized sample committed. Production soft-block/light style
used; no `make gen`, pack, or runtime claim. Integration in jp8.11.30. Wall time ~31m31s.

## jp8.11.8 — four-row collection view and world root menu (cg-gameplay)

Added `CollectionMenu.hpp/.cpp` (39 B AVR cache), the BAG/COLLECTION root menu (SAVE slot
reserved for `.23`), stable-slot four-row collection view, party-slot selection 1..3, transition
scan through `storeRead`, and permanent host/device suites. Reported out-of-ownership touches:
`CreatureGathererFX.ino` render dispatch, `tst/main.cpp`, and the `.16` world-overlay test now enters
BAG through the root menu.

Checks (lane `build/rhs7-lane`): `make test` 240,944/0 (`CollectionMenu 248/248`, `InventoryMenu
73/73`, world 258/258); `fxtest-spike test_collection` PASS 214/0 (18,930 B flash, 1,799/2,160 RAM),
`test_stack` PASS 622 B; `make ram` 25,266 B flash / 1,747 B static (813 free). Deltas vs
23,980/1,704/622 control: **+1,286 B flash**, **+43 B static**, stack unchanged; 3,918 B planning
flash headroom and 413 B static free remain. `test_collection` suite keeps >10 KB free flash.
Caveat: `test_stack` is a separate binary and does not link the new menu, so its call chain is
reasoned/static-bound rather than directly painted; revisit at the integrated `.15` stack proof.
No `make gen`, final gate, commit or output.md edit. Wall time ~14 min; lane 75 MB; 68 GiB free.

## jp8.11.9 — exact-HP box-to-party replacement (cg-gameplay)

Consumed the `.8` `CollectionChoice`: already-in-party no-op, outgoing HP mirrored to its existing
StoreRecord, selected instance's encoded/legacy HP resolved, `storeAssign` published before live
mutation, only the selected Creature rebuilt with exact HP. Added `CollectionFeedback` and
`ALREADY IN PARTY` / `SAVE NEEDED` feedback; tiny `World.cpp` apply call. No Player/Compaction change
was needed.

Checks: `make test` 241,084/0 (new 140), world 258/0; `fxtest-spike test_collection` PASS 296/0,
`test_stack` PASS 622 B; `fxtest-headless test_save` PASS 304/0 (worst frame 5,444 us); `make ram`
25,658 B flash / 1,748 B static. Deltas vs 25,266/1,747/622: **+392 flash / +1 static / 0 stack**;
3,526 B planning flash headroom, 412 B static free. v3 ABI untouched. Unverified: physical hardware;
cut/power-loss recovery host-only (fake flash limit); feedback pixels not reviewed. No final gate,
commit, generated changes, or output.md edit. Wall time ~20 min; lane 77 MB; 69 GiB free.

## jp8.11.23 — explicit SAVE and recoverable failure (cg-gameplay)

Extended the root menu to BAG/COLLECTION/SAVE at x12/y12,28,44 (marker x2); SAVE closes the menu
and calls `SaveController::begin(WORLD)` once. Fixed the Failed soft-lock: after the flash-busy gate,
fresh A re-captures/re-begins and fresh B restores the pre-save state without claiming persistence;
held edges cannot retrigger. No autosave, no new statics/formatter. Updated the permitted .8 root-nav
assertions and added host/device save-flow coverage.

Checks: `make test` 241,290/0; world 258/0; `testvm` 286/0; `fxtest-spike test_save` 474/0 +
`test_stack` 622 B; `test_world_player` 77/0; adjacent `test_collection` 298/0; `make ram` 25,766 B
flash / 1,748 B static. Deltas vs 25,658/1,748/622: **+108 flash / 0 static / 0 stack**; 3,930 B
planning flash headroom, 412 B static free. Device `test_save` 26,568 B (3,128 free), RAM 1,844.
Unverified: `AUDIO=1` Failed-resume idempotence (shipping is audio-off); no final gate/commit/visual
check. Wall time ~40 min; lane 77 MB; 71 GiB free.

## jp8.5.21 — duel consumable healing engine (cg-gameplay)

`MenuIntentKind::SelectItem` now maps to `ActionKind::UseItem` (index = consumable id). On the
committed Choice transition it bounds-checks, reads the battle bag and authored Heal once, refuses
empty/KO/full-HP/Cure/Charge/non-Heal without consuming or advancing, takes exactly one, and applies
the clamped heal before the already-committed opponent action. One resident byte added. No menu/
picker, legacy engine, generated text or save-ABI change.

Checks: `make test` 241,363/0; world 258/0; `testvm` 286/0; `fxtest-spike test_duel_session`
111/0 + `test_stack` PASS 620 B; `test_items` 273/0; `make ram` 25,932 B flash / 1,748 B static.
Deltas vs 25,766/1,748/622: **+166 flash / 0 static / -2 stack**; 3,252 B planning flash headroom,
412 B static free. `test_duel_session` sketch 28,610/29,696 (1,086 B free, above the 1 KB aim).
Unverified: device save/reload for the item (host-proven); picker/e2e route is `.19`. No final gate,
commit or generated change. Wall time ~16 min; lane 77 MB; 71 GiB free.

## jp8.11.28 — authored trainer personas (cg-gameplay)

Replaced the `trainerId % 3` fixture with the authored `DuelData::duelTrainerPersona/Rate` read once
at trainer entry (2 transition reads), stored on the session; wild stays Random/0, `chooseAction`
stays pure, and arena-demo uses its own authored `[demo] reader 45` preset. Two bytes added, absorbed
by padding. Host/device tests pin rows 0/21/22/23 + boss, repeated entry and zero steady-frame reads.
Follow-up trim moved the redundant device item-refusal matrix to the host suite; shipping source and
ELF are unchanged.

Checks: `make test` 241,409+258/0; `testvm` 286/0; `fxtest-spike test_duel_session` 100/0 +
`test_stack` 622 B; `test_duel_session` now 28,160 B (1,536 B free, restored from 232 B);
`test_battle_duel` 47/0; `test_duel_resources` 16/0; `make ram` **25,906 B flash / 1,748 B static**
(−26 flash vs control, 3,278 B planning headroom, 412 B static free). `make verify-generated` remains
blocked by the pre-existing untracked `images/pocketProps_16x16.png`; `.30` adds its fxlayout entry
and regenerates. Arena demo compile-only; no final gate/commit. Wall time ~50 min + ~12 min trim.

## jp8.11.30 — visible plot stages and deposit box (cg-gameplay)

Appended the `pocketProps` image entry to `fxlayout.toml` and branched `drawMapFast` by tile id:
551 uses frame = `plants.plantStages.getStage(FIRST_BETA_PLOT_INDEX)` (0..3), 545 uses frame 4;
all other tiles keep the shared sheet. Added a pure host mapping helper/test and device draws for
each stage/box. No map/TSX/tiles/fxsprites edit; `src/fxdata.h` regenerated by `make gen`.

Checks: `make gen` PASS; `make test` PASS; `fxtest-spike test_world_player` 82/82 + `test_stack`
622 B; `verify-generated` PASS; `test-manifest` PASS; `test-generated-libs` PASS; `test-pack-parity`
PASS after deliberate rebaseline `6af073f9… -> 22270704…`; `make ram` 25,964 B flash / 1,748 B
static. Deltas vs 25,906/1,748/622: **+58 flash / 0 static / 0 stack**; 3,220 B planning headroom,
412 B static free. Packed data +164 B; new cart SHA-256 `2227070492a0…`. `test_world_player` sketch
28,942 B (754 B free) — near-limit but above the hard floor. No final gate/commit/visual pass.
Wall time ~8 min; lane 85 MiB; 73 GiB free.

## jp8.11.13 — fixed boss and persistent ending (cg-gameplay)

Added the faced-coordinate boss trigger at (21,9) from (20,9), a one-member level-8 species-31
trainer row (`ardu,ardu,ardu,8,0,0`; slots 2/3 level 0), the authored persona row, and the ending
line `ardu yields the clearing`. Only Win sets `flag_beta_complete` after terminal feedback; Lose
uses defeat recovery; the completed interaction shows the ending and does not repeat the fight or
reward. Flag ABI: `flag_door=0`, `flag_beta_complete=1`, minted by the inert out-of-area (2,2)
script `if flag_beta_complete then tp 2 2 2 2 endif;`; no hardcoded runtime index.

Checks: `make gen` PASS; host 241,446/0 + world 258/0; `testvm` 286/0; `verify-generated` PASS;
`test-pack-parity` PASS (`3ff85462…`); `fxtest-spike test_duel_session` 15/0 + `test_stack` 622 B;
`fxtest-spike test_world_player` 60/0 + `test_stack` 622 B; `make ram` **26,532 B flash / 1,748 B
static**. Deltas vs 25,964/1,748/622: **+568 flash / 0 static / 0 stack**; 2,652 B planning flash
headroom, 412 B static free. Follow-up trimmed both device suites: `test_world_player` 7,684 B free
and now includes the device boss trigger/ending case; `test_duel_session` 13,540 B free and keeps
one-member/persona/Win/Lose/flag proof. No final gate/commit/visual pass. Wall time ~30 min + ~25 min
trim; lane 88 MiB; 73 GiB free.

## jp8.11.31 — visible GATHER cue (cg-gameplay)

Added `duel::gatherCue(state)` and a centre-strip `gather` label at x48/y41 when
`gatherable && !trainer`; the R#/type content is replaced only in that case. No 7th pick-row cell;
attack/SWAP/WAIT semantics and the B action page selector are unchanged. Host proves the cue iff
gatherable and the six-cell worst case; the device suite proves cue pixels, y47 reserved, B-page
Gather commit and progress=1. No flag/popup/per-frame read.

Checks: `make test` 241,478/0 + world 258/0; `fxtest-spike test_duel_ui` 85/0, 25,404 B (4,292 free),
`test_stack` 622 B; `test_battle_duel` 47/0; `make ram` 26,554 B flash / 1,748 B static. Deltas vs
26,532/1,748/622: **+22 flash / 0 static / 0 stack**; 2,630 B planning headroom, 412 B static free.
Unverified: final gate and non-required device suites. Wall time ~24 min; lane 89 MB; 73 GiB free.

## jp8.11.32 — gather progress track (cg-gameplay)

Added `duel::gatherTrackWidth` (uint16, no float/FX) and a 32x1 white fill at (48,47) in the
GATHER-cue branch: width = round(32*progress/need), omitted at zero and for trainers/ordinary wilds.
Resident `BattleState::gather` only; no text/flags/state/per-frame reads. Host fraction cases and
device framebuffer cases cover 0/half/full/ordinary/trainer/fresh and the y48 pick row.

Checks: `make test` 241,488/0 + world 258/0; `fxtest-spike test_duel_ui` 95/0 + `test_stack` 622 B;
`make ram` 26,632 B flash / 1,748 B static. Deltas vs 26,554/1,748/622: **+78 flash / 0 static /
0 stack**; 2,552 B planning headroom, 412 B static free; `test_duel_ui` 3,334 B free. No final
gate/commit/visual pass. Wall time ~6 min; lane 89 MB; 73 GiB free.

## jp8.5.19 — battle Heal picker (cg-gameplay)

B action page is now GATHER (gatherable wild) -> ITEMS (always) -> ESCAPE (wild); trainer B opens
ITEMS. The picker reuses the cached four-row bag with a Heal-only mask, open/scroll resolves names and
metadata, steady draw/select uses the cache, and the semantic consumable id goes through `SelectItem`.
No 7th pick-row cell, no ModeState growth, no save-buffer aliasing. The action label reuses the
authored "item" string; plural "ITEMS" would be a separate text/gen follow-up.

Checks: `make test` 241,541/0 + world 258/0; `testvm` 286/0; `fxtest-spike test_duel_session` 15/0 +
`test_stack` 622 B; `test_duel_ui` 104/0 (1,054 B free); `test_items` 273/0; `make ram` 27,430 B
flash / 1,790 B static. Deltas vs 26,632/1,748/622: **+798 flash / +42 static / 0 stack**; 1,754 B
planning flash headroom, 370 B static free. Unverified: device trainer picker reachability (host),
device item save/reload (host), four-row scroll on real data (only 3 Heal ids; host fake). No final
gate/commit/visual pass. Wall time ~40 min; lane 90 MB; 73 GiB free.

## jp8.11.15 — ordinary first-area loop proof (cg-gameplay)

Added permanent native `beta_area_flow_test.hpp` and AVR `test_beta_area.ino`/`beta_area_test.hpp`.
The native path boots ordinary fresh content and drives real world/picker/battle/menu edges through
rejected deposit, plant growth/harvest, deposit, world-triggered gatherable capture with opponent HP
cost, duplicate boxed capture, rest, 14-win exact XP/L8 evolution, full-64 refusal, full-bag hold,
InParty refusal + boxed replacement, boss lose/recovery/retry/win/ending, explicit save/reload of bag/
plants/collection/partyHP/XP/flag, interrupted save and retry. The device path covers ordinary boot,
empty-deposit refusal, authored growth/harvest, deposit, real packed-zone capture/HP cost, rest,
boss win/ending and two v3 checkpoints. No shipping-source change.

Checks: `make test` 241,768/0 + world 258/0; `testvm` 286/0; `fxtest-spike test_beta_area` 64/0 +
`test_stack` 622 B; `verify-generated` PASS; `make ram` unchanged **27,430 B flash / 1,790 B static**
(2,266 B hard free, 370 B static free). Device suite 28,230 B (1,466 B free) / 2,023/2,160 static
(137 B under cap; thin). Measured trace: 299 steps, 15 wins (14 ladder + final), 2 captures, 2 boss
attempts, evolution L8, 205 s simulated update time; 15–25 min remains unadopted. Caveats: device
walk uses a synthetic authored-pocket property window (real packed walk in other suites); exact XP/
full64/full-bag/interrupted save/boss lose-retry are native + existing device suites. No final gate/
commit. Wall time ~2 h 15 m; lane 91 MB; 73 GiB free.

## jp8.2.7 — post-integration save interruption/endurance (cg-gameplay)

Extended the host/device save suites to the named matrix: 13 observable publication boundaries (0,
4096, 4144, 4192, 4608, 4704, 4710, 4718, 4719, 4720, 4730, 8816, 8817), complete 96-byte snapshot +
all 64 records after each reboot, full 256-record journal + refused dirty tail + explicit save
without append, generation 0xFFFF→0 endurance, and the jp8.11.22 HP authority cuts (encoded/legacy,
PartyAssign, mirror+reassign, evolution, KO no-heal, inventory+party together). `docs/m1-storage.md`
records the matrix. No save-source fix.

Checks: `make test` 243,968/0 + world 258/0; `testvm` 286/0; `fxtest-spike test_save` 492/0 +
`test_stack` 622 B; `test_save_cuts` 4,786/0; `verify-generated` PASS; `make ram` unchanged
**27,430 B flash / 1,790 B static**. Device `test_save` 27,380 B (2,316 free), 1,852/2,160 static;
`test_save_cuts` 1,941/2,160 (219 free, thin) — noted. No physical power-loss run (write-limit seams).
No final gate/commit. Wall time ~60 min; lane 92 MB; 73 GiB free.

## jp8.4.10 — transition-only menu metadata reads (cg-gameplay)

Added `transitionCurrent()` to `FxReadCounter` and declared the actual transition lookups in the
currently un-declared menus (`InventoryMenu`, `CollectionMenu`, `LureZone`); in-window moves no
longer re-resolve identical rows. New `test_world_bag` + `menurun_test` device matrix covers battle
pick/action/Heal/party pages, bag open/scroll, collection root/list/party/swap, lure picker, dialog
push/pop, and arena in its enabled suites. Metadata reads occur on transitions and are zero on steady
frames; sprite/text rasters are excluded.

Checks: `make test` 258/0; `fxtest-spike test_world_bag` 113/0 + `test_stack` 622 B;
`test_items` 334/0; extra `test_readcounter` 22, `test_arenaview` 70, `test_arenacatalog` 46;
`make ram` **27,376 B flash / 1,790 B static** (control 27,430; −54 flash, 0 static). Device
`test_world_bag` 26,966 B (2,730 free), static 1,985/2,160 (175 free); `test_items` 16,784 B.
Unverified: forced-replacement UI route device-driven (shares the proven beginParty transition);
Event dialog pop; no per-open/scroll negative control. No final gate/commit. Wall time ~1 h 45 m.

## b2a.6 — first-beta label/width/position table (cg-gameplay)

Frozen and pixel-tested the beta label table: battle pick/action pages, plural `items` label
(30 px), party picker, bag rows, lure picker/empty/footer, ending/lure messages
(`ardu yields the clearing` 125 px), save status, collection root/list/party, feedback rows and duel
footer. Fixed a real `earth` label clip (4→5 glyphs) and made selected cells invert per the frozen
table; no general text abstraction or new name cache.

Checks: `make gen` PASS; `verify-generated` PASS; `test-generated-libs` PASS; `test-pack-parity` PASS
after deliberate rebaseline (`3ff85462`→`847beef8`, append-only +63 B); `make test` 243,968 + 258/0;
new `battle48-screen_test.mjs` 7/7; `fxtest-spike test_battlepresentation` 102/0 + `test_stack` 622 B;
extra `test_items` 334/0, `test_collection` 298/0. Shipping **27,340 B flash / 1,782 B static**
(−36/−8 vs control), 378 B static free; presentation suite 13,494 B free. Unverified: node test not
wired into a Make target; CollectionMenu polarity is an adjacent review touch; no visual/final gate.
Wall time ~1 h 50 m; lane 98 MiB; 73 GiB free.

## b1ua — duel resource fixture restore (cg-gameplay, test-only)

Added `tst/fxdatatest/duel_session_fixture.hpp` with the minimal shared fixtures removed by the .13
trim; re-pointed `duel_resource_test.hpp` and the identically affected `win_exp_test.hpp`. No
shipping/generated change. Host 258/0; `test_duel_resources` 16/0 + `test_stack` 622 B; boss/world
suites unchanged (12,900/7,650 B free); all 36 device suites now compile. Full run remains red only
on the separate `test_scripts` fixture (`ud5q`), not this dependency. Wall time ~14 min.

## ud5q — test_scripts fixture for the boss-complete clause (cg-gameplay, test-only)

Updated `tst/fxdatatest/scripts_test.hpp` to the current 31-byte packed slot and re-pointed End to
slot[30]/If at slot[15], retaining semantic opcode/coordinate/flag execution checks. Full
`make fxtest-headless` now **39/39 suites PASS**; `test_scripts` 91/0 + `test_stack` 622 B;
`make test` 258/0; shipping unchanged **27,340 B flash / 1,782 B static / 622 B stack**. Wall time
~6 min; lane 118 MiB; 73 GiB free.

## jp8.11.16 — settled final gate (producer/device lane)

`make final-gate BUILD_DIR=build/rhs7-lane FINAL_GATE_LOG_DIR=build/reports/rhs7/final-gate-jp8-11-16`
→ **PASS**. All 39 device suites PASS (including beta_area 64/0, duel_session 15/0, duel_ui 104/0,
collection 298/0, save 492/0, save_cuts 4,786/0, scripts 91/0, world_bag 113/0, world_player 60/0);
`test_stack` 622 B. Shipping: **27,340 B flash / 1,782 B static / 778 B link free** (2,356 B hard
flash free, 378 B static budget free), whole soundtrack off. Logs:
`build/reports/rhs7/final-gate-jp8-11-16/{check,ram}.log`.
The gate first caught two test-only regressions from the `.13` trim: `duel_resource`/`win_exp`
fixtures (`b1ua`, fixed with `duel_session_fixture.hpp`) and the `test_scripts` packed-slot fixture
(`ud5q`, fixed). Both are green. Only `jp8.11.29` remains open and blocks `jp8.11.16`; the installed
`cgfx-tools 0.3.0+g55501d10` is accepted by `make doctor`, but `tools/toolchain.lock` still pins
`v0.2.0` and no v0.3.0 release assets exist locally. No git commit/push performed.

## jp8.11.29 — release decision (owner)

Owner selected: publish Tools **v0.3.0** and pin that release (not a source-commit pin or local-only
acceptance). Owner action: tag/push `v0.3.0` from the selected clean CreatureGathererTools source
(Cargo version 0.3.0); the release workflow publishes `cgfx-tools-macos-arm64`,
`cgfx-tools-linux-x64` and their `.sha256` files. After the assets exist, cg-tooling updates
`tools/toolchain.lock`, CI/download paths, Ardens/default docs and the stale game-design-spec status,
runs the clean-source generation/parity/spec checks, then `jp8.11.16` can close on the existing green
gate evidence plus released-generator reproducibility. No git/remote operation performed by the
producer.

## j8l6 — review and evidence-based acceptance workflow (producer, docs/config only)

Owner-directed workflow change in local role prompts/docs. No firmware, test, Makefile, generated
artifact, build, package, service, global-config, or git operation.

### Changed paths
- `.opencode/agents/cg-reviewer.md` (new) — `subagent`, DeepSeek, denies edit/shell/delegation;
  read-only acceptance review; APPROVED / CHANGES_REQUIRED / INCOMPLETE verdicts; host overlays do
  not grant mutation.
- `.opencode/agents/cg-producer.md` — acceptance fixed before dispatch (outcome/path, scope,
  evidence, substitutions, stack/whole-image coverage, integration checkpoint); fresh-reviewer flow,
  repairs, evidence-matched signoff, no self-close.
- `.opencode/agents/cg-{world,story,art,gameplay,tooling}.md` — evidence order, exercise scope,
  standard-target wiring for new permanent tests, ready-for-review.
- `docs/agent-roles.md` — seven profiles, reviewer role/launch, REVIEW phase, acceptance and
  repair/re-review flow, test wiring, wall-vs-active timing.
- `AGENTS.md` — project-roles pointer only.
- `output.md` — this section.

### Validation
- `Bun.YAML.parse` + `JSON.parse` (Bun 1.4.2): PASS. 7 profiles, all
  `opencode-go/deepseek-v4.1-flash`; producer `primary` with exactly world/story/art/gameplay/
  tooling/reviewer allowed; specialists `all`; reviewer `subagent` with edit/shell/subagent denied;
  root `opencode.json` model/default/instructions unchanged.
- Initial docs/profile checks passed. Parent acceptance subsequently found and repaired the
  producer-deny summary, unsupported direct reviewer launch hint, task-specific language in the
  reusable producer prompt, and an overstated host-permission claim in the AGENTS pointer.
- Live producer review session `ses_e1b0d7b5fffeFOLH8yXhIaxTvI` dispatched exactly one fresh
  `cg-reviewer` child `ses_e1b0d43c2ffejOeaWI4VGE5kfB`. Final focused review returned
  `REVIEW: APPROVED` after those repairs; no reviewer edits, queue changes or builds.
- Final SHA-256 snapshots checked against the initial validated files; only the three repaired
  documents changed, and their final revisions were reviewed. Reviewer discovery/dispatch is
  verified; runtime isolation is not claimed, because host/session overlays can expose tools.
- Initial DeepSeek implementation wall time 2m09s; focused worker repair 12.5s; final reviewer
  follow-up 8.2s (bridge timestamps). Active execution time unmeasured; no inference from laptop
  suspension or wall-time gaps. No game/source/firmware checks were applicable or run.
- Storage: 73 GiB free, existing build root 229M, role profiles 32K. No build lane,
  scratch tree or cleanup needed. No hooks, global settings or existing gameplay Beads changed.

### Review correction
`cg-reviewer` found the gate log authoritative at **36** device `.ino` suites (not 39); the earlier
`ud5q` report counted suite labels/profiles. The b2a.6 action label is the authored lowercase
`items`, not `item`. Both are report corrections only; the gate remains PASS.

## 0oi1 — review repair: ordinary Heal acquisition + label gate (cg-gameplay)

Blocking review findings repaired. `BattleFlow.cpp` now grants the authored `Rewards::healItem/
healCount` entries once on the ordinary non-gatherable wild-Win terminal edge through the real bag,
saturating at 99; entry count derives from the packed `healCount - healItem` span and is pinned to
`RewardFixture::healRewardCount`. No new flag/event/kind/popup; trainer/boss/gatherable/Lose/Draw
grant nothing. The ordinary fresh flow now acquires a Heal from a normal win and spends it in a later
duel; the device beta suite mirrors the acquisition/use. `Makefile` now runs
`battle48-screen_test.mjs` and `pocket-props_test.mjs` from `test-beta-area`, so `make check` enforces
the b2a.6 label table.

Checks: `make test` 244,009/0 + world 258/0; `make test-beta-area` node 38/0; `fxtest-spike
test_beta_area` 70/0 + `test_stack` 622 B; `test_duel_session` 15/0; `make check` exit 0 (43/43 device
suites per its output); `make ram` **27,520 B flash / 1,782 B static**. Delta +180 flash / 0 static;
1,664 B planning headroom, 378 B static free. `test_beta_area` 28,872 B (824 B free), static 137 B
free; `test_stack` does not paint the grant frame (small terminal-edge loop; headroom 622 B).

### 0oi1 re-review
`cg-reviewer` re-review: **APPROVED**, no blocking findings. Ordinary heal acquisition is real-bag
and terminal-edge scoped; the label screen test runs from `test-beta-area` in `make check`. Remaining
nonblocking notes: the `make check` summary count is 36 device `.ino` suites (not 43; node 38/0),
`test_beta_area` is 824 B flash / 137 B static free, `test_stack` does not paint the terminal grant
frame, and `awardWildKoHeal`'s terminal FX reads are undeclared like the pre-existing material/exp
reads. The host fixture retarget at the final win is inert for heals; the 14-win ladder + device
suite carry the acquisition proof.

## jp8.11.29 — release attempt blocked by runner image

Owner pinned v0.3.0 to `55501d10`. cg-tooling verified the commit (0.3.0), created/pushed
`refs/tags/v0.3.0 = 55501d10` without touching the dirty Tools WIP, and triggered the release.
The Linux job succeeded and retained its artifact; the macOS job failed at zero steps with
`The configuration 'macos-14-arm64-us-default' is not supported` on both attempts, so the release
job was skipped and `gh release view v0.3.0` reports **not found**. No assets/checksums, no FX lock
change. Run: https://github.com/FreezingSnail/CreatureGathererTools/actions/runs/43934573058.
The installed local binary still matches `0.3.0+g55501d10` (`5f46d0ed…`) but is not a release asset.
`.29` remains open; `.16` remains blocked. No FX files changed.

## jp8.11.29 — release published, but 55501d10 cannot reproduce the repo

The runner fix worked: v0.3.0 is published at `82dd0900` (55501d10 + macos-15/checkout-before-publish
commits), assets/checksums present, macOS asset `0.3.0+g82dd0900` verified. But clean 55501d10
predates Tools `421f85c` (atk-less generator model), while FX `ceecdfe` already removed `atk` from
`creatures.json`; `make gen`/`make pack` with the release fail on `missing field atk`. The accepted
manifest identity `0.3.0+g55501d10` therefore came from a **dirty** 55501d10 build and is not
reproducible from a clean commit. cg-tooling reverted the provisional lock/CI pin and kept only the
verified spec/README/Ardens doc updates; artifacts remain `847beef8…` / `0.3.0+g55501d10`. `.29`/`.16`
remain open. Recommendation: release v0.3.0 from clean HEAD `67b2036` with the verified workflow
fixes, then repin and regenerate/rebaseline.

## jp8.11.29 — clean reproducible v0.3.0 and FX pin

Published `v0.3.0` from clean `67b2036` + workflow fixes at commit `fe328be2`:
https://github.com/FreezingSnail/CreatureGathererTools/releases/tag/v0.3.0
Assets: macOS-arm64 `fe72d827…`, Linux-x64 `13ce64e6…` with sidecars. The release binary verifies as
`cgfx-tools 0.3.0+gfe328be2`; `tools/toolchain.lock` pins v0.3.0, CI download/validation and
Ardens/spec docs are updated. With the release binary on PATH: `make doctor` READY, `make gen`
deterministic (`24948525…` cart), `verify-generated`/`test-manifest`/`test-generated-libs` PASS,
pack parity rebaselined `847beef8… -> 24948525…`, `make test-spec` in sync. Caveats: CI workflows are
YAML-validated but not executed on GitHub; the Linux asset checksum was verified but not run on this
host. The local test binary is kept under `build/rhs7-lane/tools/release-v0.3.0-bin/cgfx-tools`.

## jp8.11.16 — release gate PASS on pinned v0.3.0

Final gate on the pinned release binary (`build/rhs7-lane/tools/release-v0.3.0-bin/cgfx-tools`,
`0.3.0+gfe328be2`) → **PASS**: `build/reports/rhs7/final-gate-jp8-11-16-v030/{check,ram}.log`;
all device `.ino` suites PASS, `test_stack` 622 B; shipping **27,520 B flash / 1,782 B static**
(2,176 B hard flash free, 378 B static-budget free). Reproducible release `v0.3.0` at `fe328be2`,
lock/CI/docs/spec pinned; gameplay slice review APPROVED; save endurance, labels and read-counter
acceptance green. The earlier 30-minute termination was the producer shell timeout at `test_version`,
not a failure; the warm rerun completed. No git commit/push.
