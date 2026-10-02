# CreatureGathererFX-xrm.1

- Defined stable Make API: `setup`, `doctor`, `gen`, `test`, `testvm`, `build`, `check`, `fxtest`; `make help` documents prerequisites, outputs, optional Ardens behavior, and supported overrides.
- Replaced path-fragile host includes and sprite inputs with repository-relative paths. `FQBN`, `ARDUINO_CLI`, `BUILD_DIR`, `DIST_DIR`, `FXDATA_BIN`, and test-output variables support local/CI overrides.
- Moved host-test executables under ignored `build/`; packaged FX output uses ignored `dist/`. `check` now includes VM tests.
- Added `tools/tests/make-contract-test.sh`: validates help surface and FQBN/output directory overrides without Arduino hardware.

# CreatureGathererFX-xrm.2

Added deterministic `schema_version: 1` FX provenance manifests. Each manifest records `cgfx-tools` name/version, target, SHA-256 checksums for discovered generation inputs, every packaged FX payload, generated FX artifact, and generated device fixture.

`tools/assert-fxdata-manifest.sh` strictly validates schema and rejects malformed, missing, changed, and unrecorded files with path, hash state, and `make gen` remediation. Use `make verify-generated` for non-mutating verification; `make test-manifest` runs permanent fixture-backed shell coverage.

FX packing is native `cgfx-tools`; `fxlayout.toml` uses raw, image, C-array, builder, and typed-symbol sources without the Python bridge.

# CreatureGathererFX-b2a.1

Implementation: native `Arduboy2Base` renderer; interim plane-1 `FRAME(x)` mapping; 52 FPS loop; removed obsolete `FRAMESHIFT` after grep. Only permitted files changed: `CreatureGathererFX.ino`, `src/common.hpp`.

Commands and output tails:

```text
bd update CreatureGathererFX-b2a.1 --claim
✓ Updated issue: CreatureGathererFX-b2a.1 — Swap renderer to native Arduboy2Base 1bpp (interim plane-1 frames)

make build  # baseline
Sketch uses 18314 bytes (61%) of program storage space. Maximum is 29696 bytes.
Global variables use 2018 bytes (78%) of dynamic memory, leaving 542 bytes for local variables. Maximum is 2560 bytes.

make build  # after implementation
Sketch uses 17960 bytes (60%) of program storage space. Maximum is 29696 bytes.
Global variables use 2019 bytes (78%) of dynamic memory, leaving 541 bytes for local variables. Maximum is 2560 bytes.

make test
Total Passed: 809
Total Failed: 0
All tests passed!

make testvm
Total Passed: 10
Total Failed: 0
All tests passed!

make gen && git diff --exit-code -- fxdata/generated tst/fxdatatest/generated src/fxdata.h fxdata/Sprites.txt src/vm/opcodes.hpp src/flags
Packed FX image: /Users/connorfranc/code/CreatureGathererFX/build/cgfx-pack.gu3EeL/dist/fxdata.bin
# exit 0; generated-file diff empty

make verify-generated
# exit 0; no output

ARDENS=/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens make fxtest-headless
fxdatatest: PASS
test_arena: PASS
test_creatures: PASS
test_moves: PASS
test_opponents: PASS
test_rawread: PASS
test_save: PASS
test_tables: PASS
test_version: PASS
# all suites: P; exit 0

nm -C build/CreatureGathererFX.ino.elf | grep -E 'ArduboyG|startGray|waitForNextPlane|currentPlane' || true
# no output; no matching linked symbols

git status --short && git diff --check
 M CreatureGathererFX.ino
 M src/common.hpp
# exit 0; diff check clean
```

Size delta: flash `18314 -> 17960 B` (`-354 B`); globals `2018 -> 2019 B` (`+1 B`), `541 B` free; limit `2026 B`, headroom `7 B`.

Wall time (worker): approximately 1m15s (2026-10-01 18:13:10 EDT through 18:14:25 EDT).

Deviation: skipped `make run` because it opens interactive GUI, per worker instruction. Ardens visual check remains pending orchestrator; owner hardware check on new FX unit pending.

Automated gates: PASS. No commits, pushes, or `bd dolt push` performed.

Orchestrator (b2a.1): added the interim `FRAME` comment required by the design. Re-ran `make build` (flash 17960, globals 2019), `make test`, `make testvm`, full `make fxtest-headless` (9/9 PASS). Wall time: worker 95 s (gpt-5.6-luna). Ardens visual check and owner hardware check pending.

# CreatureGathererFX-jp8.5.10

Implementation: added the header-only lure/item id contract in `src/item/ItemIds.hpp`, host coverage in `tst/item_test.hpp`, and registered `ItemSuite` in `tst/main.cpp`. The header includes only `<stdint.h>`, keeps `ItemKind` based on `uint8_t`, and contains the required arithmetic-shape comment and compile-time assertions.

