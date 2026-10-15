# battle-legacy — durable source archive of the pre-duel battle system

Source-only reference copy of the **original 8-type Pokémon/SMT-style battle system**
(engine, AI, session/setup, menus, rendering/presentation, arena demo, host simulator,
permanent tests and authored data/build inputs). Captured before the
`CGFX_DUEL_BATTLE` spike begins replacing that machinery, per
`docs/duel-system-spike-plan.md` §Experiment 1 ("Before the first replacement of
existing battle code, preserve a source-only reference under `archive/battle-legacy/`").

This archive is a **reference deliverable**, not build input. It lives outside `src/`
and outside ordinary test discovery and must not enter any shipping or test build.

## Provenance

- **Repo HEAD at capture:** `4399e4fc2482c255a286242ee580569c1a11923a`
- **Capture date (UTC):** 2026-10-15
- **Captured content:** the *working tree* at capture time, not the committed blob for
  `HEAD`. Provenance was read with read-only `git status --short` and `git rev-parse
  HEAD`; no branch, history, index or working-tree mutation was performed. Bytes were
  copied with `cp` and hashed with `shasum`.
- **Archived files that were modified in the working tree relative to `HEAD`:** the
  dirty set at capture included `src/engine/battle/BattleFlow.cpp` and
  `tst/battle_flow_test.hpp`, both of which are inside this archive's scope; their
  archived copies are the modified working-tree versions. The recorded status did
  not identify other modified tracked files in the initial battle-only selection;
  the checksum inventory is the authority for the bytes actually retained.
- **Uncommitted working-tree files at capture (repo-relative, from `git status`):**
  `.beads/interactions.jsonl`, `AGENTS.md`, `src/engine/battle/BattleFlow.cpp`,
  `src/engine/world/World.cpp`, `src/engine/world/World.hpp`, `src/player/Player.cpp`,
  `src/player/Player.hpp`, `src/save/Compaction.cpp`, `src/save/Compaction.hpp`,
  `src/save/SaveController.cpp`, `src/save/StoreRecord.hpp`, `tst/battle_flow_test.hpp`,
  `tst/fxdatatest/save_test.hpp`, `tst/fxdatatest/test_world_player.ino`,
  `tst/save_test.hpp`, `tst/world_interact_test.hpp`, plus untracked
  `docs/duel-system-spike-plan.md`. Two of these are inside this archive's scope:
  `src/engine/battle/BattleFlow.cpp` and `tst/battle_flow_test.hpp`; their archived
  copies are the modified working-tree versions.
- **Shared prerequisite supplement:** on 2026-10-15 the orchestrator also retained
  the current World, Player, save and ModeState sources and related save/world tests
  listed in the inventory supplement. This preserves the known dirty shared
  dependencies instead of assuming a checkout of the recorded HEAD supplies them.
  These supplemental files were captured after the battle archive; they are not
  claimed to be a verified snapshot of every file at the initial capture instant.
- **File/hash inventory:** see `INVENTORY.txt` (SHA-256 + byte size for retained
  files, including this README, relative to this directory; the inventory excludes itself).

## Tool versions (from `make doctor` at capture)

- host: Darwin arm64
- host C++: Apple clang 21.0.0 (`/usr/bin/c++`)
- make: GNU Make 3.81
- arduino-cli: 1.2.0 (Homebrew)
- Arduboy cores and libraries: present
- cgfx-tools: 0.3.0+ga9dc6400 (`~/Applications/CreatureGathererTools/bin/cgfx-tools`)
  — identity matches manifest generator
- Ardens (for device suites): `/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens`
  (must be `captureserial`-capable; the installed 0.24.4 graphical bundle is not)

## What is archived (original relative paths preserved)

| Path | Contents |
| --- | --- |
| `src/engine/battle/` | Battle engine: `Resolve`, `Damage`, `Effects`, `Ai`, `MoveUses`, `ActionResult`, `BattleSession`, `BattleSetup`, `BattleState`, `BattleTypes`, `BattleView`, `BattleRng`, `BattleMenu`, `BattleFeedback`, `BattlePresenter`, `BattleScene`, `BattleLayout`, `BattleNameCache`, `BattlePresets`, `BattleFlow` |
| `src/engine/arena/` | Arena demo/catalog/navigation/view that drive the battle system |
| `tools/battle-sim/` | Native host batch simulator (`BatchRunner`, `ScenarioBuilder`, `SwitchPolicy`, `FixtureData`, `SimulatorReports`, `main`) |
| `tst/` | Host battle/arena suites (`battle_*_test.hpp`, `arena_*_test.hpp`) |
| `tst/fxdatatest/` | Device battle/arena suites (`battle*_test.hpp`, `arena*_test.hpp`, `test_battle*.ino`, `test_arena*.ino`, `test_battletrainer.ino`, `test_trainer_setup.ino`, `trainer_setup_test.hpp`, `type_chart_fixture.hpp`, `battlepresentation_fixture.hpp`) |
| `data/` | Authored battle inputs: `battle-presets.toml`, `arena-demo.toml`, `arena.csv`, `movesheet.csv`, `movelists.csv`, `Creaturesheet.csv`, `typetable.csv`, `opponents.csv`, `EncounterTables.csv`, `consumables.csv`, `beta-area.toml`, `json/{moves,creatures,battle_sprite_orientation}.json`, `text/strings.txt`, `CreatureGatherer Stat Sheet.xlsx` |
| `fxdata/data/` | Authored FX-layout declaration sources the battle reads (`menu.txt`, `moves.txt`, `movelists.txt`, `creatureData.txt`, `opponents.txt`, `teams.txt`, `encounters.txt`, `mapData.txt`, `chunks.txt`, `event.txt`) |
| `build/` | Build prerequisites: `Makefile.snapshot` (Makefile at capture), `fxlayout.toml`, `fxsprites.toml` |
| `docs/` | Battle design/audit evidence: `battle-flash-audit.{md,csv}`, `battle-simulator.md`, `battle-action-contract.md`, `battle-balance-recommendations.md`, `battle-presentation-research.md`, `battle-wheel-matrix.md`, `slim-battle-spike-design.md`, several `battle-*-spike` notes, the `battle-presentation-layouts.svg`, the balance HTML dashboards, and the duel brief/plan |
| Shared prerequisite supplement | `src/engine/world/World.*`, `src/player/Player.*`, `src/save/{Compaction.*,SaveController.*,StoreRecord.hpp}`, `src/engine/ModeState.*` and related save/world tests; see inventory for exact files |

### Dependency references (shared — not duplicated here)

These are pulled in by the archived sources but belong to the wider project and remain
in place. Start with the pinned revision above, then overlay every retained source
and shared prerequisite supplement at its original relative path. Dependencies
that were not captured are references rather than a self-contained snapshot:

- `src/lib/Type.hpp`, `Move.hpp`, `Stats.hpp`, `StatModifier.hpp`, `StatusEffect.hpp`,
  `Effect.hpp`, `uint24.h` — type/move/stats contracts
- `src/engine/menu/MenuIntent.hpp` — the menu intent type
- `src/GameState.hpp`, `src/globals.hpp`, `src/values.hpp` —
  resident state and HP ownership
- World-engine sources other than the retained `World.*`, including `StepEvent.cpp`
- `src/fxdata.h`, `src/flags/*`, `src/vm/opcodes.hpp` — generated headers (never copied)
- `images/`, `fxdata/generated/`, `dist/` — binary art and packed images are referenced,
  not duplicated (no binary/asset trees in this archive)
- `mock/` — the forward-looking duel browser reference. It is **not** legacy and is
  deliberately excluded from this archive.

## Reproduction / verification commands

The archive is not wired into a build, and restoration commands have not been
executed against a reconstructed checkout. To reproduce the reference, overlay
the archived source/data and shared prerequisite supplement onto a checkout of
`4399e4fc…`, restore `build/Makefile.snapshot` as the root Makefile and the two
archived layout/config files to their original root paths, then use the pinned
tooling. Regenerate through `make gen` before these verification commands:

```sh
# tool readiness
make doctor

# host suites (includes every archived tst/battle_*_test.hpp and arena suite)
make test

# host simulator
make sim
make sim-test

# full ScriptVM host suites
make testvm

# selected device battle suites through a captureserial-capable Ardens
make fxtest-headless ARDENS=/path/to/Ardens \
  FXTEST_INOS='tst/fxdatatest/test_battlesession.ino tst/fxdatatest/test_battlepresentation.ino tst/fxdatatest/test_battle_slim.ino'
```

## Known limitations of this archive

- **Source-only.** No compiled outputs, no `.hex`/`.elf`, no packed image, no build cache,
  no generated headers, no binary art. It cannot be flashed or built as-is; it reproduces
  the *sources and data* of the legacy battle, not a working binary.
- **Working-tree sources.** The initial selection includes dirty `BattleFlow.cpp`
  and `battle_flow_test.hpp`; the later supplement includes dirty shared sources.
  Their retained bytes and capture boundaries are documented above. No claim is
  made that a clean checkout alone reproduces this working tree.
- **Generated artifacts excluded by design.** `src/fxdata.h`, `src/flags/*`,
  `src/vm/opcodes.hpp`, `fxdata/generated/*`, `tst/fxdatatest/generated/*`, `dist/*`,
  `fxdata/Sprites.txt` are generated and intentionally omitted; regenerate them with
  `make gen` at the pinned revision.
- **Shared dependencies are referenced, not copied.** Reproducing a build requires the
  live tree (or a checkout at the pinned revision), not this directory alone.
- **No test-discovery wiring.** Files sit under `archive/`, which no Makefile target or
  Arduino sketch build scans; `tst/` and `tst/fxdatatest/` discovery here are inert.
- **Provenance is textual.** Recorded revision/hashes are bytes captured at this date;
  they are not a substitute for `git` history. Only read-only `git status`/`git
  rev-parse` were used to record provenance; no repository write occurred.

## Handoff

This archive is the step-1 durable deliverable required before the duel spike may replace
any legacy battle source. Step 3 (`CreatureGathererFX-d73i.3`) verifies and completes it
against the original control and confirms no archived translation unit enters a shipping
build. Keep it during build cleanup; it is not disposable spike output.
