# Device test-suite speed review — implementation-worker loop

2026-10-16. Scope: wall time of the verification loop implementation workers run (`make test`,
`make testvm`, focused `fxtest-headless`, `make check`, `make final-gate`). No source behavior is
changed by this review; every number below was measured on the current tree with warm caches
unless noted. Reproduction commands are included per section.

Environment: macOS arm64, 11 cores; `arduino-cli` 1.2.0; AVR-GCC 7.3.0 (x86_64 binary running
under Rosetta 2); `cgfx-tools` installed; 34 device suites (33 `test_*.ino` plus the legacy
`fxdatatest.ino` opponents suite).

## Headline

- Host and VM suites are already fast: `make test` 3.1 s (258 pass, 0 fail), `make testvm` 1.2 s.
- Every gate is dominated by `fxtest-build`: each of the 34 suites is rebuilt from scratch in a
  fresh stage (`rm -rf` + full ~60-TU AVR compile + LTO link), **~9.7 s per suite ≈ 5:30** per
  wildcard build, plus ~0.4–1.5 s per suite of serial capture (≈ 35 s total). Prior sessions
  logged the full wildcard at **7:41** with a **5:10** build stage.
- Root cause: identical work repeated per suite. The ~60 `src/` translation units are
  byte-identical across suites within a flag profile, but each suite gets a fresh build path and
  recompiles all of them. The LTO link itself is only ~0.65 s and the sketch TU ~0.28 s.
- Proven fix: **one persistent stage per flag profile, swap the suite `.ino` content.** First
  compile per profile ~9.7 s; every subsequent suite **1.4–2.3 s** (sketch TU + link). Expected
  wildcard build **5:30 → ~1:20 serial** (4 profiles; ~50 s if the 4 profile builds run in
  parallel). A worker's focused rebuild after a source edit drops from ~10 s to ~2 s per suite.
- Storage win: 34 stage trees (~4.6 MB each; `build/fxtest` was 212–231 MB) → 4 profile trees
  (~5 MB each) plus small per-suite `output/` copies.

## Measured budget (current tree, warm)

| step | wall | notes |
|---|---:|---|
| `make test` | 3.1 s | 258 host tests, 0 fail |
| `make testvm` | 1.2 s | VM suites |
| `make gen` | 6.4 s | `cgfx-tools`, empty tracked diff |
| `make build` (warm) | 3.7 s | shipping FX; arduino-cli reuse |
| `make verify-generated` | 3.0 s | manifest + image |
| `make test-beta-area` | 7.8 s | node suite |
| `make test-manifest` | 9.1 s | `fxdata-manifest_test.sh` 2.0 s + `ensure-generated_test.sh` 7.3 s |
| `make test-generated-libs` | 1.4 s | C++ + shell |
| `make test-world-script-profile` / `test-encounter-profile` | 0.7 / 0.6 s | |
| `make test-fxtest-ram` / `test-avr-build-budget` | 0.1 s each | |
| `make fxtest-build` (34 suites) | ~5:30 | 9.7 s/suite fresh; prior log 5:10 (33 suites) |
| `make fxtest-run` (34 suites) | ~35 s | 0.4–1.5 s/suite |
| `make check` (sum of parts) | ~7:05 | device stage ≈ 6:05 of it |

Reproduction: `/usr/bin/time -p make <target>` for each row; per-suite: `make fxtest-build
FXTEST_INOS=tst/fxdatatest/test_blit.ino` (9.85 s), `... test_save.ino` (9.97 s) — near-constant
because the cost is the shared compile, not the suite body.

## Root-cause evidence

- Fresh vs repeat (verbose compile trace, `build/fxtest/test_blit`):
  fresh stage 19:22:10→19:22:20 (~10 s, full compile of sketch + `src/` + core + link);
  immediate repeat of the identical command 19:22:20→19:22:22 (**~1.5 s**, only the regenerated
  sketch TU + LTO link).
- Isolated steps from the build log: the LTO link command run by hand **0.65 s**; the sketch TU
  compile **0.28 s**; arduino-cli overhead (include detection, core reuse, objcopy, avr-size)
  ~1 s. Everything else is the ~60 fresh `src/` TU compiles (~8 s).
- Not LTO-specific: a fresh no-LTO build measured **9.8 s** (same). Total CPU per suite is
  ~15.5 s user + 6.3 s sys spread over ~2.2 cores.