Commands and output tails:

```text
bd update CreatureGathererFX-jp8.5.10 --claim
✓ Updated issue: CreatureGathererFX-jp8.5.10 — Add the item id space header with derived type, tier, charge and rate

make build  # initial sandbox attempt
Error during build: open /Users/connorfranc/Library/Caches/arduino/sketches/0B8BB56BD4B7E2CC5D655968419FED30/build.options.json: operation not permitted

make test
ItemSuite finished
ItemSuite: Passed 32, Failed 0
Total Passed: 841
Total Failed: 0
All tests passed!

# CreatureGathererFX-jp8.5.11

Implementation: added the 32-byte indexed inventory module with saturating add, all-or-nothing take, key/out-of-range rejection, and the frozen lure-then-consumable nonzero walk. Extended the existing item suite with inventory coverage and added `Inventory.cpp` to host test sources.

Commands and results:

```text
bd update CreatureGathererFX-jp8.5.11 --claim
✓ Updated issue: CreatureGathererFX-jp8.5.11 — Add the indexed Inventory module with count, add and take

make test
ItemSuite: Passed 85, Failed 0
Total Passed: 894
Total Failed: 0
All tests passed!

make build
Sketch uses 17960 bytes (60%) of program storage space. Maximum is 29696 bytes.
Global variables use 2019 bytes (78%) of dynamic memory, leaving 541 bytes for local variables. Maximum is 2560 bytes.

make check
Host tests: 894 passed, 0 failed
VM tests: 10 passed, 0 failed
Manifest, generated-libs, generated-libs-invariants, and first-unqualified-alias checks: PASS
Ardens headless suites: fxdatatest 576, arena 462, creatures 470, moves 308, opponents 576, rawread 6, save 261, tables 241, version 1; all failed 0
# exit 0
```

Size delta: static globals `2019 -> 2019 B` (`0 B`); flash `17960 B` after the module. The inventory has no global instance.

Wall time (worker): approximately 45 seconds for implementation and requested acceptance commands (excluding earlier bead inspection).

Deviation: none. No commit or push performed.

make build  # after implementation, elevated after approval
Sketch uses 17960 bytes (60%) of program storage space. Maximum is 29696 bytes.
Global variables use 2019 bytes (78%) of dynamic memory, leaving 541 bytes for local variables. Maximum is 2560 bytes.

make check  # sandbox attempt
host tests, VM tests, manifest and generated-library checks passed
Error during build: cleaning build path: unlinkat /Users/connorfranc/Library/Caches/arduino/sketches/0760AABF6D270054967826384D4EDC9E/core: operation not permitted

make check  # elevated after approval
host tests: 841 passed, 0 failed; VM tests: 10 passed, 0 failed
manifest, generated-library, and alias checks: PASS
fxdatatest PASSED=576 FAILED=0
test_arena PASSED=462 FAILED=0
test_creatures PASSED=470 FAILED=0
test_moves PASSED=308 FAILED=0
test_opponents PASSED=576 FAILED=0
test_rawread PASSED=6 FAILED=0
test_save PASSED=261 FAILED=0
test_tables PASSED=241 FAILED=0
test_version PASSED=1 FAILED=0
# exit 0
```

Size: `make build` reports 2019 B globals and 541 B free. This matches the existing `output.md` baseline after b2a.1 (2019 B globals); the initial fresh baseline attempt was blocked by the Arduino cache permission error above. This change adds no static storage: the constants and functions are compile-time/header-only, and the new tests run on the host.

Wall time (worker): approximately 6 minutes including the approved `make build` and full `make check`; final gate (make check): approximately 25 seconds.

Deviation: the first sandbox attempts for `make build` and `make check` were blocked by Arduino cache read/cleanup permissions. Both required gates completed successfully after approval to run outside the sandbox. No commits or pushes performed.

# CreatureGathererFX-jp8.5.12

Implementation: added `item::KeyItems`, a one-byte unlocked-bit store in the independent key-item ID namespace, with clear, query, unlock, and out-of-range guards. Extended `ItemSuite` to verify per-ID bits, idempotence, invalid IDs, and isolation from `FLAG_BIT_ARRAY`; registered `KeyItems.cpp` in host sources.

Commands and results:

```text
bd update CreatureGathererFX-jp8.5.12 --claim
✓ Updated issue: CreatureGathererFX-jp8.5.12 — Add the key-item unlocked bit array in its own id namespace

make test
ItemSuite: Passed 108, Failed 0
Total Passed: 917
Total Failed: 0
All tests passed!

make build
Sketch uses 17960 bytes (60%) of program storage space. Maximum is 29696 bytes.
Global variables use 2019 bytes (78%) of dynamic memory, leaving 541 bytes for local variables. Maximum is 2560 bytes.

make check
Host tests: 917 passed, 0 failed
VM tests: 10 passed, 0 failed
Manifest, generated-libs, generated-libs-invariants, and first-unqualified-alias checks: PASS
Ardens headless suites: fxdatatest 576, arena 462, creatures 470, moves 308, opponents 576, rawread 6, save 261, tables 241, version 1; all failed 0
# exit 0

git diff --check
# exit 0
```

