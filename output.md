# CreatureGathererFX-xrm.1

- Defined stable Make API: `setup`, `doctor`, `gen`, `test`, `testvm`, `build`, `check`, `fxtest`; `make help` documents prerequisites, outputs, optional Ardens behavior, and supported overrides.
- Replaced path-fragile host includes and sprite inputs with repository-relative paths. `FQBN`, `ARDUINO_CLI`, `BUILD_DIR`, `DIST_DIR`, `FXDATA_BIN`, and test-output variables support local/CI overrides.
- Moved host-test executables under ignored `build/`; packaged FX output uses ignored `dist/`. `check` now includes VM tests.
- Added `tools/tests/make-contract-test.sh`: validates help surface and FQBN/output directory overrides without Arduino hardware.

# CreatureGathererFX-xrm.2

Added deterministic `schema_version: 1` FX provenance manifests. Each manifest records `cgfx-tools` name/version, target, SHA-256 checksums for discovered generation inputs, every packaged FX payload, generated FX artifact, and generated device fixture.

`tools/assert-fxdata-manifest.sh` strictly validates schema and rejects malformed, missing, changed, and unrecorded files with path, hash state, and `make gen` remediation. Use `make verify-generated` for non-mutating verification; `make test-manifest` runs permanent fixture-backed shell coverage.

FX packing is native `cgfx-tools`; `fxlayout.toml` uses raw, image, C-array, builder, and typed-symbol sources without the Python bridge.
