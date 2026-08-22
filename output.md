# CreatureGathererFX-xrm.1

- Defined stable Make API: `setup`, `doctor`, `gen`, `test`, `testvm`, `build`, `check`, `fxtest`; `make help` documents prerequisites, outputs, optional Ardens behavior, and supported overrides.
- Replaced path-fragile host includes and sprite inputs with repository-relative paths. `FQBN`, `ARDUINO_CLI`, `BUILD_DIR`, `DIST_DIR`, `FXDATA_BIN`, and test-output variables support local/CI overrides.
- Moved host-test executables under ignored `build/`; packaged FX output uses ignored `dist/`. `check` now includes VM tests.
- Added `tools/tests/make-contract-test.sh`: validates help surface and FQBN/output directory overrides without Arduino hardware.