Size delta: static globals `2019 -> 2019 B` (`0 B`); flash remains `17960 B`. This module has no global instance, and the size matches the jp8.5.11 baseline.

Wall time: acceptance commands approximately 51 seconds (`make test` 2 s, `make build` 5 s, `make check` 44 s); total worker elapsed time approximately 1m41s, 2026-10-01 20:16:25–20:18:06 EDT.

Deviation: none. No commits or pushes performed.

# CreatureGathererFX-jp8.5.15

Implementation: appended 3 lure tier, 8 lure type, and 8 consumable name strings and order entries; added `item::ItemName`/`itemNameAddr` with tier-then-type lure resolution and guarded IDs; extended `ItemSuite` for index arithmetic and FX read counts. The existing declaration and order counts were **104**, not the bead's stale 106; both now have **123** entries with the first 104 preserved and no duplicate declaration names. `make gen-sprites` and `make gen` published the three new index tables in `src/fxdata.h`.

Kept the historical 304-entry pack-layout migration proof by excluding only the 19 new sprites and 3 new index tables from its two expanded fixtures. Both the positive equivalence and intentionally perturbed negative diagnostic pass. Refreshed the packed-image SHA-256 baseline to `42873ff66900cdd6b135de95c3be0088dddf0a487b05f29d69b67c068cf1b57c`; updated the independently pinned `MenuStrings` table address in the first-unqualified-alias check to `0x04F8C3`.

Commands and results:

```text
make gen-sprites                 PASS (0.044 s)
make gen                         PASS (5.37 s)
make verify-generated            PASS (2.73 s)
make test-manifest               PASS (2.12 s)
make test-generated-libs         PASS after updating the pinned MenuStrings address (1.54 s)
  generated-libs: 6/0; invariants: 5/0; first-unqualified-alias: 28/0
make test                        PASS (1.73 s); host 936 passed, 0 failed; ItemSuite 127/0
make test-pack-parity             PASS after refreshing expected SHA (6.16 s)
  layout equivalence, perturbation diagnostic, and image hash: PASS
make build                       PASS after approved cache-access rerun (5.81 s)
  flash 17,960/29,696 B (60%); globals 2,019/2,560 B (541 B free)
make check                       PASS (48.67 s)
  host 936/0; VM 10/0; manifest, generated-libs, alias and generation checks: PASS
  Ardens: fxdatatest 576/0, arena 462/0, creatures 470/0, moves 308/0,
          opponents 576/0, rawread 6/0, save 261/0, tables 241/0, version 1/0
git diff --check                 PASS
```

Size delta: flash and global SRAM match the prior item-bead baseline (17,960 B and 2,019 B). `itemNameAddr` is not linked into the shipping sketch yet; it has no current device-size effect.

Wall time: approximately 9 minutes including fixture investigation and gate runs; full final gate 48.67 seconds. Deviations: initial `make test-generated-libs` found the old pinned table address, and the first `make test-pack-parity` observed the expected old SHA mismatch; both were corrected and rerun. First sandbox `make build` attempt could not clean the Arduino cache; the approved escalated rerun passed. No commit or push performed.

# CreatureGathererFX-qu9.2 — pre-fix stack baseline

Implementation: scaffolded the permanent device suite with `make new-fxtest NAME=stack`. It paints `[&__bss_end, SP - 64)` through a volatile pointer, runs the VM, battle turn, menu render, and save compaction, then reports base/top/low-water/headroom and checks collision plus the pinned 128 B minimum. The large `SaveFile` snapshot lives in a noinline save helper so the initial paint runs from a shallow frame. The test sketch supplies the production `ScriptVm` global omitted by the shared FX test harness.

Commands and results:

