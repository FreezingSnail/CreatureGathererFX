# CreatureGathererFX

Creature collecting demake game for the Arduboy.

## Generate and validate

Run `make gen` to generate game data and package the FX image with `cgfx-tools`. The current layout still contains `source = { legacy = ... }` entries; cgfx-tools 0.2.0 resolves those through its transitional `fxdata-build.py` bridge, so Python 3 and the vendored bridge input remain required until native symbol builders replace those entries.

Run `make check` for generation, host tests, and FX tests when [Ardens](https://github.com/tiberiusbrown/Ardens) is available. `fxtest` delegates only to the `fxtest-headless` serial harness: every concrete `tst/fxdatatest/*.ino` suite (including future save suites) must emit an exact `P` or `F` serial marker. Set `ARDENS=/path/to/Ardens` to enable this device-test stage; without it, it reports a skip. An Ardens binary without `captureserial` is blocked rather than treated as a pass (the installed 0.24.4 bundle is incompatible). CI builds the pinned headless Ardens source with `captureserial`; use that runner for device-suite execution rather than a graphical/manual Ardens path.

`tools/cgfx-tools.sh` resolves the data generator. It uses an executable set through `CGFX_TOOLS_BIN`, otherwise builds a sibling `../CreatureGathererTools` checkout, otherwise downloads the locked and checksummed release. Run `./tools/cgfx-tools.sh` to inspect the selected executable. Do not remove `Arduboy-Python-Utilities/fxdata-build.py` or `fxdata/fxdata.txt` until that transitional bridge is retired.