- Flag experiments gave no win: `-g0` 9.5 s, `-flto=4` 10.7 s, `-g0 -flto=4` 9.6 s.
- Independent full builds do scale: 2-wide 12.3 s, 4-wide 17.1 s, 6-wide 21.1 s, 8-wide 28.5 s,
  11-wide 33.3 s for identical fresh builds → ~2.8× at 6–8 jobs. This is a fallback only; the
  profile-stage fix removes the repeated work instead of racing it.
- Rosetta note: `avr-gcc` is x86_64 on arm64 (verified with `file`). A native arm64 AVR
  toolchain would likely cut compile cost but changes codegen, which invalidates the stack/RAM
  budget evidence this repo depends on — do not switch for this work.

## Prototype: one stage per flag profile

Probe (in `build/fxtest-probe/`, since removed): one stage directory whose sketch file is always
`suite.ino`; compile `test_blit` content, then `test_items` content, then `test_blit` again.

| compile | wall | result |
|---|---:|---|
| first (`test_blit`) | 9.7 s | full compile |
| second (`test_items`) | 1.4 s | sketch TU + link |
| third (`test_blit`) | 1.4 s | byte-identical hex to the first (sha256 prefix `fce61ba8db736652`) |

Edit cases on the same stage: a one-line `src/GameState.cpp` content edit rebuilt in **1.7 s**
(8 gcc invocations); a widely included header edit (`src/engine/world/Chunk.hpp`) in **2.3 s**
(18 invocations). Staleness is content-based: touch-only did not rebuild; a `#error` injected into
a stage copy of `src/GameState.cpp` correctly failed the build (exit 1).

Negative result: sharing one `--build-path` across per-suite stage directories does **not** work —
arduino-cli rebuilds everything when the sketch path changes (measured 9.5 s per suite even with
the same objects present). The stage must keep a stable sketch path (`suite.ino`) and swap its
content.

## Proposed change (one implementation bead)

Makefile-only restructure of `fxtest-build`; suite sources, harness, and flags stay untouched.

- Four persistent stages: `$(FXTEST_BUILD_DIR)/profiles/<profile>/suite` for the current profiles
  — default 26 suites; duel 6 (`test_battle_duel`, `test_duel_session`, `test_duel_resources`,
  `test_duel_ui`, `test_stack`, `test_win_exp`); facts 1 (`test_battle_facts`); compact 1
  (`test_battle_compact`).
- Refresh stage inputs (src, harness, headers, generated shims) once per profile per run; per
  suite copy `tst/fxdatatest/<name>.ino` → `suite.ino`, compile with the profile's exact current
  flags, copy `suite.ino.hex`/`.elf` to
  `$(FXTEST_BUILD_DIR)/<name>/output/<name>.ino.hex`/`.elf`, then run `check-fxtest-ram.sh` on
  the copied ELF.
- `fxtest-run`, `fxtest-spike`, `fxtest-headless`, and `FXTEST_INOS` interfaces stay unchanged
  (same per-suite output paths and `fxdata.bin` staging).
- `FXTEST_CLEAN=1` escape hatch wipes the profile stages for a from-scratch audit build.
- Optional follow-up: build the four profiles in parallel (~1:20 → ~50 s).

## Verification plan for the bead

```sh
# all 34 suites still pass; record wall time before/after
time make fxtest-headless ARDENS=/path/to/Ardens
# determinism: rebuild one suite twice, compare hex
make fxtest-build FXTEST_INOS=tst/fxdatatest/test_save.ino && shasum -a 256 build/fxtest/test_save/output/test_save.ino.hex
# stack suite unchanged (RAM/stack numbers identical)
make fxtest-spike FXTEST_SPIKE_INO=tst/fxdatatest/test_stack.ino ARDENS=/path/to/Ardens
# integrated gate
make final-gate ARDENS=/path/to/Ardens
# storage
du -sh build/fxtest
```

## Not recommended

- Dropping LTO or swapping the AVR toolchain for device suites — changes codegen and weakens the
  stack/RAM evidence the gate exists to produce.
- Unity-building `src/` into one TU — static-symbol collisions and a different build than any
  shipping configuration.
- Trimming suite coverage to save time.

## Secondary findings (small, optional)

- `make test-manifest` spends 7.3 s in `ensure-generated_test.sh` and 2.0 s in
  `fxdata-manifest_test.sh` on every check; profile it if it grows.
- `make test-beta-area` (node) 7.8 s.
- Parallel `fxtest-run` captures (xargs -P 4) would save ~25 s; Ardens `captureserial` concurrency
  is unverified and the run stage is already small compared to the build stage.
