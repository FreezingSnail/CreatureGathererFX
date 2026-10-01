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