```text
bd update CreatureGathererFX-qu9.2 --claim
✓ Updated issue: CreatureGathererFX-qu9.2 — Add painted-stack low-water-mark FX test suite

make new-fxtest NAME=stack
new-fxtest: created tst/fxdatatest/stack_test.hpp and tst/fxdatatest/test_stack.ino

make fxtest-headless FXTEST_INOS=tst/fxdatatest/test_stack.ino ARDENS=/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens
# first sandbox attempt: arduino-cli could not create its cache directory (operation not permitted)
# approved rerun, before save-frame isolation: test_stack: FAIL (no serial)

make fxtest-headless FXTEST_INOS=tst/fxdatatest/test_stack.ino FXTEST_MS=10000 ARDENS=/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens
# diagnostic markers reached VM and player.basic, then stopped in startFight
# diagnostic scan before startFight: base=0x8B2 (2226), top=0x858 (2136)
# SaveFile had been allocated in test_stack's entry frame, pushing SP below BSS before paint.
# Disabling the paint loop did not change that stop. The save snapshot was moved to runSave().

make fxtest-headless FXTEST_INOS=tst/fxdatatest/test_stack.ino ARDENS=/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens
Sketch uses 20460 bytes (68%) of program storage space. Maximum is 29696 bytes.
Global variables use 1970 bytes (76%) of dynamic memory, leaving 590 bytes for local variables. Maximum is 2560 bytes.
=== test_stack ===
stack base=0x8B2 (2226)
stack top=0xAA1 (2721)
stack low=0x8B2 (2226)
stack headroom=0x0 (0)
FAIL stack does not collide with globals got=0 want=1
FAIL stack headroom >= 128 B got=0 want=1
test_stack PASSED=1 FAILED=2
F
test_stack: FAIL
# exit 2; expected pre-fix failure, with save compaction completion passing

git diff --check
# exit 0
```

Baseline: the low-water mark clobbered BSS at `0x8B2` (0 B headroom) while the pinned minimum is 128 B. The suite completes the save chain and emits the exact `F` marker. This is the required pre-qu9.1/qu9.3 baseline; rerun after those fixes and require `P` before closing qu9.2. The full device gate is intentionally deferred to the orchestrator's integrated stack wave.

Wall time: approximately 5 minutes for the worker, including diagnosis and targeted reruns; final targeted device run 1.6 seconds. Deviation: initial sandbox cache access needed the already approved elevated rerun. No commit, push, or bd closure performed.

# CreatureGathererFX-qu9.3 — narrow Effect enum and discard legacy save records

Changes: `Effect` now has `uint8_t` underlying storage; range helpers and battle/move callsites use explicit casts. Host tests assert `Move` is 4 B and record host `Creature` size 34 B; device tests assert AVR `Effect` is 1 B, `Move` is 4 B, and `Creature` is 33 B. Current SaveFile v1 embedded runtime `Creature` bytes, so the old AVR 157 B record cannot be interpreted safely after this narrowing. The loader skips only 157 B records whose payload version byte equals `SAVE_VERSION`, then continues scanning; a legacy-only sector returns false without modifying the caller's output. A subsequent commit erases a legacy-first save sector before writing the current format. `SAVE_VERSION` and the v2 schema are unchanged; jp8.2.6 owns the new schema.

Commands and results:

```text
make test
# Total Passed: 942; Total Failed: 0

make build
# Sketch uses 18,024 bytes; globals 1,917 B, down 102 B from 2,019 B baseline.
# AVR Creature size: 33 B; Move size: 4 B.

make fxtest-headless FXTEST_INOS=tst/fxdatatest/test_creatures.ino
# test_creatures PASSED=470 FAILED=0; compiled AVR size assertions pass.

make check
# host: 942 passed, 0 failed; VM: 10 passed, 0 failed
# manifest, generated-libs, generated-libs-invariants, and alias tests passed
# full fxtest: all suites passed except test_stack, which emitted the expected
#   qu9.2 pre-qu9.1 failure: 0 B headroom; collision and 128 B minimum failed.
# exit 2 from make check due to the expected in-flight qu9.2 baseline.

make fxtest-headless
# 9/10 device suites passed; test_stack reported expected F, 0 B headroom.
# exit 2; rerun after qu9.1 and require P before closing qu9.2 or qu9.3.
```

Save compatibility: the old AVR v1 157 B record is deliberately discarded. `saveFileLoad` accepts no legacy payload and leaves its output unchanged; it skips the legacy bytes so a current-format record later in the sector remains discoverable. On the first subsequent commit, the old sector is erased before saving the narrowed runtime representation. No other save schema fields or version values changed.

Wall time: approximately 20 minutes including code, save-path inspection, and gates; final host suite 1.7 seconds, production build 4.0 seconds, integrated gate 42 seconds. Deviation: full device validation cannot pass until the separate qu9.1 stack remediation; the qu9.2 baseline's exact expected `F` was retained. Bead remains open pending the integrated green stack gate. No commit, push, or bd completion update performed.

# CreatureGathererFX-qu9.1 — save-path stack remediation (BLOCKED on 400 B acceptance)

Changed `journalCount` and `journalReplay` from 256 B page buffers to 32 B record-aligned windows. `saveFileLoad` now tracks the address of the latest validated record, then reads that one record into `out` only after finding a valid candidate. A failed scan leaves `out` untouched; SAVE_VERSION and record layout are unchanged. qu9.3's narrowed Effect is already present.

