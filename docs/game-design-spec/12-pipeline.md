# 12. Content pipeline, tooling and testing

## Content pipeline

All content flows through one native generator (`cgfx-tools`, Rust) driven by `make gen`:

```
data/json/creatures.json ─┐
maps/world_map.json ──────┤
data/*.toml contracts ────┤   cgfx-tools --project cgfx-project.json
data/*.csv tables ────────┼─► fxdata/generated/*  (headers, tables, fixtures)
images/*.png, text ───────┘        │
                                   ├─ copies: opcodes.hpp → src/vm, flags/* → src/flags
                                   ├─ fixtures → tst/fxdatatest/generated
                                   └─ pack (fxlayout.toml) → src/fxdata.h + dist/fxdata*.bin
```

- `fxlayout.toml` is an ordered flash layout (30 packed entries + 8 save sectors); declaration order is permanent ABI. Save offsets are sector-aligned and stable.
- Sprites/fonts regenerate natively from `fxsprites.toml`; the string table renders into a packed glyph sheet.
- `make verify-generated` checks the manifest (inputs, outputs, hashes) without mutating; `test-pack-parity` pins the packed image SHA-256; `test-manifest` / `test-generated-libs` prove the packed bytes match the published header.
- Generated files are never hand-edited; change the JSON/CSV/TOML/PNG source and regenerate, staging the generated set as one commit.

## Key make targets

| Target | Purpose |
|---|---|
| `make gen` | Full generation + pack (data, sprites/fonts, fixtures, image) |
| `make test` / `testvm` | Host C++ suites (~240.5k assertions) and ScriptVM suites (286) |
| `make test-beta-area` | Authored first-area contract vs generated data (node) |
| `make fxtest-headless ARDENS=…` | All 34 device suites through headless Ardens serial, exact `P`/`F` markers |
| `make fxtest-spike FXTEST_SPIKE_INO=…` | Touched device suite + `test_stack` headroom check |
| `make ram` / `build` / `mini` | Whole-image flash/RAM report (budget enforced) / shipping build / Mini build |
| `make final-gate ARDENS=…` | Full check + RAM report, logged under `build/final-gate/` |
| `make spec` / `test-spec` | Regenerate this document from `docs/game-design-spec/*.md` / verify it is in sync |
| `make arena-demo` / `arena-package` | Launch arena demo / build the distributable `.arduboy` zip |
| `make clean-build-artifacts [CLEAN_APPLY=1]` | Preview/apply cleanup of disposable build staging |

Lane and storage rules: one AVR/generation lane at a time; one shared `BUILD_DIR` per session; 5 GiB build + 5 GiB scratch budgets with a 10 GiB free reserve; clean each measured item after preserving evidence.

## Build variants and demos

| Variant | Flag(s) | What it does |
|---|---|---|
| Shipping FX | default | Ordinary game: authored beta start, save load/write through the test harness |
| Trainer demo | `CGFX_TRAINER_DEMO` (+ `_SWITCH_DRILL`, `_UTILITY`) | Boots straight into a 3v3 trainer duel from a named preset; no save load/write; A/B navigation; trainer rules refuse GATHER/ESCAPE |
| Wild demo | `CGFX_WILD_DEMO` | Overworld entry + one forced ordinary wild encounter; exercises the real step → encounter → session path |
| Arena demo | `CGFX_ARENA_DEMO` | Boots a random 3v3 duel (3 player teams × 5 opponent teams) and continuously re-rolls; menus are the pre-compact arena screens; distributable via `arena-package` |
| Expansion battle test | `BATTLE_TEST_EXPANSION=1` | Uses species 32–63 for player/trainer parties in an isolated build |
| Lure demo | `make lure-demo` | Alternate scenery in the lure clearing for art review |

## Testing and evidence rules

- **Host:** C++ suites exercise kernel rules, session/flow wiring, save/journal/compaction, world stepping, encounters, menu navigation, arena lifecycle. Latest counts: ~240.5k passed / 0 failed, world suite 258 / 0, VM 286 / 0.
- **Device:** 34 headless Ardens suites run the real firmware paths through serial, accepting only an exact `P`/`F` marker. Highlights: duel kernel relations and damage, real-session timing (29/68 frames), UI geometry/captions, painted stack headroom, save cuts/journal/flag extents, table read paths, script VM, FX read counter.
- **FX read rule:** program-data reads occur on transitions only. The counter resets per frame; steady-state update must add zero reads, render is checked exactly, and transition owners declare their exact reads.
- **Determinism:** RNG is seeded from hardware entropy on boot and injectable in tests; the duel kernel has a C++/JS parity check (`make sim-test`) against the canonical mock.
- **Evidence:** every resource-changing item keeps a same-source control A/B, the touched device suites, and a compact report (commands, numbers, wall time, storage).
