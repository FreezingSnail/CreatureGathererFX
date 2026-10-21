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