Measured after each step with the permanent painted-stack suite:

```text
make fxtest-headless FXTEST_INOS=tst/fxdatatest/test_stack.ino ARDENS=/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens
# After 32 B journal window alone: base=0x84C, top=0xAA5, low=0x88B,
# headroom=63 B; PASSED=2 FAILED=1, F (128 B pin still failed).

make fxtest-headless FXTEST_INOS=tst/fxdatatest/test_stack.ino ARDENS=/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens
# After last-valid-address load: base=0x84C, top=0xAA5, low=0x908,
# headroom=188 B; PASSED=3 FAILED=0, P. qu9.1 >=400 B target misses by 212 B.

make build
# Sketch 17,966 B flash; globals 1,917/2,560 B, 643 B free.

make gen
# PASS; no generated-file diff.
make test
# 942 passed, 0 failed.
make testvm
# 10 passed, 0 failed.
make verify-generated
# PASS.
make fxtest-headless FXTEST_INOS=tst/fxdatatest/test_save.ino ARDENS=/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens
# Device save suite PASSED=211 FAILED=0, P; exercises two commits, latest-record
# selection, journal replay, compaction, and save/reload of serialized fields.
git diff --check
# PASS.
```

Production LTO ELF: `saveStepAdvance` is inlined into `main`; `main` reserves 179 B (`subi r28, 0xB3`), `saveFileLoad` reserves 129 B plus 16 register saves = 145 B, and `flash_backend_detail::readBytes` saves 4 B. This is roughly 334 B along the main → save load → flash read chain with three 2 B return addresses, below the 400 B chain cap, but the test's measured low-water is the stricter recorded fact. `sizeof(SaveFile)` is 127 B on AVR; qu9.3's `Effect` is 1 B. A 69 B USB ISR reserve fits within the 188 B device mark, leaving 119 B in this test scenario.

The explicit qu9.1 acceptance of **at least 400 B painted-stack headroom** is unmet (188 B measured; 212 B deficit). Reaching that number requires another stack reduction, likely avoiding the 127 B scan candidate and/or the 127 B compaction snapshot in the device chain; both exceed this bead's pinned two changes and need a separately reviewed design. An alternate interpretation would be to amend the acceptance to a measured safe reserve, but that must be a deliberate plan correction; do not mark this bead complete from a green `P` alone. The suite's 128 B pin has not been raised. The worker did not run `make check`, commit, close the bead, or claim the 400 B criterion passed. No physical-device interactive state inspection was performed.

Worker wall time: about 3 minutes including implementation, two device stack measurements, host/VM/device save checks, generation verification, and LTO frame inspection.

# CreatureGathererFX-qu9.11 — reach 400 B painted stack headroom

At clean HEAD `9033e45`, the original 188 B painted headroom reproduced. Streaming validation in `SaveFile.cpp` computes the same byte-ordered, modulo-255 checksum through an 8 B flash window; the scanner still skips 157 B legacy v1 records and selects the last valid current record before writing to the caller's output. Compaction now loads only a validated party baseline into its live snapshot and verifies the stored record in bounded chunks, removing full-record replay and verify locals. Its mutable snapshot already has version and checksum set before commit, so compaction calls `saveFileCommitPrepared` and avoids a second 127 B copy; the public `saveFileCommit` still normalizes its input before calling that helper. `SAVE_VERSION` and the on-flash schema did not change. Host coverage checks last-valid selection, invalid-tail fallback, preservation of fields outside party, and byte-for-byte preservation after a failed party load.

Measured checkpoints with `make fxtest-headless FXTEST_INOS=tst/fxdatatest/test_stack.ino ARDENS=/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens` after each change:

| Checkpoint | Stack base | Low-water | Headroom | Result |
| --- | --- | --- | ---: | --- |
| Starting HEAD | `0x84C` | `0x908` | 188 B | P at old 128 B pin |
| 8 B streaming validation | `0x84C` | `0x979` | 301 B | P at old pin |
| Party-only replay and bounded verification | `0x84C` | `0x99B` | 335 B | P at old pin |
| Prepared compaction commit, diagnostic build | `0x84C` | `0x9F1` | 421 B | P at old pin |
| Final permanent suite, `MIN_HEADROOM=400` | `0x84C` | `0x9F8` | **428 B** | PASSED=3 FAILED=0, P |

The temporary pre-save diagnostic showed 577 B headroom before `runSave`, so the peak belongs to the save chain. The diagnostic was removed before the final measurement. Final test globals are 1,868 B of 2,560 B; 692 B remain, and the observed peak stack use is 264 B. The 428 B painted headroom contains the 69 B USB ISR reserve. The suite continues to create its 127 B conservative local snapshot, perform a full compaction, and measure all resident test globals.

Exact final worker commands and results:

```text
make build
# PASS; 17,736 B flash, 1,857 B globals (703 B free).

make fxtest-headless FXTEST_INOS=tst/fxdatatest/test_stack.ino ARDENS=/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens
# PASS; base=0x84C, top=0xAA5, low=0x9F8, headroom=428 B; P.

make fxtest-headless FXTEST_INOS=tst/fxdatatest/test_save.ino ARDENS=/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens
# PASS; 229 passed, 0 failed; P. Covers repeated commits, save/load fields,
# latest-record selection, journal replay, compaction, and controller behavior.

make test
# PASS; 953 passed, 0 failed, including 11 new streaming-selection assertions.
make testvm
# PASS; 10 passed, 0 failed.
make verify-generated
# PASS; no generated artifacts changed.
git diff --check
# PASS.
```

Production LTO ELF from `make build`: `saveStepAdvance` and `saveFileLoad` are inlined into callers; `main` reserves 48 B (down from 179 B at baseline), while `latestValidRecordAddr` saves 18 registers plus a 17 B local frame, 35 B total. `flash_backend_detail::readBytes` saves 4 B. The representative `main` → latest-valid scan → flash read chain is about 91 B including two 2 B return addresses, below the 400 B chain cap; the painted suite directly measured a larger conservative 264 B peak. Final device `runSave` reserves 169 B plus 18 register saves = 187 B. No physical hardware check was performed; the Ardens save and stack suites are green.

Worker wall time: about 5 minutes (02:43–02:48 UTC) for three measured trims, diagnostics, focused tests, LTO frame review, and reporting. Deviation: reaching 400 B required two additional production-chain copy trims after the first streaming-validation checkpoint; the rejected option was lowering the 400 B threshold.

Orchestrator integrated gate:

```text
make check
# exit 0: host 953/0; VM 10/0; generated-data and library checks pass.
# All 10 Ardens device suites print P; test_stack headroom=428 B at
# MIN_HEADROOM=400. Wall time: about 17 seconds.
```

The integrated gate is green. qu9.11 reaches its 400 B target at 428 B, with no save schema or version change. The previously blocked qu9.1 stack acceptance is now met and can be closed after this implementation commit.

# CreatureGathererFX-qu9.2 — post-fix pass and threshold assertion

After qu9.1/qu9.3, the focused stack suite measures 188 B headroom and passes at its committed 128 B minimum. Temporarily raising the minimum to 189 B produces the expected F with the measured 188 B, then restoring 128 B produces P. The committed suite remains pinned at 128 B.

```text
make fxtest-headless FXTEST_INOS=tst/fxdatatest/test_stack.ino ARDENS=/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens
# exit 0: base=0x84C, top=0xAA5, low=0x908, headroom=188; PASSED=3 FAILED=0; P.

# Temporarily set MIN_HEADROOM=189, then run the same targeted command.
make fxtest-headless FXTEST_INOS=tst/fxdatatest/test_stack.ino ARDENS=/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens
# exit 2: headroom remains 188; the headroom assertion fails and emits F.
# Restore MIN_HEADROOM=128 and rerun the first command; it exits 0 and emits P.
```

The threshold flip confirms the assertion direction. The 400 B qu9.1 criterion remains unmet and is recorded separately under that bead.

# CreatureGathererFX-qu9.4 — move save status glyphs to PROGMEM

`SaveController::drawSavingStatus` now reads the 6 x 5 `saving` and `failedText` glyph matrices from AVR internal flash with `pgm_read_byte`, once per row. This keeps the 6-by-5 glyph indexing intact and does not access the external FX SPI bus while the save flash is busy. Corrected the controller comments to explain that distinction; the FX font path remains unused here.

```text
make build
# Sketch: 17,942 B flash; globals: 1,857 B, 703 B free.
# Prior integrated production build in the qu9.1 report: 17,966 B flash,
#   1,917 B globals. Delta: -24 B flash, -60 B globals.

make fxtest-headless FXTEST_INOS=tst/fxdatatest/test_save.ino ARDENS=/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens
# exit 0; test_save PASSED=229 FAILED=0; P.
# Confirms busy before/after drawStatus, the save remains active, framebuffer
# pixels for ink/paper across all six SAVING glyphs, and FAILED F/background.

make check
# exit 0; host 942/0, VM 10/0, generated-data checks pass, and all 10 Ardens
# device suites print P (test_save 229/0; test_stack 188 B headroom).
```

The device suite observes an asynchronous save-sector erase in progress before and after `SaveController::drawStatus`, confirms the save remains active, and samples framebuffer pixels for SAVING ink and paper plus the inactive-save FAILED glyph. A first assertion run exposed that framebuffer bits are set for paper and cleared for ink; expectations were corrected and the final run passed. No screenshot is available in Ardens serial mode. Worker wall time: approximately 7 minutes including the focused coverage follow-up. The integrated gate took 26 seconds; `git diff --check` passes.

# CreatureGathererFX-qu9.7 — remove inert debug optimization flag

Removed `--optimize-for-debug` from the FX production build, Mini build, and
FX-suite compile recipes. On the installed `arduboy-homemade:avr` 1.4.0 core,
`compiler.optimization_flags` is empty and `compiler.cpp.flags` hardcodes
`-Os`, so the flag did not select different compiler options.

```text
make build                         # before removal; exit 0
# Sketch: 17,942 B flash; globals: 1,857 B, 703 B free.
shasum -a 256 build/CreatureGathererFX.ino.elf
# a396abf9abfd1391d45880a449b83ad15a46b4c386cd46f05fb7cf90b05672d3

arduino-cli compile --fqbn "arduboy-homemade:avr:arduboy-fx" --output-dir "build" .
# without the flag; exit 0, same flash and globals.
shasum -a 256 build/CreatureGathererFX.ino.elf
# a396abf9abfd1391d45880a449b83ad15a46b4c386cd46f05fb7cf90b05672d3

make build                         # after removal; exit 0
# Sketch: 17,942 B flash; globals: 1,857 B, 703 B free.
shasum -a 256 build/CreatureGathererFX.ino.elf
# a396abf9abfd1391d45880a449b83ad15a46b4c386cd46f05fb7cf90b05672d3

make -n mini fxtest-build
# PASS: both dry-run compile recipes omit --optimize-for-debug.
rg -n 'optimize-for-debug|debug build' Makefile README.md .vscode/launch.json
# exit 1 (no matches).
git diff --check
# PASS.
```

The first no-flag `arduino-cli compile` attempt failed because sandbox access to
the Arduino sketch cache was denied (`operation not permitted`). Retrying the
same command with approved escalation passed; no source change was needed for
that deviation. `make build` completed in about 3 seconds each; the direct
compile took about 2 seconds after escalation; dry-run and diff checks were
under 1 second. Worker elapsed time, including investigation and reporting:
about 6 minutes. The build sizes here include the concurrent qu9.4 source
change and are only used to compare identical inputs with and without the flag.
The worker did not run `make check`, commit, or update/close the bead.

Orchestrator integrated gate after the Makefile change:

```text
make check
# exit 0; host 942/0, VM 10/0, generated-data checks pass, all ten Ardens
# suites pass. test_stack reports headroom=188 B and passes its current 128 B
# floor. Build/test global-RAM summaries and the ELF are unchanged.
git diff --check
# PASS.
```

The integrated gate took about 28 seconds. The 188 B stack result is expected
for the separate qu9.11 trim bead and does not meet qu9.1's 400 B acceptance.

# CreatureGathererFX-jp8.1.4 — RAM report and CDC linkage diagnosis

Added `make ram`, which depends on the FX build, resolves `avr-size` and
`avr-nm` from PATH or an installed Arduino AVR-GCC package without pinning a
toolchain version, prints the section totals and stable `RAM_*` keys, then
lists the largest named `.data`/`.bss` records. `RAM_STATIC_BYTES` uses
`avr-size`'s Data section total (including unnamed linker bookkeeping); the
symbol list is attribution only. Added target/help contract coverage and CI
logging via `make ram`, which builds first. README documents the total vs
symbol-list distinction.

```text
make --no-print-directory ram BUILD_DIR=build/ram-check
# exit 0; isolated ELF: build/ram-check/CreatureGathererFX.ino.elf
# Sketch uses 17736 bytes; globals 1857 bytes; free 703 bytes.
# AVR Memory Usage: Program 17736, Data 1857.
# RAM_ELF=build/ram-check/CreatureGathererFX.ino.elf
# RAM_FLASH_BYTES=17736
# RAM_FLASH_LIMIT=29696
# RAM_STATIC_BYTES=1857
# RAM_STATIC_LIMIT=2560
# RAM_FREE_BYTES=703
# Top records: sBuffer 1024, engine 144, saveState 127, player 112,
# dialogMenu 85. The changed records reflect the already-landed qu9.11 save
# work; the historical bead values are not from this current ELF.

tools/tests/make-contract-test.sh
# PASS: make contract.

make test
# PASS: 953 passed, 0 failed.
make testvm
# PASS: 10 passed, 0 failed.
make verify-generated
# exit 0.
make test-manifest
# PASS.
make test-generated-libs
# PASS: generated libs 6/0; invariants 5/0; alias checks 28/0.

git diff --check
# PASS.
```

CDC diagnosis: production sketch code has no active `Serial` or
`HardwareSerial` calls. `src/common.hpp` includes Arduboy2; the installed
Arduboy core's `main.cpp` calls `serialEventRun`, and `CDC.cpp` defines the USB
`Serial_` object while `USBCON` is enabled. The current isolated ELF has
`Serial` 80 B, `_ZTV7Serial_` 18 B, and `_ZL12_usbLineInfo` 8 B. The commented
serial lines in the main sketch are left alone, and the exact P/F serial test
protocol under `tst/fxdatatest/` is untouched. `ARDUBOY_NO_USB` remains
disabled; any CDC removal and its USB upload/reset tradeoff remain with qu9.9.

The first sandboxed RAM build could not read Arduino CLI's sketch cache
(`operation not permitted`); the same command completed under approved
escalation. Build and report took about 32 seconds including that retry; the
contract and host/VM/generation checks took about 3 seconds total. Worker
elapsed time excluding the pause for qu9.11's gate: about 7 minutes. The
worker did not run `make check`, commit, or update/close the bead.

Orchestrator integrated gate:

```text
make check
# exit 0: host 953/0; VM 10/0; generated-data and library checks pass.
# All ten Ardens device suites print P; test_stack headroom=428 B.
# Wall time: about 17 seconds.
git diff --check
# PASS.
```

# Wave 1 — save layout, ListView, and HP/save sequencing decision

## CreatureGathererFX-jp8.2.1 — reserve the 8-sector save region

Verified the installed ArduboyFX implementation at
`/Users/connorfranc/Library/Arduino15/packages/arduboy-homemade/hardware/avr/1.4.0/libraries/ArduboyFX/src/ArduboyFX.cpp`:
`loadGameState` calls `seekSave(0)` at line 538 (and its non-AVR path at 575),
while `saveGameState` checks `(addr + size) > 4094` and erases block 0 at lines
688-692. Added the eight
sector layout with `save_main`, `save_log`, the store pair, and four named
reserved sectors. Regenerated the published constants and intentionally updated
the pack parity baseline. Image size changed from 621,568 to 646,144 bytes
(+24,576); `FX_SAVE_BYTES` is 32,768, `FX_DATA_PAGE` changed `0xf684` to
`0xf624`, and `FX_SAVE_PAGE` changed `0xffe0` to `0xff80`.

```text
make gen
# PASS; packed FX image emitted.
make verify-generated
# PASS.
make test-manifest
# fxdata-manifest: PASS.
make test-pack-parity
# layout equivalence and perturbation diagnostics PASS; pack parity: PASS.
make test
# 1,098 passed, 0 failed (included the concurrently added ListView suite).
make fxtest-headless ARDENS=/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens
# 10/10 suites PASS; test_save 229/0; test_stack headroom=428 B.
```

Worker elapsed time: about 2 minutes 47 seconds. Generated manifest and image
artifacts are ignored; tracked generated output is staged with its layout source.

## CreatureGathererFX-jp8.4.8 — add ListView

Added the 4-byte, non-wrapping ListView value type, implementation and host
suite. `make build` confirms static RAM remains at 1,857 bytes (703 free), the
same as the prior baseline. The first sandboxed build could not read the
Arduino CLI cache; the approved retry completed successfully.

```text
make test
# ListViewSuite printed; 1,098 passed, 0 failed.
make build
# PASS after approved retry; flash 17,736 B, globals 1,857 B, free 703 B.
git diff --check
# PASS.
```

Worker elapsed time: about 4 minutes. The integrated gate below includes the
new suite.

## CreatureGathererFX-jp8.7 — settle HP and save version ownership

Chose Player as persistent HP owner and BattleState as the transient owner
during a live session. BattleSession imports HP at battle start, writes it back
on every terminal exit, and synchronizes active HP before a save from BATTLE.
The explicit `partyHP[3]` field bumps M0 from SaveFile v1 to v2; the M1 store
rewrite bumps v2 to v3. Both changes intentionally invalidate the previous
pre-release schema. Updated the jp8.1.10, jp8.2/jp8.2.6, and jp8.3.11 bead notes;
no extra integration bead was needed.

```text
bd show CreatureGathererFX-jp8.1.10 --json
bd show CreatureGathererFX-jp8.2.6 --json
bd show CreatureGathererFX-jp8.3.11 --json
# Decision override notes and updated v3 titles confirmed.
# All updates succeeded. No code tests apply to this planning bead.
```

Orchestrator investigation/decision time: about 6 minutes.

## Wave 1 integrated gate

```text
make check ARDENS=/Users/connorfranc/code/Ardens/build/Ardens.app/Contents/MacOS/Ardens
# exit 0; host 1,098/0; VM 10/0; manifest PASS; generated libs 6/0;
# invariants 5/0; alias checks 28/0; all ten Ardens suites print P;
# test_stack headroom=428 B.
git diff --check
# PASS.
```

Orchestrator gate elapsed time: about 22 seconds.
